/*
 * NAND/NANDLogging.h - declarations of the symbols owned by `NAND/NANDLogging.c`: the NAND error log.
 */
#ifndef NAND_NANDLOGGING_H
#define NAND_NANDLOGGING_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The completion `NANDLoggingAddMessageAsync` reports to (GUESS: the SDK's `BOOL` result). */
typedef void (*NANDLoggingCallback)(BOOL result);

/* 0x804C9970 - appends the formatted message to the log file asynchronously; FALSE when a write is in flight. */
BOOL NANDLoggingAddMessageAsync(NANDLoggingCallback callback, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
