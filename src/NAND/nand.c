/*
 * NAND/nand.c - the Revolution SDK NAND library's file calls: create/delete, read/write/seek, create-directory,
 *    move, status, open/close and their completions.
 * RANGE. .text 0x804C70E0-0x804C8770 (33 functions); .sdata 0x80793F08-0x80793F10 (the "/" and "/%s" literals of the
 *    move).  Cut from the NAND library 0x804C70E0-0x804C9F00: the build string "<< RVL_SDK - NAND ... (0x4302_145) >>"
 *    opens .data at 0x8061B9A0 and the OS one opens the next library at 0x8061BC20.  The seam with NAND/NANDCore.c at
 *    0x804C8770 is proven by the string literals: .sdata carries "/" twice (0x80793F08 read by the move, 0x80793F14 by
 *    nandGetRelativeName/nandConvertPath) and "/%s" twice (0x80793F0C, 0x80793F24), and under -str reuse one
 *    translation unit emits each literal once.  The open/close run 0x804C8260-0x804C8770 reads no data, so no seam
 *    inside this range is evidenced.
 * FLAGS. cflags_base in the `OS` lib block of configure.py (Wii/1.3); `nandOpen` is under `dont_inline` (the target
 *    calls it from the four open wrappers; the compiler inlines it otherwise); no float code.
 * NAMES. the `NAND*`/`nand*` names are the map's (dump).  GUESS: `nandCloseCallback` (the close completion), renamed
 *    from its map stem; the ISFS prototypes are in FS/fs.h and the NAND/NANDCore.c helpers in NAND/NANDCore.h.
 * RESIDUALS. written (15 functions, 14 at 100 %): the read/write/seek/close calls, `nandOpen` and its four wrappers
 *    and callback, and `nandComposePerm`.  `NANDSeekAsync` (90 %) schedules the descriptor load after the switch.  Not attempted
 *    (18 functions): create/delete, create-directory, move, status and the usage query; the .sdata literals are
 *    emitted only as far as the written bodies use them.
 * SHAPES. none yet.
 */

#include "types.h"
#include "FS/fs.h"
#include "MSL_C/string.h"
#include "MSL_C/strstr.h"
#include "NAND/nand.h"
#include "NAND/NANDCore.h"

#define NAND_FILE_OPEN 1
#define NAND_FILE_CLOSED 2
#define NAND_SEEK_START 0
#define NAND_SEEK_CURRENT 1
#define NAND_SEEK_END 2

void nandCloseCallback(s32 result, NANDCommandBlock* block);
void nandOpenCallback(s32 result, NANDCommandBlock* block);

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
