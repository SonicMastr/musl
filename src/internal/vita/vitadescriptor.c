#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/net/net.h>

#include "vitadescriptor.h"
#include "vitaerror.h"

DescriptorTranslation *__fdmap[MAX_OPEN_FILES];
DescriptorTranslation __fdmap_pool[MAX_OPEN_FILES];

SceKernelLwMutexWork _musl_fd_mutex __attribute__ ((aligned (8)));

void _init_vita_io(void)
{
	int ret;
	sceKernelCreateLwMutex(&_musl_fd_mutex, "fd conversion table mutex", SCE_KERNEL_MUTEX_ATTR_RECURSIVE, 1, NULL);

	memset(__fdmap, 0, sizeof(__fdmap));
	memset(__fdmap_pool, 0, sizeof(__fdmap_pool));

	// TODO: do we prefer sceKernelStdin and friends?
	ret = sceIoOpen("tty0:", SCE_O_RDONLY, 0);

	if (ret >= 0)
	{
		__fdmap[STDIN_FILENO] = &__fdmap_pool[STDIN_FILENO];
		__fdmap[STDIN_FILENO]->sce_uid = ret;
		__fdmap[STDIN_FILENO]->type = VITA_DESCRIPTOR_TTY;
		__fdmap[STDIN_FILENO]->ref_count = 1;
	}

	ret = sceIoOpen("tty0:", SCE_O_WRONLY, 0);

	if (ret >= 0)
	{
		__fdmap[STDOUT_FILENO] = &__fdmap_pool[STDOUT_FILENO];
		__fdmap[STDOUT_FILENO]->sce_uid = ret;
		__fdmap[STDOUT_FILENO]->type = VITA_DESCRIPTOR_TTY;
		__fdmap[STDOUT_FILENO]->ref_count = 1;
	}

	ret = sceIoOpen("tty0:", SCE_O_WRONLY, 0);

	if (ret >= 0)
	{
		__fdmap[STDERR_FILENO] = &__fdmap_pool[STDERR_FILENO];
		__fdmap[STDERR_FILENO]->sce_uid = ret;
		__fdmap[STDERR_FILENO]->type = VITA_DESCRIPTOR_TTY;
		__fdmap[STDERR_FILENO]->ref_count = 1;
	}

	sceKernelUnlockLwMutex(&_musl_fd_mutex, 1);
}

void _free_vita_io(void) {
	sceKernelLockLwMutex(&_musl_fd_mutex, 1, 0);

	if (__fdmap[STDIN_FILENO])
	{
		sceIoClose(__fdmap[STDIN_FILENO]->sce_uid);
		memset(__fdmap[STDIN_FILENO], 0, sizeof(DescriptorTranslation));
		__fdmap[STDIN_FILENO] = NULL;
	}
	if (__fdmap[STDOUT_FILENO])
	{
		sceIoClose(__fdmap[STDOUT_FILENO]->sce_uid);
		memset(__fdmap[STDOUT_FILENO], 0, sizeof(DescriptorTranslation));
		__fdmap[STDOUT_FILENO] = NULL;
	}
	if (__fdmap[STDERR_FILENO])
	{
		sceIoClose(__fdmap[STDERR_FILENO]->sce_uid);
		memset(__fdmap[STDERR_FILENO], 0, sizeof(DescriptorTranslation));
		__fdmap[STDERR_FILENO] = NULL;
	}

	sceKernelUnlockLwMutex(&_musl_fd_mutex, 1);
	sceKernelDeleteLwMutex(&_musl_fd_mutex);
}

