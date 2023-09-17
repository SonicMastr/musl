#ifndef VITADESCRIPTOR_H
#define VITADESCRIPTOR_H

#include <stddef.h>

#include <features.h>

#define MAX_OPEN_FILES 256

typedef enum
{
    VITA_DESCRIPTOR_FILE,
    VITA_DESCRIPTOR_SOCKET,
    VITA_DESCRIPTOR_PIPE,
    VITA_DESCRIPTOR_TTY,
    VITA_DESCRIPTOR_DIRECTORY
} DescriptorTypes;

typedef struct
{
    int sce_uid;
    DescriptorTypes type;
    int ref_count;
    char* filename;
    int flags;
} DescriptorTranslation;

hidden extern DescriptorTranslation *__fdmap[];
hidden extern DescriptorTranslation __fdmap_pool[];

hidden void _init_fd(void);
hidden void _free_fd(void);

hidden int __acquire_descriptor(void);
hidden int __release_descriptor(int fd);
hidden int __duplicate_descriptor(int fd);
hidden int __descriptor_ref_count(int fd);
hidden DescriptorTranslation *__fd_grab(int fd);
hidden int __fd_drop(DescriptorTranslation *fdmap);

static inline int is_fd_valid(int fd) {
	return (fd >= 0) && (fd < MAX_OPEN_FILES) && (__fdmap[fd] != NULL);
}

#endif