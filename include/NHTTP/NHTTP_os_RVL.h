/*
 * include/NHTTP/NHTTP_os_RVL.h - the NHTTP RVL platform unit (`src/NHTTP/NHTTP_os_RVL.c`, `.text`
 * 0x80515010..0x80515774).
 *
 * Rule 2: this unit owns `NHTTPi_InitRequestInfo` (and the RVL thread/receive-buffer helpers), so
 * `d_nhttp.c` includes this header instead of declaring them.
 */
#ifndef MHTRI_NHTTP_NHTTP_OS_RVL_H
#define MHTRI_NHTTP_NHTTP_OS_RVL_H

#include "types.h"
#include "NHTTP/d_nhttp.h"

#ifdef __cplusplus
extern "C" {
#endif

void NHTTPi_InitRequestInfo(NHTTPRequestInfo* info);
void NHTTPi_CheckCurrentThread(void* info);
s32 NHTTPi_isRecvBufFull(NHTTPInfo* info, u32 size);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NHTTP_NHTTP_OS_RVL_H */
