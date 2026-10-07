/*
 * MSL_C/file_io.h - the stdio buffer and stream entry points, owned by `MSL_C/file_io.c`.
 */
#ifndef MSL_C_FILE_IO_H
#define MSL_C_FILE_IO_H

#include "types.h"
#include "MSL_C/ansi_files.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045AB38 (0x10): absolute value of an int. */
int abs(int value);

/* 0x8045AB48 (0x10): absolute value of a long. */
long labs(long value);

/* 0x8045AB58 (0x28): resets the buffer window of `file` to the start of the buffer. */
void __prep_buffer(FILE* file);

/* 0x8045AB80 (0xB8): writes the pending buffer bytes through the stream's write callback; returns 0 or the callback's error. */
s32 __flush_buffer(FILE* file, u32* bytes_flushed);

/* 0x8045AC38 (0x308): writes `count` items of `size` bytes; returns the number of whole items written. */
/* untyped: byte range */
u32 __fwrite(const void* buffer, u32 size, u32 count, FILE* file);

/* 0x8045AF40 (0xBC): flushes and closes `file`; returns 0 or -1. */
s32 fclose(FILE* file);

/* 0x8045AFFC (0x134): flushes one stream (all streams for NULL); returns 0 or -1. */
s32 fflush(FILE* file);

/* 0x8045B130 (0xA8): returns the stream position, or -1 and sets errno. */
s32 _ftell(FILE* file);

/* 0x8045B1D8 (0x4): returns the stream position. */
s32 ftell(FILE* file);

/* 0x8045B1DC (0x1C4): moves the stream position; returns 0 or -1. */
s32 _fseek(FILE* file, u32 offset, s32 mode);

#ifdef __cplusplus
}
#endif

#endif
