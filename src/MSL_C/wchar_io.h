/*
 * MSL_C/wchar_io.h - the stream orientation entry point, owned by `MSL_C/wchar_io.c`.
 */
#ifndef MSL_C_WCHAR_IO_H
#define MSL_C_WCHAR_IO_H

#include "types.h"
#include "MSL_C/ansi_files.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80463C48 (0x78): queries (mode 0) or sets the byte/wide orientation of `file`; returns the orientation as -1, 0 or 1. */
s32 fwide(FILE* file, s32 mode);

#ifdef __cplusplus
}
#endif

#endif
