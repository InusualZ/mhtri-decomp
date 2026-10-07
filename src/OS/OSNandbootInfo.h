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
/* size: 0x101C - the record after the checksum, as the checksum's word run and as the fields the launcher fills. */
typedef union OSNandbootBody {
    /* +0x00 */ u32 payload[0x407];
    struct {
        /* +0x00 */ u32 argsOffset; /* offset of the packed argument block in the argument page, zero for none */
        /* +0x04 */ u8 pad_0x04[2];
        /* +0x06 */ u8 appType;
        /* +0x07 */ u8 launchKind; /* 1 relaunch, 2 launch, 4 launch from the disc channel */
        /* +0x08 */ u32 launchCode;
        /* +0x0C */ u8 pad_0x0C[8];
        /* +0x14 */ u32 titleIdHi;
        /* +0x18 */ u32 titleIdLo;
        /* +0x1C */ u8 pad_0x1C[0xFFC];
    } fields;
} OSNandbootBody;

typedef struct OSNandbootInfo {
    /* +0x00 */ u32 checksum;
    /* +0x04 */ OSNandbootBody body;
} OSNandbootInfo; /* size: 0x1020 */

/* 0x804D6800 - makes sure `/shared2/sys/NANDBOOTINFO` exists with permission 0x3F; returns TRUE when it does. */
BOOL __OSCreateNandbootInfo(void);

/* 0x804D68B0 - stores the checksum into `info` and writes the record to the file; returns TRUE on success. */
BOOL __OSWriteNandbootInfo(OSNandbootInfo* info);

#ifdef __cplusplus
}
#endif

#endif
