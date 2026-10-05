/*
 * Declarations for the symbols `src/fn_80040598.cpp` owns (docs/plan.md 6.5 rule 2).  Both are the
 * game-root RSO loaders that unit defines: `fn_80040598` loads a module and publishes the pool top the
 * `mode` selects, `CntSdRsoTerminate` is the same load without the pool bookkeeping.  Consumers
 * (`src/mh3_pad.cpp`) include this header instead of re-declaring them.
 */
#ifndef MHTRI_FN_80040598_H
#define MHTRI_FN_80040598_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void* fn_80040598(const char* path, void* buffer, u32 mode);
void* CntSdRsoTerminate(const char* path, void* buffer);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FN_80040598_H */
