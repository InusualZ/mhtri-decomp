/* Leaf header (docs/plan.md 6.5 rule 2): the equipment-record helpers `Pl/pl_act.cpp` defines and
 * `lobby/lb_screen_step.cpp` calls.  C linkage (the map rows are plain names). */
#ifndef MHTRI_PL_ITEM_KIND_NAME_GET_H
#define MHTRI_PL_ITEM_KIND_NAME_GET_H

#include "types.h"

struct _PLW;
struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802754B4 - asks the player's model to be rebuilt (`_PLW` +0x19). */
void pl_model_refresh_req(struct _PLW* plw);
/* 0x8027F32C - sets decoration `deco` in the first free slot that fits it. */
void equip_deco_attach(struct _EQUIP* equip, u16 deco);
/* 0x8027F548 - takes the decoration out of slot `slot`. */
void equip_deco_detach(struct _EQUIP* equip, u16 slot);
/* 0x8027F2D8 - whether decoration `deco` fits the record's free slots. */
s32 equip_deco_fit_ck(struct _EQUIP* equip, u16 deco);
/* 0x8027FF30 - raises a gun part's level by one, up to its cap. */
void equip_gun_level_up(struct _EQUIP* equip);
/* 0x8027FC84 - raises an armour piece's level by one, up to its cap. */
void equip_armor_level_up(struct _EQUIP* equip);
/* 0x8027E404 - the upgrade stage of the record at level `level` (0xFF: its own level). */
u8 equip_level_stage_get(struct _EQUIP* equip, u8 level);
/* 0x8027EB18 - the name string of skill kind `kind` (also declared by `Pl/pl_act.h`, which cannot be included beside
 * `Pl/pl_net_slot_bind.h`). */
u32 item_kind_name_get(u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_ITEM_KIND_NAME_GET_H */
