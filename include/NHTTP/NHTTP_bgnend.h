/*
 * include/NHTTP/NHTTP_bgnend.h - the NHTTP begin/end unit (`src/NHTTP/NHTTP_bgnend.c`, `.text`
 * 0x805145B8..0x80515010).
 *
 * Rule 2: this unit owns the error/alloc/list/mutex entry points, so `d_nhttp.c` includes this
 * header instead of declaring them.  The NHTTP system-info block it operates on is `d_nhttp.h`'s
 * `NHTTPInfo` (rule 1: one definition, in the owner's header).
 */
#ifndef MHTRI_NHTTP_NHTTP_BGNEND_H
#define MHTRI_NHTTP_NHTTP_BGNEND_H

#include "types.h"
#include "NHTTP/d_nhttp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The NHTTP request record; only the fields the header path touches are modelled. size: 0x34 */
typedef struct NHTTPRequest {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;              /* committed-header flag NHTTP_AddHeaderField tests */
    /* +0x08 */ u8 pad_0x08[0x28];
    /* +0x30 */ NHTTPHeaderField headerList; /* the request's circular header-field list head */
} NHTTPRequest; /* size: 0x34 */

void NHTTPi_SetError(NHTTPInfo* info, s32 err);
void NHTTPi_SetSSLError(NHTTPInfo* info, s32 err);
s32 NHTTPi_GetSSLError(NHTTPInfo* info);
s32 NHTTPi_GetError(NHTTPInfo* info);
void NHTTPi_InitListInfo(NHTTPListInfo* info);
void NHTTPi_InitSystemInfo(NHTTPInfo* info);
void NHTTPi_InitMutexInfo(NHTTPMutexInfo* info);
void NHTTPi_initLockReqList(NHTTPMutexInfo* mutex);
void NHTTPi_lockReqList(NHTTPMutexInfo* mutex);
void NHTTPi_exitLockReqList(void);
void* NHTTPi_SetHeaderField(NHTTPHeaderField* list, NHTTPInfo* owner, const char* token,
                            const char* value);
u32 NHTTP_AddHeaderField(NHTTPRequest* request, NHTTPInfo* owner, const char* token,
                         const char* value);
NHTTPHeaderField* NHTTPi_RemoveNode(NHTTPHeaderField** ppHead);
void* NHTTPi_alloc(void* heap, u32 size);
void NHTTPi_free(void* p);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NHTTP_NHTTP_BGNEND_H */
