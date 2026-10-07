/*
 * OS/OSNandbootInfo.h - the NAND boot info record and the entry points `OS/OSNandbootInfo.c` owns.
 */
#ifndef OS_OSNANDBOOTINFO_H
#define OS_OSNANDBOOTINFO_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x1020 - the file `/shared2/sys/NANDBOOTINFO`: a word sum over the rest of the record, then the payload the
 * boot code of the next title reads (its fields are not touched by the OS core, so it stays one word run). */
typedef struct OSNandbootInfo {
    /* +0x00 */ u32 checksum;
    /* +0x04 */ u32 payload[0x407];
} OSNandbootInfo; /* size: 0x1020 */

/* 0x804D6800 - makes sure `/shared2/sys/NANDBOOTINFO` exists with permission 0x3F; returns TRUE when it does. */
BOOL __OSCreateNandbootInfo(void);

/* 0x804D68B0 - stores the checksum into `info` and writes the record to the file; returns TRUE on success. */
BOOL __OSWriteNandbootInfo(OSNandbootInfo* info);

#ifdef __cplusplus
}
#endif

#endif
