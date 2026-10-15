/*
 * Declarations for the symbols `src/RSO/runtime.c` owns (docs/plan.md 6.5 rule 2).
 *
 * `fn_804DA7E4` is the owner's `RSOStaticLocateObject` (BOOL fn_804DA7E4(RSOModule*)); a consumer
 * (`src/mh3_pad.cpp`) calls it on a freshly-loaded module and then invokes the module's prolog at
 * +0x24.  The `RSOModule` view here is the partial one the consumers need - the BSS size and the
 * prolog/epilog entry points - with the SDK header's real 0x58-byte size.  Moving the full struct out
 * of `src/RSO/runtime.c` and having it include this header is the follow-up.
 */
#ifndef MHTRI_RSO_RUNTIME_H
#define MHTRI_RSO_RUNTIME_H

#include "types.h"

typedef struct RSOModule {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ u32 bss_size;
    /* +0x20 */ u8 pad_0x20[0x4];
    /* +0x24 */ void (*prolog)(struct RSOModule*);
    /* +0x28 */ void (*epilog)(struct RSOModule*);
    /* +0x2C */ u8 pad_0x2c[0x2C];
} RSOModule; /* size: 0x58 */

#ifdef __cplusplus
extern "C" {
#endif

int fn_804DA7E4(RSOModule* module);

/* 0x804DAE40 (0x70): resets the module queue, locates the static module and queues it. */
BOOL RSOListInit(RSOModule* staticRso);

/* 0x804DAF8C (0x8): locates `newRso`, links it both ways with every queued module and queues it. */
/* untyped: byte range (the module's bss buffer) */
BOOL RSOLinkList(RSOModule* newRso, void* bss);

/* 0x804DAF94 (0xB8): dequeues `oldRso`, unlinks it from every queued module and un-locates it. */
BOOL RSOUnLinkList(RSOModule* oldRso);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RSO_RUNTIME_H */
