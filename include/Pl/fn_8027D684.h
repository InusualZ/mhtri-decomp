/*
 * Declarations owned by `Pl/fn_8027D684.cpp` (docs/plan.md 6.5 rule 2): `fn_8027D738` is defined in
 * that source (`src/Pl/fn_8027D684.cpp:132`) and lives at 0x8027D738, inside the unit's `.text`
 * 0x8027D684-0x802840DC, so its declaration belongs in that unit's header and every consumer includes
 * it.  Added with `menu/menu_item.cpp`, which calls it through `fn_802A5444`/`fn_802A579C`.
 */
#ifndef MHTRI_PL_FN_8027D684_H
#define MHTRI_PL_FN_8027D684_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027D738 - the player work record's "ready" byte (`_PLW` +0x268, gated by +0x655's high bit),
 * read as the boolean the menu's item pick waits on.  The map name is a plain `fn_XXXXXXXX` stem, so
 * the owner defines it `extern "C"` and it is declared at C linkage here.  `u32`, and the owner
 * defines it `u32` too: the item menu's caller compares the result unsigned (`bl fn_8027D738;
 * cmplwi r3,0x1` at 0x802A008C), and an `s32` declaration there compiles to `cmpwi` and costs that
 * function 1.28 points. */
u32 fn_8027D738(struct _PLW* self);

/* 0x8027D76C - the owner's own action-complete hook (`fn_8027D684.cpp:147`, `extern "C" void`), the
 * call the AI band makes once a motion resolves.  Added with `ai/fn_802C474C.cpp`, its first consumer
 * (docs/plan.md 6.5 rule 2). */
void fn_8027D76C(struct _PLW* self);

/* 0x8027FF88 - classify one packed equipment word (`_EQUIP_INDEX` +0x00) and return the number of
 * colour/variant forms it has (0 = none).  Read by the equipment-information screen
 * (`menu/menu_infomation.cpp`), whose header is the first consumer. */
u8 fn_8027FF88(u32 equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_8027D684_H */
