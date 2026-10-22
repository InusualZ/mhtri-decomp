/* Pl/pl_status_sheet_get.h - leaf header for the equipment-section getters `src/Pl/pl_act.cpp` defines with C linkage
 * that the equipment-information screen (`menu/menu_infomation.cpp`) calls (docs/plan.md 6.5 rule 2).  A leaf because
 * `Pl/pl_act.cpp` defines them with untyped record parameters; the records they return are in `Pl/fn_8027D684.h`. */
#ifndef MHTRI_PL_PL_STATUS_SHEET_GET_H
#define MHTRI_PL_PL_STATUS_SHEET_GET_H

#include "types.h"
#include "Pl/fn_8027D684.h"

struct _PLW;
struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80279360 - the weapon row's ammo entry for one ammo kind (its +0x18..+0x1B list), or NULL.  GUESS name. */
struct GunAmmoEntry* gun_ammo_entry_find(PlWeaponRec* rec, u8 ammo);

/* 0x80270F50 - fills the hunter's status sheet, first wearing the nine pieces of `columns` when it is given (and
 * restoring the worn set afterwards).  GUESS name. */
void pl_status_sheet_get(struct _PLW* plw, struct _EQUIP* columns, PlStatusSheet* sheet);

/* 0x80271674 - whether a decoration skill applies to the hunter's weapon class (0 for a skill of the other
 * class's list).  GUESS name. */
u32 pl_skill_valid_ck(struct _PLW* plw, u8 skill);

/* 0x8027176C - whether a skill applies to the hunter for a piece's weapon class (`pl_skill_valid_ck` keyed by the
 * piece's kind).  GUESS name. */
u32 pl_equip_skill_valid_ck(struct _PLW* plw, struct _EQUIP* equip, struct _EQUIP* barrel, u8 skill);

/* 0x802719B8 - `pl_equip_skill_valid_ck` for an active skill id (mapped to its tree by `pl_skill_tree_get`).  GUESS name. */
u32 pl_equip_skill_id_valid_ck(struct _PLW* plw, struct _EQUIP* equip, struct _EQUIP* barrel, u16 skill_id);

/* 0x80271E0C - whether a piece adds points to a skill tree (its row or a decoration).  GUESS name. */
u32 equip_skill_tree_has_ck(struct _EQUIP* equip, u8 tree);

/* 0x80272084 - the torso's points in a skill tree multiplied by the column array's torso-up pieces.  GUESS name. */
s8 equip_columns_torso_points_get(struct _EQUIP* columns, u8 tree);

/* 0x80272218 - whether any piece of a column array adds points to a skill tree.  GUESS name. */
u32 equip_columns_tree_has_ck(struct _EQUIP* columns, u8 tree);

/* 0x80271A18 - whether one of the hunter's bonus skills (`_PLW` +0x612) applies to the hunter's weapon class.
 * GUESS name. */
u32 pl_bonus_skill_valid_ck(struct _PLW* plw, u16 skill);

/* 0x80271BD4 - the points a piece adds to one skill (its own row and its decorations).  GUESS name. */
s8 equip_skill_points_get(struct _EQUIP* equip, u8 skill);

/* 0x80271978 - whether an active skill id applies to the hunter's weapon class.  GUESS name. */
u32 pl_skill_id_valid_ck(struct _PLW* plw, u16 skill_id);

/* 0x8027234C - the points a column array of equipment adds up to in one skill tree.  GUESS name. */
s8 equip_columns_tree_points_get(struct _EQUIP* columns, u8 tree);

/* 0x8027243C - the points the hunter's worn equipment adds up to in one skill tree.  GUESS name. */
s8 pl_skill_tree_points_get(struct _PLW* plw, u8 tree);

/* 0x8027252C - the skills a column array of equipment activates: up to eight skill ids, their point levels and
 * their trees.  GUESS name. */
void equip_columns_skills_get(struct _EQUIP* columns, u16* ids, s8* levels, u8* trees);

/* 0x8027E29C - the weapon class index of an equipment kind (kind - 7).  GUESS name. */
u8 weapon_class_index(u8 kind);

/* 0x8027EA84 - the description text of a piece (its kind's text table at the piece's id).  GUESS name. */
u32 equip_info_text_get(u8 kind, u16 id);

/* 0x8027FF20 - the highest upgrade level a bowgun frame can reach (`fn_8027FE50` on its kind and id).  GUESS name. */
u8 gun_frame_level_cap_get(struct _EQUIP* equip);

/* 0x8027F5C4 - the decoration ids a piece carries (up to three into `ids`; `by_slot` 0 repeats each by its slot
 * size).  GUESS name. */
void equip_deco_ids_get(struct _EQUIP* equip, u16* ids, s32 by_slot);

/* 0x8027FC70 - the highest upgrade level a piece can reach (`fn_8027FB84` on its kind, id and level).  GUESS name. */
u8 equip_level_cap_get(struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_STATUS_SHEET_GET_H */
