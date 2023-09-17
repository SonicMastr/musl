#ifndef VITAERROR_H
#define VITAERROR_H

#include <features.h>

#define SCE_ERRNO_MASK 0xFF
#define SCE_ERRNO_NONE 0x80010000
typedef enum
{
	ERROR_GENERIC,
	ERROR_SOCKET,
	ERROR_FIOS
} ErrorType;

hidden int __scefios2_errno_to_errno(int sce_errno);
hidden int __scenet_errno_to_errno(int sce_errno);
hidden int __sce_errno_to_errno(int sce_errno, int type);
hidden int __make_sce_errno(int posix_errno);

#endif