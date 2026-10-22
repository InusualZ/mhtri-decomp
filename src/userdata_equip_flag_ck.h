/* Leaf header (docs/plan.md 6.5 rule 2): the per-equipment flag bits `fn_8004CAD8.cpp` defines (user data +0x4B50,
 * one byte per kind and item) and `lobby/lb_screen_step.cpp` calls.  C linkage (the map rows are plain names). */
#ifndef MHTRI_USERDATA_EQUIP_FLAG_CK_H
#define MHTRI_USERDATA_EQUIP_FLAG_CK_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004D45C - whether flag bit `bit` of item `item` of kind `kind` is set. */
u32 userdata_equip_flag_ck(u8 kind, u16 item, u16 bit);
/* 0x8004D378 - sets flag bit `bit` of item `item` of kind `kind`. */
void userdata_equip_flag_set(u8 kind, u16 item, u16 bit);
/* 0x8004D3E8 - clears flag bit `bit` of item `item` of kind `kind`. */
void userdata_equip_flag_clear(u8 kind, u16 item, u16 bit);
/* 0x8004D444 - clears flag bit `bit` of the equipment record's item. */
void userdata_equip_rec_flag_clear(struct _EQUIP* equip, u16 bit);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_EQUIP_FLAG_CK_H */
