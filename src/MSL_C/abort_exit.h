/*
 * MSL_C/abort_exit.h - the abort path and the stdio exit hook, owned by `MSL_C/abort_exit.c`.
 */
#ifndef MSL_C_ABORT_EXIT_H
#define MSL_C_ABORT_EXIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794E1C - the hook `exit` runs to close the stdio streams; set by `__stdio_atexit`. */
extern void (*__stdio_exit)(void);

/* 0x80463D98 (0x34): raises SIGABRT, marks the program as aborting and exits with status 1. */
void abort(void);

/* 0x80463DCC (0x18): calls the registered runtime-constraint handler, if any. */
/* untyped: caller-owned pointer */
void __msl_runtime_constraint_violation_s(const char* message, void* pointer, s32 error);

#ifdef __cplusplus
}
#endif

#endif
