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
 * FLAGS. cflags_base in the `OS` lib block of configure.py (Wii/1.3); no float code.  `#pragma dont_inline` covers
 *    the run from `nandMove` to `nandGetStatus` (`nandMove`, `nandComposePerm`, `nandSplitPerm`, `nandGetStatus`) and
 *    `nandOpen` (the target calls each out of line from the callers that follow it; the compiler inlines them
 *    otherwise).
 * NAMES. the `NAND*`/`nand*` names are the map's (dump).  GUESS: `nandCloseCallback` (the close completion);
 *    `NANDCreateAsync`, `NANDCreateDirAsync` (the public twins that refuse the private directory), `nandMove`
 *    (`ISFS_Rename`/`ISFS_RenameAsync`), `NANDMove` (its asynchronous public caller: it stores the callback and passes
 *    async=1; the SDK spelling is `NANDMoveAsync`), `nandSplitPerm` (permission byte to access words), `nandGetStatus`
 *    (`ISFS_GetAttr`/`ISFS_GetAttrAsync`), `nandGetStatusCallback` and `NANDGetHomeUsage` (`ISFS_GetUsage` on the home
 *    directory), in the SDK's scheme; the inline helpers `nandCreate`, `nandDelete`, `nandCreateDirAsync` and
 *    `nandSeekAsync` (no map row: every call is inlined).
 * RESIDUALS. `NANDMove` 0x804C7D40-0x804C7DC0 (0x80) is not written: NAND/nand.h declares it with two parameters
 *    while the target and its caller (`menu/menu_plsearch.cpp` 0x80448E64) pass four (path, destDir, callback,
 *    block); the body is `block->callback = callback; return nandConvertErrorCode(nandMove(path, destDir, block,
 *    TRUE, FALSE));` behind the readiness test.  Every other function matches.
 * SHAPES. the create/delete/create-directory/seek bodies are static inline helpers taking constant async/privileged
 *    flags, which is what puts the descriptor load of `NANDSeekAsync` before the switch and the callback store before
 *    the path clear.
 */

#include "types.h"
#include "FS/fs.h"
#include "MSL_C/alloc.h"
#include "NAND/nand.h"
#include "NAND/NANDCore.h"

#define NAND_FILE_OPEN 1
#define NAND_FILE_CLOSED 2
#define NAND_SEEK_START 0
#define NAND_SEEK_CURRENT 1
#define NAND_SEEK_END 2
#define NAND_PERM_OWNER_READ 0x10
#define ISFS_ERROR_ACCESS (-102)
#define ISFS_ERROR_INVALID (-101)

void nandCloseCallback(s32 result, NANDCommandBlock* block);
static void nandOpenCallback(s32 result, NANDCommandBlock* block);
void nandSplitPerm(u8 perm, u32* ownerAcc, u32* groupAcc, u32* othersAcc);
void nandGetStatusCallback(s32 result, NANDCommandBlock* block);

/* Creates the file `path` through ISFS (asynchronously when `async`); a private path needs `privileged`. */
static inline s32 nandCreate(const char* path, u8 perm, u8 attr, NANDCommandBlock* block, BOOL async, BOOL privileged)
{
    char absPath[64] = { 0 };
    u32 ownerAcc = 0;
    u32 groupAcc = 0;
    u32 othersAcc = 0;

    nandGenerateAbsPath(absPath, path);
    if (!privileged && nandIsPrivatePath(absPath)) {
        return ISFS_ERROR_ACCESS;
    }
    if (!(perm & NAND_PERM_OWNER_READ)) {
        return ISFS_ERROR_INVALID;
    }
    nandSplitPerm(perm, &ownerAcc, &groupAcc, &othersAcc);
    if (async) {
        return ISFS_CreateFileAsync(absPath, attr, ownerAcc, groupAcc, othersAcc, nandAsyncCallback, block);
    }
    return ISFS_CreateFile(absPath, attr, ownerAcc, groupAcc, othersAcc);
}

/* 0x804C70E0 (0xF0): creates the file `path` by its absolute name. */
s32 NANDPrivateCreate(const char* path, u8 permission, u8 attribute)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    return nandConvertErrorCode(nandCreate(path, permission, attribute, NULL, FALSE, TRUE));
}

/* 0x804C71D0 (0x118): starts creating the file `path`; the private directory is refused. */
s32 NANDCreateAsync(const char* path, u8 permission, u8 attribute, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    return nandConvertErrorCode(nandCreate(path, permission, attribute, block, TRUE, FALSE));
}

