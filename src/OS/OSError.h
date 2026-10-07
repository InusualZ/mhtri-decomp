/*
 * OS/OSError.h - declarations of the symbols owned by `OS/OSError.c` that other units call or read.
 */
#ifndef OS_OSERROR_H
#define OS_OSERROR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OSContext;

/* The handler run for a processor error: the error number, the interrupted context and the error's extra arguments. */
typedef void (*OSErrorHandler)(u8 error, struct OSContext* context, ...);

/* 0x804CD7E0 - installs the handler for processor error `error` and returns the previous one. */
OSErrorHandler OSSetErrorHandler(u8 error, OSErrorHandler handler);

void OSPanic(const char* file, int line, const char* msg, ...);

void OSReport(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
