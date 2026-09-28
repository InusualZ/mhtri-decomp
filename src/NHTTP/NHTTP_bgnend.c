/*
 * NHTTP_bgnend.c - the Revolution SDK NHTTP library's begin/end TU, `.text`
 * 0x805145B8..0x80515010 (27 functions, 2648 B).
 *
 * REGISTRATION (recon lane, 2026-09-27).  Left edge 0x805145B8 is the NHTTP library start: the
 * `-func_align 16` -> `-func_align 4` change at this address (every function above it is 16-byte
 * aligned with `gap_*` padding, this one and everything below packs on 4) is a hard instruction-level
 * boundary, and `NHTTPi_alloc` (0x805145E8), `NHTTPi_free`, `NHTTPi_SetError`, `NHTTPi_SetSSLError`
 * and `NHTTPi_Startup` (0x805146A0) begin here.  Right edge 0x80515010 is the `.sdata` run-jump
 * interval's lower bound for the next TU (`tudiscover`: `.sdata run jump 0x80794394 -> 0x807943A0`
 * admits cuts [18653,18686]); 0x80515010 is `NHTTPi_CheckCurrentThread`, the first referrer of the
 * next TU's private string, so that anchor starts the next unit.
 *
 * SOURCE FILE.  Real name recovered from the unit's own pool: `NHTTPi_Startup` loads
 * `.data:0x80630A28` = `"NHTTP_bgnend.c"` (the group 0x80630A08, which also carries the
 * `NCDGetCurrentIpConfig`/`*warning: %d connections rests` strings).  `dumpmap.py`/the runtime dump
 * answer only `zz_05145b8_` for the code, so the `__FILE__` string is the name evidence.
 *
 * LIBRARY.  `NHTTP`: `NHTTPi_Startup` (the anchor for the file name above) and every other named
 * symbol here (`NHTTPi_alloc`/`free`/`SetError`/`SetSSLError`/`CleanupAsync`) is an NHTTPi entry
 * point, and the unit's callees are `NHTTPi_InitListInfo`/`initLockReqList`/`createCommThread` plus
 * `NCDGetCurrentIpConfig` - i.e. the HTTP library initialising itself, not the game.
 *
 * SECTIONS.  `.text` only - the target object is exactly that (the four `.data` literal groups and
 * the one `.rodata` code table the bodies address belong to dtk's auto data runs, so they are
 * declared `extern` in `include/unsplit/NHTTP.h` and never defined here; playbook 29/58).
 *
 * FLAGS.  `cflags_nhttp` (`Wii/1.3`, `-func_align 4`, copied from `cflags_dwc`/`cflags_os`) - the
 * 4-byte packing above is the evidence for the alignment; nothing else is tuned.
 *
 * NAMING (rule 7).  Six names were derived from their bodies (each a GUESS - the runtime dump answers
 * only `zz_05145b8_`):
 *   0x80514698 -> NHTTPi_GetSSLError     the 0x7D4 getter paired with NHTTPi_SetSSLError
 *   0x80514920 -> NHTTPi_GetError        the 0x7D8 getter paired with NHTTPi_SetError
 *   0x80514A64 -> NHTTPi_RemoveNode      unlinks the head of the request's circular header list
 *   0x80514928 -> NHTTPi_SetHeaderField  searches/allocates a header-field node
 *   0x805145B8 -> NHTTPi_InitSystemInfo  zeroes the info block (ref'd by d_nhttp's GetSystemInfoP)
 *   0x80514EA8 -> NHTTPi_InitMutexInfo   clears the mutex-info lazy-init flag (ref'd by d_nhttp)
 * The body pass added the rest, all GUESSes from their own code:
 *   0x80514ACC -> NHTTPAddPostDataRaw    the function's own printf names it (the `(exclusive
 *                                        fucntion)` warning string at `.data` 0x80630AAC)
 *   0x80514C68 -> NHTTPi_insertRequest   inserts a request node and hands back the list's next id
 *   0x80514D34 -> NHTTPi_cancelRequest   unlinks one id and completes its connection
 *   0x80514E50 -> NHTTPi_cancelConnectionRequests  that, repeated for every node of one connection
 *   0x80514EA4 -> NHTTPi_RemoveHeaderField          a 4-byte tail to NHTTPi_RemoveNode
 *   0x80514EFC -> NHTTPi_unlockReqList   the OSUnlockMutex half of NHTTPi_lockReqList
 *   0x80514FDC -> NHTTPi_receiveMessage  blocking receive on the comm queue
 *   0x80515004 -> NHTTPi_sendQuitMessage non-blocking null send (the quit signal)
 * and the two callees it reaches inside the next units are named with their owners
 * (`NHTTPi_commThreadMain`/`NHTTPi_InitRequestInfo` in `NHTTP_os_RVL.h`, `NHTTPi_commThreadLoop` /
 * `NHTTPi_markCommThreadReady` / `NHTTPi_isCommThreadReady` / `NHTTPi_strlen` /
 * `NHTTPi_containsString` in `d_nhttp.h`).
 *
 * BODY.  All 27 functions are reconstructed and measured; 21 are byte-identical, the unit is 93.24 %
 * fuzzy over the 2648 B of `.text` (2644 B ours), and the remaining five rows are:
 *
 *    93.85  NHTTPi_Startup             (400/400)
 *    93.99  NHTTPi_SetHeaderField       (316/316)
 *    85.64  NHTTPAddPostDataRaw        (396/384)
 *    84.45  NHTTPi_insertRequest       (204/200)
 *    83.68  NHTTPi_cancelRequest       (284/296)
 *    99.33  NHTTPi_RemoveNode          (72/72)
 *
 * Residuals (each measured with `measure.py --diff` on this file):
 *   - NHTTPi_Startup: retail gives the first parameter register r31 and ours gives it r26, so every
 *     `info` use is one register off (`mr r3,r26` where retail has `mr r3,r31`); the function's shape
 *     (including the one `NCDGetCurrentIpConfig` call and the hoisted message-group base) is identical.
 *     The colouring follows the source's *declaration order* - declaring the message-group local first
 *     is what fixed `lis r31` vs `lis r30` - and moving the parameter's web to the front of that order
 *     is not reachable from the C side (tried: parameter copy to a local first, declaration reorder).
 *   - NHTTPi_SetHeaderField: register naming in the search walk plus a two-instruction tail.
 *   - NHTTPAddPostDataRaw: the inner walk re-derives the buffer slot per statement where retail keeps
 *     `request + i` live in a register, and the `advance` flag's two exits are re-tested.
 *   - NHTTPi_insertRequest / NHTTPi_cancelRequest: a four-byte frame difference and the loop variable's
 *     colouring (`r4` vs `r7`); the id comparison is `cmplw` in both.
 *   - `NHTTPi_alloc`/`NHTTPi_free`/`NHTTPi_insertRequest`/`NHTTPi_SetHeaderField`/`NHTTPi_RemoveHeaderField`
 *     all needed `#pragma dont_inline on`: with `-inline auto` MWCC folds the small callees and drops
 *     retail's `bl` (72 B instead of 4 B on the tail wrapper, 228 B instead of 204 on the insert).
 */