int __acquire_descriptor(void)
{
	int fd = -1;
	int i = 0;
	sceKernelLockLwMutex(&_musl_fd_mutex, 1, 0);

	// get free descriptor
	// only allocate descriptors after stdin/stdout/stderr -> aka 0/1/2
	for (fd = 3; fd < MAX_OPEN_FILES; ++fd)
	{
		if (__fdmap[fd] == NULL)
		{
			// get free pool
			for (i = 0; i < MAX_OPEN_FILES; ++i)
			{
				if (__fdmap_pool[i].ref_count == 0)
				{
					__fdmap[fd] = &__fdmap_pool[i];
					__fdmap[fd]->ref_count = 1;
					sceKernelUnlockLwMutex(&_musl_fd_mutex, 1);
					return fd;
				}
			}
		}
	}

	// no mores descriptors available...
	sceKernelUnlockLwMutex(&_musl_fd_mutex, 1);
	return -1;
}

int __release_descriptor(int fd)
{
	int res = -1;

	sceKernelLockLwMutex(&_musl_fd_mutex, 1, 0);

	if (is_fd_valid(fd) && __fd_drop(__fdmap[fd]) >= 0)
	{
		__fdmap[fd] = NULL;
		res = 0;
	}

	sceKernelUnlockLwMutex(&_musl_fd_mutex, 1);
	return res;
}

int __duplicate_descriptor(int fd)
{
	int fd2 = -1;

	sceKernelLockLwMutex(&_musl_fd_mutex, 1, 0);

	if (is_fd_valid(fd))
	{
		// get free descriptor
		// only allocate descriptors after stdin/stdout/stderr -> aka 0/1/2
		for (fd2 = 3; fd2 < MAX_OPEN_FILES; ++fd2)
		{
			if (__fdmap[fd2] == NULL)
			{
				__fdmap[fd2] = __fdmap[fd];
				__fdmap[fd2]->ref_count++;
				sceKernelUnlockLwMutex(&_musl_fd_mutex, 1);
				return fd2;
			}
		}
	}

	sceKernelUnlockLwMutex(&_musl_fd_mutex, 1);
	return -1;
}

int __descriptor_ref_count(int fd)
{
	int res = 0;
	sceKernelLockLwMutex(&_musl_fd_mutex, 1, 0);
	res = __fdmap[fd]->ref_count;
	sceKernelUnlockLwMutex(&_musl_fd_mutex, 1);
	return res;
}

DescriptorTranslation *__fd_grab(int fd)
{
	DescriptorTranslation *map = NULL;

	sceKernelLockLwMutex(&_musl_fd_mutex, 1, 0);

	if (is_fd_valid(fd))
	{
		map = __fdmap[fd];

		if (map)
			map->ref_count++;
	}

	sceKernelUnlockLwMutex(&_musl_fd_mutex, 1);
	return map;
}

int __fd_drop(DescriptorTranslation *map)
{
	sceKernelLockLwMutex(&_musl_fd_mutex, 1, 0);

	if (map->ref_count == 1)
	{
		int ret = 0;

		switch (map->type)
		{
			case VITA_DESCRIPTOR_FILE:
			case VITA_DESCRIPTOR_TTY:
			{
				ret = sceIoClose(map->sce_uid);
				if (map->filename)
				{
					free(map->filename);
				}
				break;
			}
			case VITA_DESCRIPTOR_DIRECTORY:
			{
				ret = sceIoDclose(map->sce_uid);
				if (map->filename)
				{
					free(map->filename);
				}
				break;
			}
			case VITA_DESCRIPTOR_SOCKET:
			{
				ret = sceNetSocketClose(map->sce_uid);
				if (ret < 0)
				{
					sceKernelUnlockLwMutex(&_musl_fd_mutex, 1);
					return -__scenet_errno_to_errno(ret);
				}
				break;
			}
			case VITA_DESCRIPTOR_PIPE:
			{
				ret = sceKernelDeleteMsgPipe(map->sce_uid);
				break;
			}
		}

		if (ret < 0)
		{
			sceKernelUnlockLwMutex(&_musl_fd_mutex, 1);
			return -__sce_errno_to_errno(ret, ERROR_GENERIC);
		}

		map->ref_count--;
		memset(map, 0, sizeof(DescriptorTranslation));
	}
	else
	{
		map->ref_count--;
	}

	sceKernelUnlockLwMutex(&_musl_fd_mutex, 1);
	return 0;
}