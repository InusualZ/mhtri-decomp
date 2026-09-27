/*
 * NHTTP_bgnend.c - the Revolution SDK NHTTP library's begin/end TU, `.text`
 * 0x805145B8..0x80515010 (27 functions, 2648 B).
 *
 * REGISTRATION (recon lane, 2026-09-27).  Left edge 0x805145B8 is the NHTTP library start: the
 * `-func_align 16` -> `-func_align 4` change at this address (every function above it is 16-byte
 * aligned with `gap_*` padding, this one and everything below packs on 4) is a hard instruction-level
 * boundary, and `NHTTPi_alloc` (0x805145E8), `NHTTPi_free`, `NHTTPi_SetError`, `NHTTPi_SetSSLError`
 * and `NHTTPi_Startup` (0x805146A0) begin here.  Right edge 0x80515010 is the `.sdata` run-jump
 * interval's lower bound for the next TU (`tudiscover`: `.sdata run jump lbl_80794394 -> lbl_807943A0`
 * admits cuts [18653,18686]); 0x80515010 is `NHTTPi_CheckCurrentThread`, the first referrer of the
 * next TU's private string, so that anchor starts the next unit.
 *
 * SOURCE FILE.  Real name recovered from the unit's own pool: `NHTTPi_Startup` loads
 * `.data:0x80630A28` = `"NHTTP_bgnend.c"` (the group lbl_80630A08, which also carries the
 * `NCDGetCurrentIpConfig`/`*warning: %d connections rests` strings).  `dumpmap.py`/the runtime dump
 * answer only `zz_05145b8_` for the code, so the `__FILE__` string is the name evidence.
 *
 * LIBRARY.  `NHTTP`: `NHTTPi_Startup` (the anchor for the file name above) and every other named
 * symbol here (`NHTTPi_alloc`/`free`/`SetError`/`SetSSLError`/`CleanupAsync`) is an NHTTPi entry
 * point, and the unit's callees are `NHTTPi_InitListInfo`/`initLockReqList`/`createCommThread` plus
 * `NCDGetCurrentIpConfig` - i.e. the HTTP library initialising itself, not the game.
 *
 * SECTIONS.  `.text` only.
 *
 * FLAGS.  `cflags_nhttp` (`Wii/1.3`, `-func_align 4`, copied from `cflags_dwc`/`cflags_os`) - the
 * 4-byte packing above is the evidence for the alignment; nothing else is tuned.
 *
 * NAMING (rule 7).  Four `fn_` names the unit owns were derived from their bodies (each a GUESS - the
 * runtime dump answers only `zz_05145b8_`):
 *   fn_80514698 -> NHTTPi_GetSSLError     the 0x7D4 getter paired with NHTTPi_SetSSLError
 *   fn_80514920 -> NHTTPi_GetError        the 0x7D8 getter paired with NHTTPi_SetError
 *   fn_80514A64 -> NHTTPi_RemoveNode      unlinks the head of the request's circular header list
 *   fn_80514928 -> NHTTPi_SetHeaderField  searches/allocates a header-field node (ref'd, body elsewhere)
 *   fn_805145B8 -> NHTTPi_InitSystemInfo  zeroes the info block (ref'd by d_nhttp's GetSystemInfoP)
 *   fn_80514EA8 -> NHTTPi_InitMutexInfo   clears the mutex-info lazy-init flag (ref'd by d_nhttp)
 * The remaining `fn_` names in this range have no body here and stay the map's placeholders.
 *
 * BODY (probe).  12 of the 27 functions, all small and fully pinned by the disassembly; 11 are
 * byte-identical at 100 %, `NHTTPi_RemoveNode` at 99.33 % (one evaluation-order pair).
 */

#include "NHTTP/NHTTP_bgnend.h"

extern void OSLockMutex(void* mutex);
extern void OSInitMutex(void* mutex);

/* 0x80514688 (8): store the NHTTP error into the info block. */
void NHTTPi_SetError(NHTTPInfo* info, s32 err) {
    info->error = err;
}

/* 0x80514690 (8): same, the SSL slot. */
void NHTTPi_SetSSLError(NHTTPInfo* info, s32 err) {
    info->ssl_error = err;
}

/* 0x80514698 (8): the SSL error getter. */
s32 NHTTPi_GetSSLError(NHTTPInfo* info) {
    return info->ssl_error;
}

/* 0x80514920 (8): the NHTTP error getter. */
s32 NHTTPi_GetError(NHTTPInfo* info) {
    return info->error;
}

/* 0x80514C58 (0x10): zero a list-info record. */
void NHTTPi_InitListInfo(NHTTPListInfo* info) {
    info->head = 0;
    info->tail = 0;
}

/* 0x80514EF4 (4): no-op tail (the lock is a no-op in this build). */
void NHTTPi_exitLockReqList(void) {
}

/* 0x80514EF8 (4): tail to the OS mutex lock. */
void NHTTPi_lockReqList(NHTTPMutexInfo* mutex) {
    OSLockMutex(mutex);
}

/* 0x80514EB4 (0x40): create the request-list mutex once. */
void NHTTPi_initLockReqList(NHTTPMutexInfo* mutex) {
    if (mutex->created == 0) {
        OSInitMutex(mutex);
        mutex->created = 1;
    }
}

/* 0x80514AAC (0x20): append a header field unless the request already committed one.  The four
 * arguments are forwarded verbatim to NHTTPi_SetHeaderField (retail's tail call leaves r4..r6
 * untouched), which is why the signature carries `owner`/`token`/`value` even though the body does
 * not read them. */
u32 NHTTP_AddHeaderField(NHTTPRequest* request, NHTTPInfo* owner, const char* token,
                         const char* value) {
    if (request->field_0x04 != 0) {
        return 0;
    }
    return (u32)NHTTPi_SetHeaderField(&request->headerList, owner, token, value);
}

/* 0x80514A64 (0x48): unlink the head node of a circular doubly-linked header-field list. */
NHTTPHeaderField* NHTTPi_RemoveNode(NHTTPHeaderField** ppHead) {
    NHTTPHeaderField* node = *ppHead;
    if (node != 0) {
        if (node != node->next) {
            node->next->prev = node->prev;
            node->prev->next = node->next;
            *ppHead = node->prev;
        } else {
            *ppHead = 0;
        }
    }
    return node;
}

/* 0x805145E8 (0x5C): forward to the registered allocator. */
void* NHTTPi_alloc(void* heap, u32 size) {
    void* fn = NHTTPi_GetSystemInfoP()->alloc_fn;
    void* memory;
    if (fn != 0) {
        memory = ((void* (*)(void*, u32))fn)(heap, size);
    } else {
        memory = 0;
    }
    return memory;
}

/* 0x80514644 (0x44): forward to the registered free. */
void NHTTPi_free(void* p) {
    void* fn = NHTTPi_GetSystemInfoP()->free_fn;
    if (fn != 0) {
        ((void (*)(void*))fn)(p);
    }
}
