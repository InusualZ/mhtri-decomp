/*
 * NAND/NANDCore.h - declarations of the symbols owned by `NAND/NANDCore.c`: library bring-up, the home directory, the
 * node-type probe and the path/result helpers the other NAND units call.
 */
#ifndef NAND_NANDCORE_H
#define NAND_NANDCORE_H

#include "NAND/nand.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804C8E10 - bring the NAND filesystem up; zero on success. */
s32 NANDInit(void);
/* 0x804C9020 - copy the title's home directory path into `path`. */
s32 NANDGetHomeDir(char* path);
/* 0x804C93A0 - probe the node type of `path` (1 file, 2 directory) by its absolute name. */
s32 NANDPrivateGetTypeAsync(const char* path, u8* type, NANDAsyncCallback callback, NANDCommandBlock* block);

/* 0x804C8BD0 - the NAND result for an ISFS result. */
s32 nandConvertErrorCode(s32 result);
/* 0x804C8BB0 - whether the library has been brought up. */
BOOL nandIsInitialized(void);
/* 0x804C9080 - the shared async completion: converts the ISFS result and calls the caller's callback. */
void nandAsyncCallback(s32 result, NANDCommandBlock* block);
/* 0x804C8D40 - the absolute path of `path` (relative paths are taken below the home directory). */
void nandGenerateAbsPath(char* absPath, const char* path);
/* 0x804C8B10 - whether `path` is the private directory. */
BOOL nandIsPrivatePath(const char* path);

#ifdef __cplusplus
}
#endif

#endif
