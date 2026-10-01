/*
 * NAND/nand.c - the Revolution SDK NAND, OS and PAD run, `.text` 0x804C6D70..0x804D9B4C.
 *
 * WHAT IT IS. a merged guess block of phase 1 band `f`: the NAND library (nand, NANDOpenClose, NANDCore, NANDLogging), the OS
 * library files (OS, OSAlarm, OSAlloc, OSArena, OSAudioSystem, OSCache, OSContext, OSError, OSExec, OSFatal, OSFont,
 * OSInterrupt, OSMessage, OSMemory, OSMutex, OSReboot, OSReset, OSRtc, OSSync, OSThread, OSTime, OSUtf, OSIpc, OSStateTM,
 * OSPlayRecord, OSStateFlags, OSNet, OSNandbootInfo, OSPlayTime, OSLaunch), `ppc_eabi_init` and PAD, up to the RSO notify
 * thunks at 0x804D9B3C.  The phase 1 cut inside the OS files (`AlarmQueue` is a `scope:local` static read from
 * `__OSInitAlarm` 0x804CB464 to `__OSCancelInternalAlarms` 0x804CBC80) keeps `OSAlarm` 0x804CB440..0x804CBD10 one piece.
 *
 * WHY IT SITS HERE. phase 1 grade medium (inherited from the registered `OS/OSAlarm.c` edge); every cut inside is a guess
 * and the renderer merges the run.  The unit absorbed the registered `OS/OSAlarm.c` (one 16 B function, `cPhs_Set` at
 * 0x804CBC50, formerly `Object(Matching)`): the merged object is `NonMatching` because it now owns the whole run and its
 * data (.bss 0x1520, .data 0xE1F0, .rodata 0x158, .sbss 0x188, .sdata 0xF8, .sdata2 0x70) that this source does not emit.
 *
 * UNKNOWN. every other body and the file boundaries between the pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (cflags_base + `-func_align 4`); the former OSAlarm.c measured 100.0 % with them: -O4,p
 * reproduces the retail window byte-for-byte while -O3 leaves the `li r0,-1` after the first store.
 *
 * `cPhs_Set` stores userData at +0x28 and 0xFFFFFFFF at +0x04 (the tag): 4 instructions, no frame, no relocation.  The name is
 * the map's; the neighbourhood is the OSAlarm cluster and the helper looks like a file-static of it.
 */

typedef struct OSAlarm_s {
    void *handler;         /* +0x00 */
    unsigned int tag;      /* +0x04 */
    unsigned char pad[0x20];
    unsigned int userData; /* +0x28 */
} OSAlarm;

void cPhs_Set(OSAlarm *alarm, unsigned int userData)
{
    alarm->userData = userData;
    alarm->tag = -1;
}