#include "NHTTP/NHTTP_bgnend.h"
#include "NHTTP/NHTTP_os_RVL.h"   /* NHTTPi_InitRequestInfo, NHTTPi_CheckCurrentThread (rule 2) */
#include "unsplit/OS.h"           /* the OS thread/message set + OSReport/OSPanic (rule 2 band) */
#include "unsplit/SO.h"           /* SOClose (rule 2 band) */
#include "unsplit/NCD.h"          /* NCDGetCurrentIpConfig (rule 2 band) */
#include "unsplit/NHTTP.h"        /* the four literal groups + the code table (rule 2 band) */
#include "unsplit/Runtime.PPCEABI.H.h" /* printf (rule 2 band) */


/* -------------------------------------------------------------------------------------------------
 * .text 0x805145B8 .. 0x80515010, in address order.
 * ------------------------------------------------------------------------------------------------ */

/* 0x805145B8 (0x30): reset the info block's error, allocator and comm-buffer slots.  Retail clears
 * the two error words before the allocator pair, so the store order is kept verbatim. */
void NHTTPi_InitSystemInfo(NHTTPInfo* info) {
    info->ssl_error = 0;
    info->error = 0;
    info->alloc_fn = 0;
    info->free_fn = 0;
    info->field_0x7CC = 0;
    info->field_0x7D0 = -1;
    info->field_0x7DC = 0;
    info->comm_stack = 0;
    info->field_0x7E4 = 0;
}

