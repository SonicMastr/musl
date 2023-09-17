#ifndef SYSCALL_INTERFACE_H
#define SYSCALL_INTERFACE_H

#include <features.h>

hidden const char *__lookup_syscall_name(int n);

#endif