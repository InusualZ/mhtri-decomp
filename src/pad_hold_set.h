/* Leaf header (docs/plan.md 6.5 rule 2): symbols `pad_connect.cpp` defines, for a consumer that does not include the
 * owner's header.  C linkage (the map rows are plain names). */
#ifndef MHTRI_PAD_HOLD_SET_H
#define MHTRI_PAD_HOLD_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80046EE4 - sets the pad-hold flag (`system_w.field_0x30 = 1`) that `pad_hold_ck` reads. */
void pad_hold_set(void);
/* 0x80046EF8 - clears that flag. */
void pad_hold_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PAD_HOLD_SET_H */
