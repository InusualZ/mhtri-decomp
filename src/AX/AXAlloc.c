/*
 * AX/AXAlloc.c - the AX voice allocator: the per-priority voice lists, the callback stack, `AXAcquireVoice` and
 *    `AXFreeVoice`.
 *
 * RANGE. .text 0x8046E4F0-0x8046EA90 (10 functions, 0x5A0 B); .bss 0x806F74E0-0x806F75E0; .sbss
 *    0x80794ED0-0x80794ED8.  Cut from the old ARC/AX block between `AX/AX.c` (0x8046E4F0) and `AX/AXAux.c`
 *    (0x8046EA90).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the `__AX*` and `AX*` names are the map's; `__AXAllocInit` (0x8046E5C0, the map's `PatInterface_VTable`),
 *    `__AXPushFreeStack` (0x8046E720), `AXSetVoicePriority` (0x8046E9F0), `__AXStackHead`, `__AXStackTail` and
 *    `__AXCallbackStack` are GUESSes named from what the code does; the file name `AXAlloc.c` is a GUESS from the
 *    library's naming.
 * EVIDENCE. `.bss` 0x806F74E0 and 0x806F7560 (0x80 B each: head and tail of 32 priority lists, list 0 is the free
 *    list) are read only by the list helpers, `AXFreeVoice` and `AXAcquireVoice`; `.sbss` 0x80794ED0 (the callback
 *    stack head) is read by the same functions; the next `.bss` object (0x806F75E0) is `__AXAuxInit`'s.
 * RESIDUALS. none recorded yet.
 * SHAPES. doubly linked lists per priority; the list insert is a static inline expanded in `AXAcquireVoice` and
 *    `AXSetVoicePriority`.
 */

#include "types.h"

#include "AX/AXAlloc.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"

#define AX_PRIORITY_COUNT 32

AXVPB* __AXStackTail[AX_PRIORITY_COUNT];
AXVPB* __AXStackHead[AX_PRIORITY_COUNT];
AXVPB* __AXCallbackStack;

/* 0x8046E4F0 (0x14): returns the head voice of a priority list. */
AXVPB* __AXGetStackHead(u32 priority)
{
    return __AXStackHead[priority];
}

/* Pops the next voice off the callback stack, or returns NULL. */
static inline AXVPB* __AXPopCallbackStack(void)
{
    AXVPB* vpb = __AXCallbackStack;

    if (vpb != NULL) {
        __AXCallbackStack = vpb->nextCallback;
    }
    return vpb;
}

/* 0x8046E510 (0xAC): runs the callback of every voice queued for release and returns it to the free list. */
void __AXServiceCallbackStack(void)
{
    AXVPB* vpb;

    vpb = __AXPopCallbackStack();
    while (vpb != NULL) {
        if (vpb->priority != 0) {
            if (vpb->callback != NULL) {
                vpb->callback(vpb);
            }
            __AXRemoveFromStack(vpb);
            vpb->next = __AXStackHead[0];
            __AXStackHead[0] = vpb;
            vpb->priority = 0;
        }
        vpb = __AXPopCallbackStack();
    }
}

/* 0x8046E5C0 (0xB0): empties every priority list and the callback stack. */
void __AXAllocInit(void)
{
    u32 i;

    __AXCallbackStack = NULL;
    for (i = 0; i < AX_PRIORITY_COUNT; i++) {
        __AXStackTail[i] = NULL;
        __AXStackHead[i] = NULL;
    }
}

/* 0x8046E670 (0xB0): empties every priority list and the callback stack. */
void __AXAllocQuit(void)
{
    u32 i;

    __AXCallbackStack = NULL;
    for (i = 0; i < AX_PRIORITY_COUNT; i++) {
        __AXStackTail[i] = NULL;
        __AXStackHead[i] = NULL;
    }
}

/* 0x8046E720 (0x1C): pushes a voice onto the free list. */
void __AXPushFreeStack(AXVPB* vpb)
{
    vpb->next = __AXStackHead[0];
    __AXStackHead[0] = vpb;
    vpb->priority = 0;
}

