/*
 * NAND/NANDCore.c - the Revolution SDK NAND library's core: path tokenising and conversion, the private-path tests,
 *    library bring-up and shutdown, result conversion, the home directory and the node-type probe.
 * RANGE. .text 0x804C8770-0x804C9480 (18 functions); .rodata 0x80573A10-0x80573B58 (the error-code table);
 *    .data 0x8061B9A0-0x8061BAF0 (the version string, `s_currentDir`, `s_shutdownFuncInfo` and the strings);
 *    .bss 0x8074CF40-0x8074CF80 (`s_homeDir`); .sdata 0x80793F10-0x80793F30 (`__NANDVersion` and the path literals);
 *    .sbss 0x807952B8-0x807952C0 (`s_libState` and the 8-alignment hole after it).  Cut from the NAND library
 *    0x804C70E0-0x804C9F00.  Seam with NAND/nand.c at 0x804C8770: the duplicate "/" and "/%s" .sdata literals (one
 *    translation unit emits each once under -str reuse).  Seam with NAND/NANDCheck.c at 0x804C9480: this unit inlines
 *    `nandIsInitialized` (`NANDPrivateGetTypeAsync`) while `NANDGetUsage` and `NANDCheckAsync` call it out of line.
 *    Seam with NAND/NANDLogging.c in .sbss: 0x807952BC is a 4-byte hole no code references, the padding before an
 *    8-aligned translation unit start.  The local statics and the @2779 string anchor (`nandConvertErrorCode`,
 *    `NANDInit`) make .data 0x8061B9A0-0x8061BAF0 one unit.
 * FLAGS. cflags_base in the `OS` lib block of configure.py (Wii/1.3); no float code.  `#pragma dont_inline` covers
 *    `nandRemoveTailToken` and `nandGetHeadToken` (the target calls both out of line from `nandConvertPath`).
 *    `s_currentDir` and `s_homeDir` are 32-byte aligned (ISFS DMA buffers); the two message buffers of
 *    `nandConvertErrorCode` are 64-byte aligned (its frame is realigned with `clrlwi r11,r1,26`).
 * NAMES. the `NAND*`/`nand*` names are the map's (dump).  GUESS: the unit name (SDK spelling); `nandAsyncCallback`
 *    (the shared async completion), `nandOnShutdown` (the shutdown function `NANDInit` registers), `nandGetType` and
 *    `nandGetTypeCallback` (the node-type probe and its completion), `nandShutdownCallback` (the ISFS completion of the
 *    shutdown wait), `nandLibReady`, `nandIsRelativePath` and `s_shutdownFuncInfo` (the shutdown-function record
 *    `NANDInit` registers).
 * RESIDUALS. none: every function, and the .rodata/.data/.bss/.sdata/.sbss objects, match the target.
 * SHAPES. `nandLibReady` is the inline readiness test `NANDGetHomeDir` uses (a 0/1 value compared again);
 *    `nandIsRelativePath` and `nandIsUnderPrivatePath` inline into `nandGenerateAbsPath`/`nandGetType` the same way.
 *    `nandConvertErrorCode` keeps the range test `(void)(result < -23 && result > -100)` after each log: the target
 *    carries its two compares with no consumer.  `nandGetHeadToken` needs an `int` index (`s32` folds the next-byte
 *    load into `lbzu`, `u32` swaps the `add` operands).  `nandConvertPath` declares its four buffers in reverse frame
 *    order (token, relative rest, parent, joined path).
 */

#include "types.h"
#include "ESP/esp.h"
#include "FS/fs.h"
#include "MSL/strlen.h"
#include "MSL_C/alloc.h"
#include "MSL_C/strstr.h"
#include "OS/OS.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/OSReset.h"
#include "OS/OSTime.h"
#include "NAND/NANDCore.h"
#include "NAND/NANDLogging.h"