/* 0x805145E8 (0x5C): forward to the registered allocator (a null slot answers null). */
/* untyped: byte range */
void* NHTTPi_alloc(u32 size, u32 align) {
    NHTTPAllocFn fn = (NHTTPAllocFn)NHTTPi_GetSystemInfoP()->alloc_fn;
    void* memory;

    if (fn != 0) {
        memory = fn(size, align);
    } else {
        memory = 0;
    }
    return memory;
}

/* 0x80514644 (0x44): forward to the registered free. */
/* untyped: byte range */
void NHTTPi_free(void* block) {
    NHTTPFreeFn fn = (NHTTPFreeFn)NHTTPi_GetSystemInfoP()->free_fn;

    if (fn != 0) {
        fn(block);
    }
}

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

/* 0x805146A0 (0x190): initialise the caller's info block, allocate the comm thread's stack and
 * start the comm thread.  The allocator and free come from the *singleton* info block, not from the
 * block being initialised, so the two are distinct objects here. */
s32 NHTTPi_Startup(NHTTPInfo* info, NHTTPAllocFn alloc, NHTTPFreeFn free, u32 arg) {
    NHTTPInfo* base = info;
    const char* messages = NHTTPi_startupMessages;
    NHTTPInfo* bgnend = (NHTTPInfo*)NHTTPi_GetBgnEndInfoP(base);
    NHTTPListInfo* list = (NHTTPListInfo*)NHTTPi_GetListInfoP(base);
    NHTTPRequestInfo* request = (NHTTPRequestInfo*)NHTTPi_GetReqInfoP(base);
    NHTTPMutexInfo* mutex = (NHTTPMutexInfo*)NHTTPi_GetMutexInfoP(base);
    NHTTPThreadInfo* thread = (NHTTPThreadInfo*)NHTTPi_GetThreadInfoP(base);
    u8* memory = 0;

    bgnend->alloc_fn = (void*)alloc;
    bgnend->free_fn = (void*)free;
    bgnend->error = 0;
    bgnend->ssl_error = 0;
    bgnend->field_0x7DC = 0;
    NHTTPi_InitListInfo(list);
    NHTTPi_InitRequestInfo(request);
    NHTTPi_initLockReqList(mutex);
    NHTTPi_InitConnectionList();
    bgnend->field_0x7D0 = -1;
    {
        NHTTPInfo* sys = NHTTPi_GetSystemInfoP();

        if (sys->alloc_fn != 0) {
            memory = (u8*)((NHTTPAllocFn)sys->alloc_fn)(8192, 8);
        }
    }
    bgnend->comm_stack = memory;
    if (memory == 0) {
        bgnend->error = 1;
        NHTTPi_exitLockReqList();
        return 0;
    }
    if (NHTTPi_createCommThread(thread, arg, memory) == 0) {
        NHTTPInfo* sys;
        u8* buffer;

        bgnend->error = 9;
        buffer = (u8*)bgnend->comm_stack;
        sys = NHTTPi_GetSystemInfoP();
        if (sys->free_fn != 0) {
            ((NHTTPFreeFn)sys->free_fn)(buffer);
        }
        bgnend->comm_stack = 0;
        NHTTPi_exitLockReqList();
        return 0;
    }
    {
        s32 error = NCDGetCurrentIpConfig(bgnend);

        if (error < 0) {
            OSReport(messages, error);
            OSPanic(messages + 0x20, 230, messages + 0x30);
        }
    }
    bgnend->field_0x7CC = 1;
    return 1;
}

