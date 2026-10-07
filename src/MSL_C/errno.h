/*
 * MSL_C/errno.h - the C library's `errno` word, owned by `MSL_C/errno.c`.
 */
#ifndef MSL_C_ERRNO_H
#define MSL_C_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794E08 - the last error code the math, stdio and string-conversion routines stored. */
extern int errno;

#ifdef __cplusplus
}
#endif

#endif
