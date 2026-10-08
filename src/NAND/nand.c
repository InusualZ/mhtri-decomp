/*
 * NAND/nand.c - the Revolution SDK NAND library: create/delete/read/write/seek/open/close/status, path conversion,
 *    home directory, check and the error log.
 * RANGE. .text 0x804C70E0-0x804C9F00 (58 functions); .rodata 0x80573A10-0x80573B58; .data 0x8061B9A0-0x8061BC20; .bss
 *    0x8074CF40-0x8074D280; .sdata 0x80793F08-0x80793F50; .sbss 0x807952B8-0x807952C8.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the build string "<< RVL_SDK - NAND ... (0x4302_145) >>" opens .data at
 *    0x8061B9A0 and the OS one opens the next library at 0x8061BC20; every NAND function reads only .data
 *    0x8061B9A0..0x8061BC20, .bss 0x8074CF40.., .sdata 0x80793F08..0x80793F50 and .sbss 0x807952B8..0x807952C8;
 *    `__OSFPRInit` (0x804C9F00) is the first function that reads OS-owned data (.sbss `ZeroPS`/`ZeroF`).
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; `nandOpen` is under `dont_inline` (the target calls it
 *    from the four open wrappers; the compiler inlines it otherwise).
 * NAMES. the `NAND*`/`nand*` names are the map's (dump); the SDK splits the library over several source files (nand,
 *    NANDOpenClose, NANDCore, NANDCheck, NANDLogging) but the image carries no `__FILE__` string for it and its
 *    `.data` strings are ordered by first use across the whole run, so no seam inside is proven and the library
 *    is one unit.  GUESS: `nandAsyncCallback` (the shared async completion: converts the result and calls the caller's
 *    callback), `nandCloseCallback` (the close completion) and `nandSetDoneFlag` (the map said
 *    `EmissionControllerFinished`, a name from an unrelated library; the body stores 1 into the first word of the command
 *    block), renamed from their map stems; `s_fileSlot` and `nandLibReady` are GUESSes; the ISFS prototypes are in FS/fs.h.
 * RESIDUALS. written (21 at 100 %): the read/write/seek/close calls, `nandOpen` and its four wrappers and callback,
 *    `nandComposePerm`, the path predicates, the home-directory getters and `reserveFileDescriptor`.  `NANDSeekAsync`
 *    (90 %) schedules the descriptor load after the switch; `NANDGetHomeDir` (75 %) keeps the inlined readiness test as a
 *    0/1 variable the source shape does not reproduce.  Not attempted (37 functions): create/delete/status/move, path
 *    conversion, error conversion (a 41-entry table copied to the stack), init, usage, check and logging; the .data,
 *    .rodata, .sdata and .bss objects are emitted only as far as the written bodies use them.
 * SHAPES. none yet.
 */

#include "types.h"
#include "FS/fs.h"
#include "MSL_C/string.h"
#include "MSL_C/strstr.h"
#include "NAND/nand.h"

#define NAND_RESULT_NOT_INITIALIZED (-128)
#define NAND_RESULT_INVALID_STATE (-8)
#define NAND_LIB_STATE_READY 2
#define NAND_FILE_OPEN 1
#define NAND_FILE_CLOSED 2
#define NAND_FILE_SLOT_FREE (-255)
#define NAND_FILE_SLOT_TAKEN (-254)
#define NAND_SEEK_START 0
#define NAND_SEEK_CURRENT 1
#define NAND_SEEK_END 2

s32 nandConvertErrorCode(s32 result);
BOOL nandIsInitialized(void);
void nandAsyncCallback(s32 result, NANDCommandBlock* block);
void nandCloseCallback(s32 result, NANDCommandBlock* block);
void nandOpenCallback(s32 result, NANDCommandBlock* block);
void nandGenerateAbsPath(char* absPath, const char* path);
BOOL nandIsPrivatePath(const char* path);

static char s_homeDir[64];
static s32 s_libState;
static s32 s_fileSlot[2] = { NAND_FILE_SLOT_FREE, 0 };

/* 0x804C76E0 (0x68): reads `length` bytes of the open file into `buffer`. */
/* untyped: byte range */
s32 NANDRead(NANDFileInfo* info, void* buffer, u32 length)
{
    if (nandIsInitialized()) {
        return nandConvertErrorCode(ISFS_Read(info->fileDescriptor, buffer, length));
    }
    return NAND_RESULT_NOT_INITIALIZED;
}

/* 0x804C7750 (0x78): starts reading `length` bytes of the open file into `buffer`. */
/* untyped: byte range */
s32 NANDReadAsync(NANDFileInfo* info, void* buffer, u32 length, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    return nandConvertErrorCode(ISFS_ReadAsync(info->fileDescriptor, buffer, length, nandAsyncCallback, block));
}

/* 0x804C77D0 (0x68): writes `length` bytes of `buffer` to the open file. */
/* untyped: byte range */
s32 NANDWrite(NANDFileInfo* info, const void* buffer, u32 length)
{
    if (nandIsInitialized()) {
        return nandConvertErrorCode(ISFS_Write(info->fileDescriptor, buffer, length));
    }
    return NAND_RESULT_NOT_INITIALIZED;
}

/* 0x804C7840 (0x78): starts writing `length` bytes of `buffer` to the open file. */
/* untyped: byte range */
s32 NANDWriteAsync(NANDFileInfo* info, const void* buffer, u32 length, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    return nandConvertErrorCode(ISFS_WriteAsync(info->fileDescriptor, buffer, length, nandAsyncCallback, block));
}