/* 0x80514830 (0xF0): stop the comm thread, free its stack, close the socket and report whatever the
 * caller's completion callback and the leftover connections have to say. */
void NHTTPi_CleanupAsync(NHTTPInfo* info, NHTTPCompletionCallback callback) {
    NHTTPInfo* bgnend = (NHTTPInfo*)NHTTPi_GetBgnEndInfoP(info);
    NHTTPThreadInfo* thread = (NHTTPThreadInfo*)NHTTPi_GetThreadInfoP(info);
    u8* buffer;
    s32 count;
    s32 fd;

    NHTTPi_CheckCurrentThread(thread, 1);
    NHTTPi_cancelAllRequests(info);
    NHTTPi_destroyCommThread(thread, bgnend);
    buffer = (u8*)bgnend->comm_stack;
    {
        NHTTPInfo* sys = NHTTPi_GetSystemInfoP();

        if (sys->free_fn != 0) {
            ((NHTTPFreeFn)sys->free_fn)(buffer);
        }
    }
    buffer = 0;
    bgnend->comm_stack = buffer;
    NHTTPi_exitLockReqList();
    bgnend->field_0x7CC = 0;
    if (callback != 0) {
        callback();
    }
    count = NHTTPi_GetConnectionListLength();
    if (count != 0) {
        printf(NHTTPi_connRestWarning, count);
    }
    fd = bgnend->field_0x7D0;
    if (fd >= 0) {
        SOClose(fd);
        bgnend->field_0x7D0 = -1;
    }
}

/* 0x80514920 (8): the NHTTP error getter. */
s32 NHTTPi_GetError(NHTTPInfo* info) {
    return info->error;
}

/* 0x80514928 (0x13C): find the header-field node whose token matches and overwrite its value, or
 * allocate a new node, fill it in and link it in before the list head.  The search walks `prev`
 * around the circular list, retail's first comparison unrolled.  Retail keeps the
 * `bl NHTTPi_alloc`, so the inliner is held off here (playbook 61). */
#pragma dont_inline on
s32 NHTTPi_SetHeaderField(NHTTPHeaderField** ppHead, NHTTPInfo* owner, const char* token,
                          const char* value) {
    NHTTPHeaderField* node = *ppHead;
    s32 found = 0;

    if (node != 0) {
        if (NHTTPi_compareToken(token, node->token) == 0) {
            found = 1;
        } else {
            for (node = node->prev; node != *ppHead; node = node->prev) {
                if (NHTTPi_compareToken(token, node->token) == 0) {
                    found = 1;
                    break;
                }
            }
        }
    }
    if (found) {
        node->value = value;
    } else {
        NHTTPHeaderField* newNode = (NHTTPHeaderField*)NHTTPi_alloc(24, 4);

        if (newNode == 0) {
            printf(NHTTPi_allocFailMessage);
            NHTTPi_SetError(owner, 1);
            return 0;
        }
        newNode->token = token;
        newNode->value = value;
        newNode->field_0x10 = 0;
        newNode->field_0x14 = 0;
        if (*ppHead != 0) {
            newNode->next = (*ppHead)->next;
            newNode->prev = *ppHead;
            (*ppHead)->next->prev = newNode;
            (*ppHead)->next = newNode;
        } else {
            newNode->prev = newNode;
            newNode->next = newNode;
            *ppHead = newNode;
        }
    }
    return 1;
}
#pragma dont_inline off

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