#define NAND_LIB_STATE_NONE 0
#define NAND_LIB_STATE_BUSY 1
#define NAND_LIB_STATE_READY 2
#define ISFS_ERROR_ACCESS (-102)
#define ISFS_ERROR_INVALID (-101)
#define NAND_RESULT_UNKNOWN (-64)
#define NAND_RESULT_BUSY (-3)
#define NAND_TYPE_FILE 1
#define NAND_TYPE_DIR 2
#define NAND_SHUTDOWN_TIMEOUT_MS 500

const char* __NANDVersion = "<< RVL_SDK - NAND \trelease build: Feb 27 2009 10:05:16 (0x4302_145) >>";

static char s_currentDir[64] __attribute__((aligned(32))) = "/";
static char s_homeDir[64] __attribute__((aligned(32)));
static s32 s_libState;

static BOOL nandOnShutdown(BOOL final, u32 event);
static OSShutdownFunctionInfo s_shutdownFuncInfo = { nandOnShutdown, 255 };

void nandShutdownCallback(s32 result, NANDCommandBlock* block);
void nandGetTypeCallback(s32 result, NANDCommandBlock* block);

#pragma dont_inline on

/* 0x804C8770 (0xD4): copies `oldPath` without its last component into `newPath` ("/" stays "/"). */
void nandRemoveTailToken(char* newPath, const char* oldPath)
{
    if (oldPath[0] == '/' && oldPath[1] == '\0') {
        newPath[0] = '/';
        newPath[1] = '\0';
    } else {
        s32 i;

        for (i = strlen(oldPath) - 1; i >= 0; i--) {
            if (oldPath[i] == '/') {
                if (i != 0) {
                    strncpy(newPath, oldPath, i);
                    newPath[i] = '\0';
                } else {
                    newPath[0] = '/';
                    newPath[1] = '\0';
                }
                break;
            }
        }
    }
}

/* 0x804C8850 (0xD4): splits the first component of `oldPath` into `token` and the rest into `newPath`. */
void nandGetHeadToken(char* token, char* newPath, const char* oldPath)
{
    int i;

    for (i = 0; i <= strlen(oldPath); i++) {
        if (oldPath[i] == '/') {
            strncpy(token, oldPath, i);
            token[i] = '\0';
            if (oldPath[i + 1] == '\0') {
                newPath[0] = '\0';
            } else {
                strcpy(newPath, oldPath + i + 1);
            }
            break;
        } else if (oldPath[i] == '\0') {
            strncpy(token, oldPath, i);
            token[i] = '\0';
            newPath[0] = '\0';
            break;
        }
    }
}

#pragma dont_inline reset

/* 0x804C8930 (0x98): copies the last component of the absolute `path` into `name` (empty for the root). */
void nandGetRelativeName(char* name, const char* path)
{
    if (strcmp("/", path) == 0) {
        strcpy(name, "");
    } else {
        s32 i;

        for (i = strlen(path) - 1; i >= 0; i--) {
            if (path[i] == '/') {
                break;
            }
        }
        strcpy(name, path + i + 1);
    }
}

