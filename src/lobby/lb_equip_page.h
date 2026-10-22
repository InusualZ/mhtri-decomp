/* The lobby equipment-page unit `lobby/lb_equip_page.cpp`: the trade-ship screen (lobby screen 8) and the player
 * model rig helpers the NPC control band and the player action unit (`Pl/pl_act.cpp`) reach.  Its consumers include
 * it inside their own namespaces, so the record types are only named in elaborated parameter types (a standalone
 * `struct X;` would declare a new type in the includer's namespace).
 */
#ifndef MHTRI_LOBBY_LB_EQUIP_PAGE_H
#define MHTRI_LOBBY_LB_EQUIP_PAGE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80220114 - counts one voyage down on every trade ship still at sea (GUESS name). */
void lb_ship_voyage_tick(void);
/* 0x80223258 - dyes rig model `slot` with its equipment piece's colour (GUESS name). */
void pl_equip_color_apply(struct _PLW* self, u8 slot);
/* 0x80224884 - whether the player's effects are drawn (0 while the player is hidden, inactive or not yet ready).
 * GUESS name. */
u8 pl_eft_draw_ck(struct _PLW* plw);

/* 0x805BAA90 / 0x805BAC98 - the per-weapon-type attachment kinds and joint-offset tables the player action unit
 * indexes by the weapon type (`_PLW::field_0x002`). */
extern u8 pl_rig_attach_kind_tbl[];
extern u8 pl_rig_pose_drawn[];

#ifdef __cplusplus
}
#endif

/* Declarations `enemy/em020_prog.cpp` calls (its lobby/quest bands). */
#ifdef __cplusplus
struct _PLW;
struct _PLW_PHYSICS;
extern "C" {
/* 0x80222F18 - resets every model of the actor's model rig and binds it to `plw`. */
void pl_physics_rebind(struct _PLW_PHYSICS* physics, struct _PLW* plw);
}
#endif

#endif /* MHTRI_LOBBY_LB_EQUIP_PAGE_H */