/* 0x80514ACC (0x18C): the exclusive "post data raw" entry.  It walks the request's 18-byte code
 * (`+0x3A`) up to `NHTTPi_postDataRawCode` one character at a time, re-submitting the code through
 * `NHTTPi_containsString` after every step, and then registers the field.  The name is the one the
 * function's own warning string carries (`.data` 0x80630AAC). */
s32 NHTTPAddPostDataRaw(NHTTPRequest* request, NHTTPInfo* owner, const char* token,
                        const char* value) {
    s32 length = 0;
    s32 doRegister = 0;
    s32 i;

    if (request->field_0x04 != 0) {
        return 0;
    }
    if (request->field_0x10 != 0) {
        printf(NHTTPi_postDataRawMessage);
        return 0;
    }
    if (value != 0) {
        length = NHTTPi_strlen(value);
    }
    if (NHTTPi_containsString(value, length, request->code, 18) < 0) {
        doRegister = 1;
    } else {
        for (i = 19; i >= 2; i--) {
            for (;;) {
                u8 next = (u8)((u8*)request)[56 + i] + 1;

                if (next == '{') {
                    next = '0';
                } else if (next == '[') {
                    next = 'a';
                } else if (next == ':') {
                    next = 'A';
                }
                ((u8*)request)[56 + i] = next;
                if ((char)next == NHTTPi_postDataRawCode[i]) {
                    break;
                }
                if (NHTTPi_containsString(value, length, request->code, 18) < 0) {
                    doRegister = 1;
                    break;
                }
            }
            if (doRegister) {
                break;
            }
        }
    }
    {
        NHTTPHeaderField* node = 0;

        if (doRegister) {
            node = (NHTTPHeaderField*)NHTTPi_SetHeaderField(&request->postDataList, owner, token,
                                                            value);
            if (node != 0) {
                node->field_0x10 = (u32)length;
            }
        }
        return (s32)node;
    }
}

/* 0x80514C58 (0x10): zero a list-info record. */
void NHTTPi_InitListInfo(NHTTPListInfo* info) {
    info->head = 0;
    info->tail = 0;
}

/* 0x80514C68 (0xCC): link a new request node in after the list head and hand back the list's next
 * id (`-1` when the allocation failed).  `list->tail` is the id counter, not a tail pointer.
 * Retail keeps the `bl NHTTPi_alloc`, so the inliner is held off here (playbook 61). */
#pragma dont_inline on
u32 NHTTPi_insertRequest(NHTTPRequestList* list, s32 request) {
    NHTTPRequestNode* node = (NHTTPRequestNode*)NHTTPi_alloc(20, 4);
    u32 id = (u32)-1;

    if (node != 0) {
        if (list->head != 0) {
            node->next = list->head->next;
            node->prev = list->head;
            list->head->next->prev = node;
            list->head->next = node;
        } else {
            node->next = node;
            node->prev = node;
            list->head = node;
        }
        node->id = list->nextId;
        list->nextId = list->nextId + 1;
        node->request = request;
        node->state = -1;
        id = node->id;
        if ((s32)list->nextId < 0) {
            list->nextId = 0;
        }
    }
    return id;
}
#pragma dont_inline off

/* 0x80514D34 (0x11C): unlink the request with the given id, tear its request object down and
 * complete the connection it belonged to. */
