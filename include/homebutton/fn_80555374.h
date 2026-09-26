/*
 * homebutton/fn_80555374.h - the 0x80555374-0x8055C894 band's entry points that other units call
 * (docs/plan.md 6.5, rule 2: an extern lives with the TU that owns the symbol).
 *
 * `src/homebutton/fn_80555374.cpp` is that band (registered, `.text` 0x80555374-0x8055C894), so the
 * four addresses below - 0x8055A3E0, 0x8055BEF0, 0x8055C1D4 and 0x8055C2CC - are inside its range and
 * their declarations belong here rather than in a consumer's source.  The band's symbols are the map's
 * `fn_XXXXXXXX` stems (its own file header carries the rule-7 deferral), so they keep C linkage.
 *
 * Added with the `fn_805482CC.cpp` registration, whose trailing helpers tail-call into this band.
 */
#ifndef MHTRI_HOMEBUTTON_FN_80555374_H
#define MHTRI_HOMEBUTTON_FN_80555374_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* r3 is the address the caller hands over (the +0x1A04 record of the panel at 0x805482CC and the
 * sub-object at its +0x10); `fn_8055A3E0` additionally takes the mode word the caller materialises. */
void fn_8055BEF0(void* record);
void fn_8055C1D4(void* record);
void fn_8055C2CC(void* record);
void fn_8055A3E0(void* sub, s32 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HOMEBUTTON_FN_80555374_H */
