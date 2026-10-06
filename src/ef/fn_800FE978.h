#ifndef MHTRI_EF_FN_800FE978_H
#define MHTRI_EF_FN_800FE978_H

#include "types.h"

/* ef/fn_800FE978.h - the declaration of `ef/fn_800FD864_fx.cpp`'s spawn/init state machine `fn_800FE978`
 * (docs/plan.md 6.5 rule 2), against the 0x48-byte effect slot (`struct _EFT` is `ef.h`'s). */
struct _EFT;
/* The owner defines it `extern "C"` (the target references the plain name); a C++ declaration would mangle it
 * (`fn_800FE978__FP4_EFT`) at the consumer. */
#ifdef __cplusplus
extern "C" {
#endif
void fn_800FE978(struct _EFT* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_800FE978_H */