/* 0x804C72F0 (0x100): starts creating the file `path` by its absolute name. */
s32 NANDPrivateCreateAsync(const char* path, u8 permission, u8 attribute, NANDAsyncCallback callback,
                           NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    return nandConvertErrorCode(nandCreate(path, permission, attribute, block, TRUE, TRUE));
}

/* Deletes `path` through ISFS (asynchronously when `async`); a private path needs `privileged`. */
static inline s32 nandDelete(const char* path, NANDCommandBlock* block, BOOL async, BOOL privileged)
{
    char absPath[64] = { 0 };

    nandGenerateAbsPath(absPath, path);
    if (!privileged && nandIsPrivatePath(absPath)) {
        return ISFS_ERROR_ACCESS;
    }
    if (async) {
        return ISFS_DeleteAsync(absPath, nandAsyncCallback, block);
    }
    return ISFS_Delete(absPath);
}

/* 0x804C73F0 (0xB0): deletes `path`; the private directory is refused. */
s32 NANDDelete(const char* path)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    return nandConvertErrorCode(nandDelete(path, NULL, FALSE, FALSE));
}

/* 0x804C74A0 (0x98): deletes `path` by its absolute name. */
s32 NANDPrivateDelete(const char* path)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    return nandConvertErrorCode(nandDelete(path, NULL, FALSE, TRUE));
}

/* 0x804C7540 (0xD8): starts deleting `path`; the private directory is refused. */
s32 NANDDeleteAsync(const char* path, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    return nandConvertErrorCode(nandDelete(path, block, TRUE, FALSE));
}

/* 0x804C7620 (0xC0): starts deleting `path` by its absolute name. */
s32 NANDPrivateDeleteAsync(const char* path, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    return nandConvertErrorCode(nandDelete(path, block, TRUE, TRUE));
}

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

/* Starts moving the position of the open file `fd`. */
static inline s32 nandSeekAsync(s32 fd, s32 offset, s32 whence, NANDCommandBlock* block)
{
    s32 isfsWhence = -1;

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
    return ISFS_SeekAsync(fd, offset, isfsWhence, nandAsyncCallback, block);
}

/* 0x804C78C0 (0xA8): starts moving the position of the open file. */
s32 NANDSeekAsync(NANDFileInfo* info, s32 offset, s32 whence, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    return nandConvertErrorCode(nandSeekAsync(info->fileDescriptor, offset, whence, block));
}

/* Starts creating the directory `path` through ISFS; a private path needs `privileged`. */
static inline s32 nandCreateDirAsync(const char* path, u8 perm, u8 attr, NANDCommandBlock* block, BOOL privileged)
{
    char absPath[64] = { 0 };

    nandGenerateAbsPath(absPath, path);
    if (!privileged && nandIsPrivatePath(absPath)) {
        return ISFS_ERROR_ACCESS;
    }
    if (!(perm & NAND_PERM_OWNER_READ)) {
        return ISFS_ERROR_INVALID;
    }
    {
        u32 ownerAcc = 0;
        u32 groupAcc = 0;
        u32 othersAcc = 0;

        nandSplitPerm(perm, &ownerAcc, &groupAcc, &othersAcc);
        return ISFS_CreateDirAsync(absPath, attr, ownerAcc, groupAcc, othersAcc, nandAsyncCallback, block);
    }
}

/* 0x804C7970 (0x118): starts creating the directory `path`; the private directory is refused. */
s32 NANDCreateDirAsync(const char* path, u8 permission, u8 attribute, NANDAsyncCallback callback,
                       NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    return nandConvertErrorCode(nandCreateDirAsync(path, permission, attribute, block, FALSE));
}

/* 0x804C7A90 (0x100): starts creating the directory `path` by its absolute name. */
s32 NANDPrivateCreateDirAsync(const char* path, u8 permission, u8 attribute, NANDAsyncCallback callback,
                              NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    return nandConvertErrorCode(nandCreateDirAsync(path, permission, attribute, block, TRUE));
}

#pragma dont_inline on

