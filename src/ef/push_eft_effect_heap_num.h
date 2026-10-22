/* ef/push_eft_effect_heap_num.h - leaf header (docs/plan.md 6.5 rule 2): `ef/eft_res.cpp`'s
 *   `push_eft_effect_heap_num` (0x800F8AB8), for `lobby/fn_802FA9A0.cpp`.  A C++ free function: the map spells it
 *   `push_eft_effect_heap_num__FPPQ34nw4r2ef6Effectl` (rule 9). */
#ifndef MHTRI_EF_PUSH_EFT_EFFECT_HEAP_NUM_H
#define MHTRI_EF_PUSH_EFT_EFFECT_HEAP_NUM_H

#include "types.h"

namespace nw4r { namespace ef { class Effect; } }

/* 0x800F8AB8: returns `count` pooled effects from `effects` to the effect heap. */
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);

#endif /* MHTRI_EF_PUSH_EFT_EFFECT_HEAP_NUM_H */
