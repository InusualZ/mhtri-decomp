/*
 * OS/OSStateFlags.h - the persistent state-flags record and the entry points `OS/OSStateFlags.c` owns.
 */
#ifndef OS_OSSTATEFLAGS_H
#define OS_OSSTATEFLAGS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x20 - the boot/reset state record kept in the NAND (only the first 8 bytes are touched by the OS core); `words` is the
 * word view the file checksum runs over. */
typedef union OSStateFlags {
    struct {
        /* +0x00 */ u32 checksum;
        /* +0x04 */ u8 flags;
        /* +0x05 */ u8 type;        /* the reset kind (1 normal, 3 return-to-menu, 5 idle shutdown) */
        /* +0x06 */ u8 discState;   /* 1 disc ready, 2 disc inserted, 3 cover open */
        /* +0x07 */ u8 returnToMenu;
        /* +0x08 */ u8 pad_0x08[0x18];
    };
    /* +0x00 */ u32 words[8];
} OSStateFlags; /* size: 0x20 */

/* 0x804D6520 / 0x804D6610 - write / read the state-flags record. */
BOOL __OSWriteStateFlags(OSStateFlags* flags);
BOOL __OSReadStateFlags(OSStateFlags* flags);

#ifdef __cplusplus
}
#endif

#endif