/* 0x804C7B90 (0x1A8): moves `path` into the directory `destDir` through ISFS (asynchronously when `async`). */
s32 nandMove(const char* path, const char* destDir, NANDCommandBlock* block, BOOL async, BOOL privileged)
{
    char absOldPath[64] = { 0 };
    char absNewPath[64] = { 0 };
    char relativeName[13] = { 0 };

    nandGenerateAbsPath(absOldPath, path);
    nandGetRelativeName(relativeName, absOldPath);
    nandGenerateAbsPath(absNewPath, destDir);
    if (strcmp(absNewPath, "/") == 0) {
        sprintf(absNewPath, "/%s", relativeName);
    } else {
        strcat(absNewPath, "/");
        strcat(absNewPath, relativeName);
    }
    if (!privileged && (nandIsPrivatePath(absOldPath) || nandIsPrivatePath(absNewPath))) {
        return ISFS_ERROR_ACCESS;
    }
    if (async) {
        return ISFS_RenameAsync(absOldPath, absNewPath, nandAsyncCallback, block);
    }
    return ISFS_Rename(absOldPath, absNewPath);
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

/* 0x804C7E20 (0x8C): splits a permission byte into the owner, group and other access words. */
void nandSplitPerm(u8 perm, u32* ownerAcc, u32* groupAcc, u32* othersAcc)
{
    *ownerAcc = 0;
    *groupAcc = 0;
    *othersAcc = 0;
    if (perm & 0x10) {
        *ownerAcc |= 1;
    }
    if (perm & 0x20) {
        *ownerAcc |= 2;
    }
    if (perm & 4) {
        *groupAcc |= 1;
    }
    if (perm & 8) {
        *groupAcc |= 2;
    }
    if (perm & 1) {
        *othersAcc |= 1;
    }
    if (perm & 2) {
        *othersAcc |= 2;
    }
}

/* 0x804C7EB0 (0x154): reads the status record of `path` through ISFS (asynchronously when `async`); a path below the
 * private directory needs `privileged`. */
s32 nandGetStatus(const char* path, NANDStatus* status, NANDCommandBlock* block, BOOL async, BOOL privileged)
{
    char absPath[64] = { 0 };

    nandGenerateAbsPath(absPath, path);
    if (!privileged && nandIsUnderPrivatePath(absPath)) {
        return ISFS_ERROR_ACCESS;
    }
    if (async) {
        return ISFS_GetAttrAsync(absPath, &status->ownerId, &status->groupId, &block->attr, &block->ownerAcc,
                                 &block->groupAcc, &block->othersAcc, nandGetStatusCallback, block);
    } else {
        u32 attr = 0;
        u32 ownerAcc = 0;
        u32 groupAcc = 0;
        u32 othersAcc = 0;
        s32 result = ISFS_GetAttr(absPath, &status->ownerId, &status->groupId, &attr, &ownerAcc, &groupAcc,
                                  &othersAcc);

        if (result == 0) {
            nandComposePerm(&status->permission, ownerAcc, groupAcc, othersAcc);
            status->attribute = attr;
        }
        return result;
    }
}

#pragma dont_inline reset

/* 0x804C8010 (0x74): finishes an asynchronous status read: fills the record and calls the caller's callback. */
void nandGetStatusCallback(s32 result, NANDCommandBlock* block)
{
    if (result == 0) {
        NANDStatus* status = block->status;

        status->attribute = block->attr;
        nandComposePerm(&status->permission, block->ownerAcc, block->groupAcc, block->othersAcc);
    }
    block->callback(nandConvertErrorCode(result), block);
}

/* 0x804C8090 (0x64): reads the status record of `path`. */
s32 NANDGetStatus(const char* path, NANDStatus* status)
{
    if (nandIsInitialized()) {
        return nandConvertErrorCode(nandGetStatus(path, status, NULL, FALSE, FALSE));
    }
    return NAND_RESULT_NOT_INITIALIZED;
}

/* 0x804C8100 (0x64): reads the status record of `path` by its absolute name. */
s32 NANDPrivateGetStatus(const char* path, NANDStatus* status)
{
    if (nandIsInitialized()) {
        return nandConvertErrorCode(nandGetStatus(path, status, NULL, FALSE, TRUE));
    }
    return NAND_RESULT_NOT_INITIALIZED;
}

/* 0x804C8170 (0x84): starts reading the status record of `path` by its absolute name. */
s32 NANDPrivateGetStatusAsync(const char* path, NANDStatus* status, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    block->status = status;
    return nandConvertErrorCode(nandGetStatus(path, status, block, TRUE, TRUE));
}

/* 0x804C8200 (0x5C): reads the blocks and inodes the title's home directory uses. */
s32 NANDGetHomeUsage(u32* blocks, u32* inodes)
{
    if (nandIsInitialized()) {
        return nandConvertErrorCode(ISFS_GetUsage(nandGetHomeDir(), blocks, inodes));
    }
    return NAND_RESULT_NOT_INITIALIZED;
}


#pragma dont_inline on

/* 0x804C8260 (0x110): opens `path` through ISFS (asynchronously when `async`); a private path needs `privileged`. */
static s32 nandOpen(const char* path, u8 mode, NANDCommandBlock* block, BOOL async, BOOL privileged)
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
static void nandOpenCallback(s32 result, NANDCommandBlock* block)
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