/* untyped: opaque handle */
s32 NHTTPi_cancelRequest(NHTTPRequestList* list, void* connection, u32 id) {
    NHTTPRequestNode* head = list->head;
    NHTTPRequestNode* node = 0;
    s32 ret = 0;

    if (head != 0) {
        if (head->id == id) {
            node = head;
        } else {
            NHTTPRequestNode* n;

            for (n = head->prev; n != head; n = n->prev) {
                if (n->id == id) {
                    node = n;
                    break;
                }
            }
        }
    }
    if (node != 0) {
        s32 obj;

        if (head != head->next) {
            node->next->prev = node->prev;
            node->prev->next = node->next;
            if (list->head == node) {
                list->head = node->prev;
            }
        } else {
            list->head = 0;
        }
        obj = NHTTPi_Request2Connection(connection, node->request);
        NHTTPi_destroyRequestObject(connection, node->request);
        NHTTPi_free(node);
        if (obj != 0) {
            NHTTPi_CompleteCallback(connection, (void*)obj);
        }
        ret = 1;
    }
    return ret;
}

/* 0x80514E50 (0x54): cancel every request the list holds for one connection. */
/* untyped: opaque handle */
void NHTTPi_cancelConnectionRequests(NHTTPRequestList* list, void* connection) {
    while (list->head != 0) {
        NHTTPi_cancelRequest(list, connection, list->head->id);
    }
}

/* 0x80514EA4 (4): remove and return the list's head node.  Retail keeps the tail call to
 * NHTTPi_RemoveNode, so the inliner is held off for this one function (playbook 61). */
#pragma dont_inline on
NHTTPHeaderField* NHTTPi_RemoveHeaderField(NHTTPHeaderField** ppHead) {
    return NHTTPi_RemoveNode(ppHead);
}
#pragma dont_inline off

/* 0x80514EA8 (0xC): clear the mutex-info lazy-init flag. */
void NHTTPi_InitMutexInfo(NHTTPMutexInfo* info) {
    info->created = 0;
}

/* 0x80514EB4 (0x40): create the request-list mutex once. */
void NHTTPi_initLockReqList(NHTTPMutexInfo* mutex) {
    if (mutex->created == 0) {
        OSInitMutex(mutex);
        mutex->created = 1;
    }
}

/* 0x80514EF4 (4): no-op tail (the lock is a no-op in this build). */
void NHTTPi_exitLockReqList(void) {
}

/* 0x80514EF8 (4): tail to the OS mutex lock. */
void NHTTPi_lockReqList(NHTTPMutexInfo* mutex) {
    OSLockMutex(mutex);
}

/* 0x80514EFC (4): tail to the OS mutex unlock. */
void NHTTPi_unlockReqList(NHTTPMutexInfo* mutex) {
    OSUnlockMutex(mutex);
}

/* 0x80514F00 (0x94): initialise the comm thread's message queue once, then create and start the
 * thread over the caller's 8192-byte stack block (the stack pointer starts at its top). */
s32 NHTTPi_createCommThread(NHTTPThreadInfo* thread, u32 arg, u8* stack) {
    if (NHTTPi_isCommThreadReady(thread) == 0) {
        OSInitMessageQueue(&thread->queue, thread->msgArray, 3);
        NHTTPi_markCommThreadReady(thread);
    }
    OSCreateThread(&thread->thread, NHTTPi_commThreadMain, 0, stack + 8192, 8192, arg, 0);
    OSResumeThread(&thread->thread);
    return 1;
}

/* 0x80514F94 (0x48): flag the quit, wake the queue and wait for the thread to exit. */
void NHTTPi_destroyCommThread(NHTTPThreadInfo* thread, NHTTPInfo* info) {
    info->field_0x7DC = 1;
    OSSendMessage(&thread->queue, 0, OS_MESSAGE_NOBLOCK);
    OSJoinThread(&thread->thread, 0);
}

/* 0x80514FDC (0x28): take one message from the comm queue, blocking until there is one. */
BOOL NHTTPi_receiveMessage(OSMessageQueue* queue) {
    OSMessage message;

    return OSReceiveMessage(queue, &message, OS_MESSAGE_BLOCK);
}

/* 0x80515004 (0xC): post the null quit message without blocking. */
BOOL NHTTPi_sendQuitMessage(OSMessageQueue* queue) {
    return OSSendMessage(queue, 0, OS_MESSAGE_NOBLOCK);
}