/* 0x8046E740 (0x10): pushes a voice onto the callback stack. */
void __AXPushCallbackStack(AXVPB* vpb)
{
    vpb->nextCallback = __AXCallbackStack;
    __AXCallbackStack = vpb;
}

/* 0x8046E750 (0x84): unlinks a voice from its priority list. */
void __AXRemoveFromStack(AXVPB* vpb)
{
    u32 priority = vpb->priority;
    AXVPB* head = __AXStackHead[priority];
    AXVPB* tail = __AXStackTail[priority];

    if (head == tail) {
        __AXStackTail[priority] = NULL;
        __AXStackHead[priority] = NULL;
    } else if (vpb == head) {
        __AXStackHead[priority] = vpb->next;
        vpb->next->prev = NULL;
    } else if (vpb == tail) {
        __AXStackTail[priority] = vpb->prev;
        vpb->prev->next = NULL;
    } else {
        AXVPB* prev = vpb->prev;
        AXVPB* next = vpb->next;

        prev->next = next;
        next->prev = prev;
    }
}

/* 0x8046E7E0 (0x7C): releases a voice back to the free list, queueing a depop when its DSP voice is running. */
void AXFreeVoice(AXVPB* vpb)
{
    BOOL level = OSDisableInterrupts();

    __AXRemoveFromStack(vpb);
    if (vpb->pb.running == 1) {
        vpb->depop = 1;
    }
    __AXSetPBDefault(vpb);
    vpb->next = __AXStackHead[0];
    __AXStackHead[0] = vpb;
    vpb->priority = 0;
    OSRestoreInterrupts(level);
}

/* Links a voice at the head of a priority list. */
static inline void __AXAddToStack(AXVPB* vpb, u32 priority)
{
    AXVPB* head = __AXStackHead[priority];

    vpb->next = head;
    vpb->prev = NULL;
    if (head != NULL) {
        __AXStackHead[priority]->prev = vpb;
        __AXStackHead[priority] = vpb;
    } else {
        __AXStackHead[priority] = vpb;
        __AXStackTail[priority] = vpb;
    }
}

/* 0x8046E860 (0x184): takes a free voice, or steals the tail voice of a lower priority list, and installs it. */
AXVPB* AXAcquireVoice(u32 priority, AXVPBCallback callback, u32 userContext)
{
    BOOL level = OSDisableInterrupts();
    AXVPB* vpb = __AXStackHead[0];
    u32 i;

    if (vpb != NULL) {
        __AXStackHead[0] = vpb->next;
    }
    if (vpb == NULL) {
        for (i = 1; i < priority; i++) {
            vpb = NULL;
            if (__AXStackHead[i] != NULL) {
                AXVPB* tail = __AXStackTail[i];

                if (__AXStackHead[i] == tail) {
                    __AXStackTail[i] = NULL;
                    vpb = __AXStackHead[i];
                    __AXStackHead[i] = NULL;
                } else if (tail != NULL) {
                    AXVPB* prev = tail->prev;

                    vpb = tail;
                    __AXStackTail[i] = prev;
                    prev->next = NULL;
                }
            }
            if (vpb != NULL) {
                if (vpb->pb.running == 1) {
                    vpb->depop = 1;
                }
                if (vpb->callback != NULL) {
                    vpb->callback(vpb);
                }
                break;
            }
        }
    }
    if (vpb != NULL) {
        __AXAddToStack(vpb, priority);
        vpb->priority = priority;
        vpb->callback = callback;
        vpb->userContext = userContext;
        __AXSetPBDefault(vpb);
    }
    OSRestoreInterrupts(level);
    return vpb;
}

/* 0x8046E9F0 (0x9C): moves a voice to another priority list. */
void AXSetVoicePriority(AXVPB* vpb, u32 priority)
{
    BOOL level = OSDisableInterrupts();

    __AXRemoveFromStack(vpb);
    __AXAddToStack(vpb, priority);
    vpb->priority = priority;
    OSRestoreInterrupts(level);
}
