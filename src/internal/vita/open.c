#include "open.h"
#include "vitafs.h"
#include "vitadescriptor.h"
#include "vitaerror.h"

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/clib.h>

int __vita_open(const char *path, int oflag)
{
    int ret;
    int sce_flags = _fcntl2sony(oflag);
    int is_dir = 0;

    // check flags

    char *full_path = __realpath(path);
    if (!full_path)
    {
        return -1;
    }

    // get full path and stat. if dir - use dir funcs, otherwise - file
    // if O_DIRECTORY passed - check that path is indeed dir and return ENOTDIR otherwise
    // ENOENT, etc is handled by sce funcs
    if (__is_dir(full_path) == 0)
    {
        is_dir = 1;
    }
    if (oflag & O_DIRECTORY && !is_dir)
    {
        free(full_path);
        return -ENOTDIR;
    }

    ret = is_dir ? sceIoDopen(full_path) : sceIoOpen(full_path, sce_flags, 0666);
    if (ret < 0)
    {
        free(full_path);
        return __sce_errno_to_errno(ret, ERROR_GENERIC);
    }

    int fd = __acquire_descriptor();

    if (fd < 0)
    {
        free(full_path);
        is_dir ? sceIoDclose(ret) : sceIoClose(ret);
        return -EMFILE;
    }

    __fdmap[fd]->sce_uid = ret;
    __fdmap[fd]->type = is_dir ? VITA_DESCRIPTOR_DIRECTORY : VITA_DESCRIPTOR_FILE;
    __fdmap[fd]->filename = strdup(full_path);
    __fdmap[fd]->flags = oflag;

    free(full_path);

    return fd;
}