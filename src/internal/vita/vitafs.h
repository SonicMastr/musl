#ifndef VITAFS_H
#define VITAFS_H

#include "features.h"
#include <sys/syslimits.h>

hidden extern char __cwd[PATH_MAX];

hidden int __get_drive(const char *path);
hidden void __init_cwd(void);
hidden char *__resolve_path(const char *path, char *resolved);
hidden char *__realpath(const char *path);
hidden int __is_dir(const char *path);
hidden int _fcntl2sony(int flags);

#endif