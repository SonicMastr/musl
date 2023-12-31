#include <features.h>
#include "libc.h"

#define START "_start"

#include "crt_arch.h"

#define ARGC_MAX (19)

int main();
weak void _init();
weak void _fini();
_Noreturn int __libc_start_main(int (*)(), int, char **,
        void (*)(), void(*)(), void(*)());

static inline int _strlen(const char *str)
{
    const char *beg = str;
    while (*str != 0) str++;
    return str - beg;
}

void _start_vita_c(unsigned int args, char *argp)
{
    char *argv[ARGC_MAX+1] = {""}; // Program name
    int argc = 1;
    int loc = 0;
    char *ptr = argp;

    /* Turn our thread arguments into main()'s argc and argv[]. */
    while (loc < args)
    {
        argv[argc++] = &ptr[loc];
        loc += _strlen(&ptr[loc])+1;

        if (argc == ARGC_MAX)
            break;
    }

    argv[argc] = 0;
    __libc_start_main(main, argc, argv, _init, _fini, 0); // DO VITA SPECIFIC STUFF IN HERE FFS
}