/*
 * homebutton/fn_80555374.h - the 0x80555374-0x8055C894 band's entry points that other units call
 * (docs/plan.md 6.5, rule 2: an extern lives with the TU that owns the symbol).
 *
 * `src/homebutton/fn_80555374.cpp` is that band (`.text` 0x80555374-0x8055C894), so every address
 * below is inside its range and its declaration belongs here rather than in a consumer's source.
 * The band's symbols are the map's `fn_XXXXXXXX` stems (its own file header carries the rule-7
 * deferral), so they keep C linkage.
 *
 * Two consumers, merged 2026-09-26 (each side had added the file for its own half): the band below,
 * `homebutton/fn_8054E894.cpp` (`.text` 0x8054E894-0x80555374), calls fn_8055C494 / fn_8055C638 /
 * fn_8055C1D4 / fn_8055C2CC - fn_8055C494 is the band's destructor entry, called with a `0` flag
 * before `__dl__FPv`, and fn_8055C638 re-sorts the widget's list; and `fn_805482CC.cpp`'s trailing
 * helpers tail-call into fn_8055A3E0 / fn_8055BEF0 / fn_8055C1D4 / fn_8055C2CC.
 */
#ifndef MHTRI_HOMEBUTTON_FN_80555374_H
#define MHTRI_HOMEBUTTON_FN_80555374_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* r3 is the address the caller hands over (the +0x1A04 record of the panel at 0x805482CC and the
 * sub-object at its +0x10); `fn_8055A3E0` additionally takes the mode word the caller materialises. */
void fn_8055A3E0(void* sub, s32 mode);
void fn_8055BEF0(void* record);
void fn_8055C1D4(void* record);
void fn_8055C2CC(void* record);
void* fn_8055C494(void* self, int flag);
void fn_8055C638(void* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HOMEBUTTON_FN_80555374_H */
