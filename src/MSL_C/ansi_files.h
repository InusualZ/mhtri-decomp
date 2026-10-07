/*
 * MSL_C/ansi_files.h - the stdio stream record and the stream table, owned by `MSL_C/ansi_files.c`.
 */
#ifndef MSL_C_ANSI_FILES_H
#define MSL_C_ANSI_FILES_H

#include "types.h"

/* How a stream was opened and what it is attached to. */
typedef struct FileModes {
    /* +0x0 */ unsigned int open_mode : 2;         /* 0 existing, 1 create, 2 truncate */
    /* +0x0 */ unsigned int io_mode : 3;          /* bit 0 read, bit 1 write, bit 2 append */
    /* +0x0 */ unsigned int buffer_mode : 2;      /* 0 unbuffered, 1 line buffered, 2 fully buffered */
    /* +0x0 */ unsigned int file_kind : 3;        /* 0 closed, 1 disk file, 2 console */
    /* +0x0 */ unsigned int file_orientation : 2; /* 0 unoriented, 1 byte, 2 wide */
    /* +0x0 */ unsigned int binary_io : 1;        /* no newline translation */
} FileModes; /* size: 0x4 */

/* The read/write position bookkeeping of a stream. */
typedef struct FileState {
    /* +0x0 */ unsigned int io_state : 3;          /* 0 idle, 1 writing, 2 reading, 3+ reading with pushed-back characters */
    /* +0x0 */ unsigned int free_buffer : 1;      /* the buffer was allocated by the library */
    /* +0x1 */ u8 eof;
    /* +0x2 */ u8 error;
} FileState; /* size: 0x4 */

typedef s32 (*FilePositionProc)(s32 handle, u32* position, s32 mode, void (*idle_proc)(void));
typedef s32 (*FileIOProc)(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void));
typedef s32 (*FileCloseProc)(s32 handle);

/* One stdio stream. */
typedef struct FILE {
    /* +0x00 */ s32 handle;
    /* +0x04 */ FileModes mode;
    /* +0x08 */ FileState state;
    /* +0x0C */ u8 is_dynamically_allocated;      /* the record came from the allocator (fopen), not __files */
    /* +0x0D */ u8 pad_0x0D[11];                  /* character push-back buffers */
    /* +0x18 */ u32 position;                      /* file position of the start of the buffer window */
    /* +0x1C */ u8* buffer;
    /* +0x20 */ u32 buffer_size;
    /* +0x24 */ u8* buffer_ptr;                    /* next byte to read or write inside the buffer */
    /* +0x28 */ u32 buffer_len;                    /* bytes left in the buffer window */
    /* +0x2C */ u32 buffer_alignment;              /* mask of the position bits the buffer start must preserve */
    /* +0x30 */ u32 save_buffer_len;
    /* +0x34 */ u32 buffer_pos;                    /* file position of buffer[0] */
    /* +0x38 */ FilePositionProc position_proc;
    /* +0x3C */ FileIOProc read_proc;
    /* +0x40 */ FileIOProc write_proc;
    /* +0x44 */ FileCloseProc close_proc;
    /* +0x48 */ void (*idle_proc)(void);
    /* +0x4C */ struct FILE* next_file;            /* next record of the list that starts at __files */
} FILE; /* size: 0x50 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8060E958 - stdin, stdout, stderr and a spare record. */
extern FILE __files[4];

/* 0x804591A8 (0xA4): closes every open stream and releases the records that were allocated. */
void __close_all(void);

/* 0x8045924C (0x6C): flushes every open stream; returns 0 or -1 when one failed. */
s32 __flush_all(void);

#ifdef __cplusplus
}
#endif

#endif