/* 0x804C89D0 (0x13C): resolves `relPath` ("." and ".." included) against the directory `dirPath` into `absPath`. */
void nandConvertPath(char* absPath, const char* dirPath, const char* relPath)
{
    char token[128];
    char newRelPath[128];
    char parentDir[128];
    char newDirPath[128];

    if (strlen(relPath) == 0) {
        strcpy(absPath, dirPath);
        return;
    }
    nandGetHeadToken(token, newRelPath, relPath);
    if (strcmp(token, ".") == 0) {
        nandConvertPath(absPath, dirPath, newRelPath);
    } else if (strcmp(token, "..") == 0) {
        nandRemoveTailToken(parentDir, dirPath);
        nandConvertPath(absPath, parentDir, newRelPath);
    } else if (token[0] != '\0') {
        if (strcmp(dirPath, "/") == 0) {
            sprintf(newDirPath, "/%s", token);
        } else {
            sprintf(newDirPath, "%s/%s", dirPath, token);
        }
        nandConvertPath(absPath, newDirPath, newRelPath);
    } else {
        strcpy(absPath, dirPath);
    }
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

/* 0x804C8BD0 (0x168): maps an ISFS result to the NAND result; logs the unexpected ones. */
s32 nandConvertErrorCode(s32 result)
{
    const s32 errorMap[] = {
        0,    0,    -102, -1,   -103, -4,   -114, -5,   -105, -6,   -116, -15,  -101, -8,   -108, -9,   -109,
        -10,  -107, -11,  -110, -16,  -106, -12,  -115, -13,  -104, -64,  -111, -14,  -117, -64,  -118, -3,
        -119, -128, -1,   -1,   -2,   -6,   -3,   -64,  -4,   -8,   -5,   -64,  -6,   -12,  -7,   -64,  -8,
        -3,   -9,   -64,  -10,  -64,  -11,  -64,  -12,  -5,   -13,  -64,  -14,  -64,  -15,  -64,  -16,  -64,
        -17,  -64,  -18,  -64,  -19,  -64,  -20,  -64,  -21,  -64,  -22,  -2,   -23,  -64,
    };
    s32 i = 0;

    if (result >= 0) {
        return result;
    }
    for (; i < sizeof(errorMap) / sizeof(errorMap[0]); i += 2) {
        if (result == errorMap[i]) {
            if (result == -114 || result == -116 || result == -117 || result == -9 || result == -12) {
                char message[128] __attribute__((aligned(64)));

                sprintf(message, "ISFS error code: %d", result);
                NANDLoggingAddMessageAsync(NULL, message);
            }
            (void)(result < -23 && result > -100);
            return errorMap[i + 1];
        }
    }
    OSReport("CAUTION!  Unexpected error code [%d] was found.\n", result);
    {
        char message[128] __attribute__((aligned(64)));

        sprintf(message, "ISFS unexpected error code: %d", result);
        NANDLoggingAddMessageAsync(NULL, message);
    }
    (void)(result < -23 && result > -100);
    return NAND_RESULT_UNKNOWN;
}

/* Whether `path` is relative (does not start at the root). */
static inline BOOL nandIsRelativePath(const char* path)
{
    return path[0] == '/' ? FALSE : TRUE;
}

/* 0x804C8D40 (0xC8): the absolute path of `path`; a relative path is resolved against the current directory. */
void nandGenerateAbsPath(char* absPath, const char* path)
{
    u32 length;

    if (strlen(path) == 0) {
        strcpy(absPath, "");
    } else if (nandIsRelativePath(path)) {
        nandConvertPath(absPath, s_currentDir, path);
    } else {
        strcpy(absPath, path);
        length = strlen(absPath);
        if (length > 0 && absPath[length - 1] == '/' && length - 1 != 0) {
            absPath[length - 1] = '\0';
        }
    }
}

/* 0x804C8E10 (0x12C): brings the NAND file system up and sets the home directory; zero on success. */
s32 NANDInit(void)
{
    BOOL enabled = OSDisableInterrupts();
    s32 result;

    if (s_libState == NAND_LIB_STATE_BUSY) {
        OSRestoreInterrupts(enabled);
        return NAND_RESULT_BUSY;
    } else if (s_libState == NAND_LIB_STATE_READY) {
        OSRestoreInterrupts(enabled);
        return 0;
    }
    s_libState = NAND_LIB_STATE_BUSY;
    OSRestoreInterrupts(enabled);
    result = ISFS_OpenLib();
    if (result == 0) {
        u64 titleId;

        result = ESP_InitLib();
        if (result == 0) {
            result = ESP_GetTitleId(&titleId);
        }
        if (result == 0) {
            result = ESP_GetDataDir(titleId, s_homeDir);
        }
        if (result == 0) {
            strcpy(s_currentDir, s_homeDir);
        }
        ESP_CloseLib();
        if (result != 0) {
            OSReport("Failed to set home directory.\n");
        }
        OSRegisterShutdownFunction(&s_shutdownFuncInfo);
        enabled = OSDisableInterrupts();
        s_libState = NAND_LIB_STATE_READY;
        OSRestoreInterrupts(enabled);
        OSRegisterVersion(__NANDVersion);
        return 0;
    } else {
        enabled = OSDisableInterrupts();
        s_libState = NAND_LIB_STATE_NONE;
        OSRestoreInterrupts(enabled);
        return nandConvertErrorCode(result);
    }
}

/* 0x804C8F40 (0xCC): the shutdown function: waits up to 500 ms for the ISFS shutdown before a reset. */
static BOOL nandOnShutdown(BOOL final, u32 event)
{
    if (!final) {
        if (event == 2) {
            volatile s32 done = 0;
            s64 start = OSGetTime();

            ISFS_ShutdownAsync(nandShutdownCallback, (void*)&done);
            while ((OSGetTime() - start) / (OS_BUS_CLOCK / 4 / 1000) < NAND_SHUTDOWN_TIMEOUT_MS) {
                if (done) {
                    break;
                }
            }
        }
        return TRUE;
    }
    return TRUE;
}

/* 0x804C9010 (0xC): the ISFS completion of the shutdown wait: sets the caller's s32 done flag to 1. */
void nandShutdownCallback(s32 result, NANDCommandBlock* block)
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

/* 0x804C90C0 (0x2D4): probes the node type of `path` (asynchronously when `async`); a path below the private
 * directory needs `privileged`. */
s32 nandGetType(const char* path, u8* type, NANDCommandBlock* block, BOOL async, BOOL privileged)
{
    if (strlen(path) == 0) {
        return ISFS_ERROR_INVALID;
    }
    if (async) {
        nandGenerateAbsPath(block->absPath, path);
        if (!privileged && nandIsUnderPrivatePath(block->absPath)) {
            return ISFS_ERROR_ACCESS;
        }
        block->type = type;
        return ISFS_ReadDirAsync(block->absPath, NULL, &block->num, nandGetTypeCallback, block);
    } else {
        char absPath[64] = { 0 };
        u32 num;
        s32 result;

        nandGenerateAbsPath(absPath, path);
        if (!privileged && nandIsUnderPrivatePath(absPath)) {
            return ISFS_ERROR_ACCESS;
        }
        num = 0;
        result = ISFS_ReadDir(absPath, NULL, &num);
        if (result == 0 || result == ISFS_ERROR_ACCESS) {
            *type = NAND_TYPE_DIR;
            result = 0;
        } else if (result == ISFS_ERROR_INVALID) {
            *type = NAND_TYPE_FILE;
            result = 0;
        }
        return result;
    }
}

/* 0x804C93A0 (0x48): starts probing the node type of `path` by its absolute name. */
s32 NANDPrivateGetTypeAsync(const char* path, u8* type, NANDAsyncCallback callback, NANDCommandBlock* block)
{
    if (!nandIsInitialized()) {
        return NAND_RESULT_NOT_INITIALIZED;
    }
    block->callback = callback;
    return nandConvertErrorCode(nandGetType(path, type, block, TRUE, TRUE));
}

/* 0x804C93F0 (0x78): finishes an asynchronous type probe: stores the node type and calls the caller's callback. */
void nandGetTypeCallback(s32 result, NANDCommandBlock* block)
{
    if (result == 0 || result == ISFS_ERROR_ACCESS) {
        *block->type = NAND_TYPE_DIR;
        result = 0;
    } else if (result == ISFS_ERROR_INVALID) {
        *block->type = NAND_TYPE_FILE;
        result = 0;
    }
    block->callback(nandConvertErrorCode(result), block);
}

/* 0x804C9470 (0xC): returns the title's home directory path. */
char* nandGetHomeDir(void)
{
    return s_homeDir;
}