/* 0x804C78C0 (0xA8): starts moving the position of the open file. */
s32 NANDSeekAsync(NANDFileInfo* info, s32 offset, s32 whence, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    s32 isfsWhence;

    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    isfsWhence = -1;
    switch (whence) {
    case NAND_SEEK_START:
        isfsWhence = 0;
        break;
    case NAND_SEEK_CURRENT:
        isfsWhence = 1;
        break;
    case NAND_SEEK_END:
        isfsWhence = 2;
        break;
    }
    return nandConvertErrorCode(ISFS_SeekAsync(info->fileDescriptor, offset, isfsWhence, nandAsyncCallback, block));
}

/* 0x804C7DC0 (0x54): packs the owner, group and other access bits into one permission byte. */
void nandComposePerm(u8* perm, u32 owner, u32 group, u32 other)
{
    u32 bits = 0;

    if (owner & 1) {
        bits |= 0x10;
    }
    if (owner & 2) {
        bits |= 0x20;
    }
    if (group & 1) {
        bits |= 4;
    }
    if (group & 2) {
        bits |= 8;
    }
    if (other & 1) {
        bits |= 1;
    }
    if (other & 2) {
        bits |= 2;
    }
    *perm = bits;
}

#pragma dont_inline on

/* 0x804C8260 (0x110): opens `path` through ISFS (asynchronously when `async`); a private path needs `privileged`. */
s32 nandOpen(const char* path, u8 mode, NANDCommandBlock* block, BOOL async, BOOL privileged)
{
    char absPath[64] = { 0 };
    s32 isfsMode = 0;

    nandGenerateAbsPath(absPath, path);
    if (!privileged && nandIsPrivatePath(absPath)) {
        return -102;
    }
    switch (mode) {
    case 3:
        isfsMode = 3;
        break;
    case 1:
        isfsMode = 1;
        break;
    case 2:
        isfsMode = 2;
        break;
    }
    if (async) {
        return ISFS_OpenAsync(absPath, isfsMode, nandOpenCallback, block);
    }
    return ISFS_Open(absPath, isfsMode);
}

#pragma dont_inline reset

/* 0x804C8370 (0x8C): opens `path` and fills the file record. */
s32 NANDOpen(const char* path, NANDFileInfo* info, u8 mode)
{
    s32 result;

    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    result = nandOpen(path, mode, NULL, FALSE, FALSE);
    if (result >= 0) {
        info->fileDescriptor = result;
        info->mark = NAND_FILE_OPEN;
        return 0;
    }
    return nandConvertErrorCode(result);
}

/* 0x804C8400 (0x8C): opens the private `path` and fills the file record. */
s32 NANDPrivateOpen(const char* path, NANDFileInfo* info, u8 mode)
{
    s32 result;

    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    result = nandOpen(path, mode, NULL, FALSE, TRUE);
    if (result >= 0) {
        info->fileDescriptor = result;
        info->mark = NAND_FILE_OPEN;
        return 0;
    }
    return nandConvertErrorCode(result);
}

/* 0x804C8490 (0x78): starts opening `path`. */
s32 NANDOpenAsync(const char* path, NANDFileInfo* info, u8 mode, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    block->info = info;
    return nandConvertErrorCode(nandOpen(path, mode, block, TRUE, FALSE));
}

/* 0x804C8510 (0x78): starts opening the private `path`. */
s32 NANDPrivateOpenAsync(const char* path, NANDFileInfo* info, u8 mode, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    block->info = info;
    return nandConvertErrorCode(nandOpen(path, mode, block, TRUE, TRUE));
}

/* 0x804C8590 (0x78): finishes an asynchronous open: fills the file record and calls the caller's callback. */
void nandOpenCallback(s32 result, NANDCommandBlock* block)
{
    if (result >= 0) {
        block->info->fileDescriptor = result;
        block->info->stage = 2;
        block->info->mark = NAND_FILE_OPEN;
        block->callback(0, block);
        return;
    }
    block->callback(nandConvertErrorCode(result), block);
}

/* 0x804C8610 (0x6C): closes the open file. */
s32 NANDClose(NANDFileInfo* info)
{
    s32 result;

    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    if (info->mark != NAND_FILE_OPEN) {
        return NAND_RESULT_INVALID_STATE;
    }
    result = ISFS_Close(info->fileDescriptor);
    if (result == 0) {
        info->mark = NAND_FILE_CLOSED;
    }
    return nandConvertErrorCode(result);
}

/* 0x804C8680 (0x88): starts closing the open file. */
s32 NANDCloseAsync(NANDFileInfo* info, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    if (info->mark != NAND_FILE_OPEN) {
        return NAND_RESULT_INVALID_STATE;
    }
    block->callback = callback;
    block->info = info;
    return nandConvertErrorCode(ISFS_CloseAsync(info->fileDescriptor, nandCloseCallback, block));
}

/* 0x804C8710 (0x5C): finishes an asynchronous close: marks the file record closed and calls the caller's callback. */
void nandCloseCallback(s32 result, NANDCommandBlock* block)
{
    if (result == 0) {
        block->info->stage = 7;
        block->info->mark = NAND_FILE_CLOSED;
    }
    block->callback(nandConvertErrorCode(result), block);
}

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

/* 0x804C9010 (0xC): marks a synchronous wait as done by setting the first word of the command block. */
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

/* 0x804C9910 (0x54): takes the file descriptor slot; FALSE when it is already taken. */
BOOL reserveFileDescriptor(void)
{
    BOOL level;
    BOOL taken;

    level = OSDisableInterrupts();
    if (s_fileSlot[0] == NAND_FILE_SLOT_FREE) {
        s_fileSlot[0] = NAND_FILE_SLOT_TAKEN;
        taken = FALSE;
    } else {
        taken = TRUE;
    }
    OSRestoreInterrupts(level);
    return taken == FALSE;
}
