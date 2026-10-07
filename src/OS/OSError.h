/*
 * OS/OSError.h - declarations of the symbols owned by `OS/OSError.c` that other units call or read.
 */
#ifndef OS_OSERROR_H
#define OS_OSERROR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void OSPanic(const char* file, int line, const char* msg, ...);

void OSReport(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
