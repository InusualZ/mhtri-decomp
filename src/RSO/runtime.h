/*
 * Declarations for the symbols `src/RSO/runtime.c` owns (docs/plan.md 6.5 rule 2).
 *
 * `fn_804DA7E4` is the owner's `RSOStaticLocateObject` (BOOL fn_804DA7E4(RSOModule*)); a consumer
 * (`src/mh3_pad.cpp`) calls it on a freshly-loaded module and then invokes the module's prolog at
 * +0x24.  The `RSOModule` view here is the partial one that consumer needs - the prolog/epilog entry
 * points - with the SDK header's real 0x58-byte size.  Moving the full struct out of
 * `src/RSO/runtime.c` and having it include this header is the follow-up.
 */
#ifndef MHTRI_RSO_RUNTIME_H
#define MHTRI_RSO_RUNTIME_H

#include "types.h"

typedef struct RSOModule {
    /* +0x00 */ u8 pad_0x00[0x24];
    /* +0x24 */ void (*prolog)(struct RSOModule*);
    /* +0x28 */ void (*epilog)(struct RSOModule*);
    /* +0x2C */ u8 pad_0x2c[0x2C];
} RSOModule; /* size: 0x58 */

#ifdef __cplusplus
extern "C" {
#endif

int fn_804DA7E4(RSOModule* module);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RSO_RUNTIME_H */
