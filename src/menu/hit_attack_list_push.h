/*
 * menu/hit_attack_list_push.h - `menu/menu_item.cpp`'s hit-registry record and the two globals it declares; the
 *   array types the band reads are `Pl/pl_coll.h`'s (rule 2).
 */
#ifndef MHTRI_MENU_HIT_ATTACK_LIST_PUSH_H
#define MHTRI_MENU_HIT_ATTACK_LIST_PUSH_H

#include "types.h"
#include "nw4r/math.h"
#include "Pl/hit_w.h"   /* `_HIT_W` / `HitRegistry`, shared with `menu/menu_item.cpp` (rule 1) */



/* The land record (`.bss` `pl_land_data`, 15 x 0x88) and the 0x3C-byte hit-box record (`.bss`
 * `pl_hit_box`, 10 of them) are declared by their owner's header `Pl/pl_coll.h` (rule 2); the layouts live
 * there. */

/* The 0x14-byte position record `fn_80297BE4` copies, `fn_802977E4` initialises and the joint
 * followers write through `fn_8029971C`/`fn_80299780`.  `fn_802977E4` zeroes the eight header bytes
 * one at a time and then sets the vector, and `fn_80297BE4` copies exactly those fields, which is
 * what fixes the layout.
 * size: 0x14 */
struct PlHitPoint {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u16 field_0x04;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ nw4r::math::VEC3 pos_0x08;
};

#ifdef __cplusplus
extern "C" {
#endif

/* The remaining globals this unit's functions operate on: the hit registry (`.bss` `lbl_806AC8A8`,
 * 0x10, through the `HitRegistry` type above) and the 0x25-byte tile-id table `lbl_805CDC88`
 * (`.data`), both in `menu/menu_item.cpp`'s own sections, declared never defined (playbook 29); the land table `pl_land_data` and the
 * hit-id list `pl_hit_id_list` moved to their owner's header `Pl/pl_coll.h` (rule 2). */
/* The 0x25-byte tile-id table `fn_8029B8F4` indexes (`menu/menu_item.cpp`'s own `.data`). */
extern u8 lbl_805CDC88[];
extern HitRegistry lbl_806AC8A8;

/* The registry helpers the shell pool (`stage/shell.cpp`) drives.  The owner argument is whatever record the hit belongs to (a shell,
 * a player or an enemy, by kind). */
void hit_attack_list_push(_HIT_W* hit);
void hit_body_list_push(_HIT_W* hit);
void hit_source_set(_HIT_W* self, u8 kind, void* owner /* untyped: caller-owned payload - the shell, player or enemy record the kind names */);
void hit_owner_set(_HIT_W* self, u8 kind, void* owner /* untyped: caller-owned payload - the shell, player or enemy record the kind names */);
void hit_id_list_clear(u16* ids, u8* count);
void hit_data_apply(_HIT_W* hit, struct _HIT_DATA* data);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_HIT_ATTACK_LIST_PUSH_H */
