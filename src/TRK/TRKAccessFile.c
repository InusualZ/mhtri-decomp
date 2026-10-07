/*
 * TRK/TRKAccessFile.c - the MetroTRK host-file stubs: each is a bare trap followed by a return.
 *
 * RANGE. .text 0x80468358..0x80468378 (4 functions in the map, 0x20 B).
 * FLAGS. the `OS` lib's `cflags_os`; the bodies are `asm` (the trap word is not reachable from C).
 * NAMES. `TRKAccessFile` is the dump's; `TRKOpenFile`, `TRKCloseFile` are GUESS (neighbour scheme; the dump labels
 *    all three siblings `TRKPositionFile`, kept for the last), `TRKPositionFile` is the dump's.
 * EVIDENCE. the dump names `TRKAccessFile`; `TRK/mslsupp.c` calls it, the three siblings have no caller.
 * RESIDUALS. COARSE: the three siblings have no named caller; signatures are `void`.
 * SHAPES. `nofralloc` `twui r0,0` + `blr`.
 */
asm void TRKAccessFile(void)
{
    nofralloc
    twi 31, r0, 0
    blr
}

__declspec(export) asm void TRKOpenFile(void)
{
    nofralloc
    twi 31, r0, 0
    blr
}

__declspec(export) asm void TRKCloseFile(void)
{
    nofralloc
    twi 31, r0, 0
    blr
}

__declspec(export) asm void TRKPositionFile(void)
{
    nofralloc
    twi 31, r0, 0
    blr
}
