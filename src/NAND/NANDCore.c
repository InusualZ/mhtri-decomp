/*
 * NAND/NANDCore.c - the Revolution SDK NAND library's core: path tokenising and conversion, the private-path tests,
 *    library bring-up and shutdown, result conversion, the home directory and the node-type probe.
 * RANGE. .text 0x804C8770-0x804C9480 (18 functions); .rodata 0x80573A10-0x80573B58 (the error-code table);
 *    .data 0x8061B9A0-0x8061BAF0; .bss 0x8074CF40-0x8074CF80 (`s_homeDir`); .sdata 0x80793F10-0x80793F30;
 *    .sbss 0x807952B8-0x807952C0 (`s_libState` and the 8-alignment hole after it).  Cut from the NAND library
 *    0x804C70E0-0x804C9F00.  Seam with NAND/nand.c at 0x804C8770: the duplicate "/" and "/%s" .sdata literals (one
 *    translation unit emits each once under -str reuse).  Seam with NAND/NANDCheck.c at 0x804C9480: this unit inlines
 *    `nandIsInitialized` (`NANDGetHomeDir`, `NANDPrivateGetTypeAsync`) while `NANDGetUsage` and `NANDCheckAsync` call it
 *    out of line.  Seam with NAND/NANDLogging.c in .sbss: 0x807952BC is a 4-byte hole no code references, the
 *    padding before an 8-aligned translation unit start.  The local statics and the @2779 string anchor
 *    (`nandConvertErrorCode`, `NANDInit`) make .data 0x8061B9A0-0x8061BAF0 one unit.
 * FLAGS. cflags_base in the `OS` lib block of configure.py (Wii/1.3); no pragma; no float code.
 * NAMES. the `NAND*`/`nand*` names are the map's (dump).  GUESS: the unit name (SDK spelling); `nandAsyncCallback`
 *    (the shared async completion), `nandSetDoneFlag` (the map said `EmissionControllerFinished`, a name from an
 *    unrelated library) and `nandLibReady`.
 * RESIDUALS. written (8 functions, 7 at 100 %): the private-path tests, `nandIsInitialized`, `nandSetDoneFlag`,
 *    `NANDGetHomeDir`, `nandAsyncCallback` and `nandGetHomeDir`.  `NANDGetHomeDir` (75 %) keeps the inlined readiness
 *    test as a 0/1 variable the source shape does not reproduce.  Not attempted (10 functions): path tokenising and
 *    conversion, result conversion (a 41-entry table copied to the stack), `nandGenerateAbsPath`, `NANDInit` and its
 *    shutdown function, and the node-type probe; the .data, .rodata and .sdata objects are not emitted.
 * SHAPES. `nandLibReady` is the inline form of `nandIsInitialized` the later callers use.
 */

#include "types.h"
#include "MSL_C/string.h"
#include "MSL_C/strstr.h"
#include "NAND/NANDCore.h"

#define NAND_LIB_STATE_READY 2

static char s_homeDir[64];
static s32 s_libState;

/* 0x804C8B10 (0x34): whether `path` is the private directory itself. */
BOOL nandIsPrivatePath(const char* path)
{
    return strncmp(path, "/shared2", 8) == 0;
}

/* 0x804C8B50 (0x58): whether `path` names something below the private directory. */
BOOL nandIsUnderPrivatePath(const char* path)
{
    if (strncmp(path, "/shared2/", 9) == 0 && path[9] != 0) {
        return TRUE;
    }
    return FALSE;
}

/* 0x804C8BB0 (0x14): whether the library has been brought up. */
BOOL nandIsInitialized(void)
{
    return s_libState == NAND_LIB_STATE_READY;
}

/* Whether the library has been brought up, as the later callers inline it. */
static inline BOOL nandLibReady(void)
{
    if (s_libState == NAND_LIB_STATE_READY) {
        return TRUE;
    }
    return FALSE;
}

/* 0x804C9010 (0xC): the ISFS completion of a synchronous wait: sets the caller's s32 done flag to 1. */
void nandSetDoneFlag(s32 result, NANDCommandBlock* block)
{
    *(s32*)block = 1;
}

/* 0x804C9020 (0x54): copies the title's home directory path into `path`. */
s32 NANDGetHomeDir(char* path)
{
    if (!nandLibReady()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    strcpy(path, s_homeDir);
    return 0;
}

/* 0x804C9080 (0x3C): finishes an asynchronous call: converts the ISFS result and calls the caller's callback. */
void nandAsyncCallback(s32 result, NANDCommandBlock* block)
{
    block->callback(nandConvertErrorCode(result), block);
}

/* 0x804C9470 (0xC): returns the title's home directory path. */
char* nandGetHomeDir(void)
{
    return s_homeDir;
}
