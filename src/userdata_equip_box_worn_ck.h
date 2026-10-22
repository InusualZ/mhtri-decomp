/* Leaf header (docs/plan.md 6.5 rule 2): the equipment-box helpers `userdata_item.cpp` defines and
 * `lobby/lb_screen_step.cpp` calls, which the owner's header does not declare.  C linkage (the map rows are plain
 * names). */
#ifndef MHTRI_USERDATA_EQUIP_BOX_WORN_CK_H
#define MHTRI_USERDATA_EQUIP_BOX_WORN_CK_H

#include "types.h"

struct _PLW;
struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004C038 - dresses the player `plw` in the look and the worn equipment of the user data `userdata`. */
void userdata_pl_look_apply(struct _PLW* plw, u8* userdata);
/* 0x8004C7BC - whether box entry `box` is one of the nine worn pieces. */
u32 userdata_equip_box_worn_ck(u8* userdata, u16 box);
/* 0x8004C660 - overwrites box entry `box` with `equip`. */
void userdata_equip_box_set(u8* userdata, u16 box, struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_EQUIP_BOX_WORN_CK_H */
