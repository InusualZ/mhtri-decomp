/* The entry points of `Pl/pl_act_step.cpp` (0x802430E8-0x802673A4) its per-frame control cluster calls, then the callees
 * it calls whose owner's header does not declare them (or declares a different signature; the shapes here are the
 * call sites').  Moving each to its owner's header is open rule-2 work.
 */
#ifndef MHTRI_PL_FN_8025F088_H
#define MHTRI_PL_FN_8025F088_H

#include "types.h"
#include "nw4r/math.h"
/* `pl_model_state_set` (0x80267270, `Pl/pl_act_step.cpp`) comes from here, `void` like its definition. */
#include "Pl/fn_80262940.h"
#include "hud/Pl_net_send.h" /* the owner's leaf header (rule 2) */
#include "fn_8004CAD8/item_se_ck.h" /* the owner's leaf header (rule 2) */

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* ---- this unit's own entry points (unmangled `fn_*` stems, so C linkage - playbook 42/48) ---- */
void fn_8025F088(struct _PLW* self);
void fn_8025F478(struct _PLW* self);
s32 fn_8025F588(struct _PLW* self);
void fn_8025FA00(struct _PLW* self);
s32 fn_8025FF0C(struct _PLW* self, u8 mode);
s32 fn_80260198(struct _PLW* self);
void fn_80260248(struct _PLW* self, s32 mode, s16 delta);
void fn_802602A0(struct _PLW* self);
s32 fn_8026077C(u8 a, u8 b);
void fn_802607C4(struct _PLW* self);
void fn_8026099C(struct _PLW* self);
s16 fn_80260A18(struct _PLW* self);
s32 fn_80260A58(struct _PLW* self);
void pl_act_status_step(struct _PLW* self);
void fn_80261770(struct _PLW* self);
void fn_802621B0(struct _PLW* self);
s32 fn_80262688(struct _PLW* self);
void fn_8025E0C8(struct _PLW* self);
void fn_8025E448(struct _PLW* self);
void fn_8025EC58(struct _PLW* self);
void fn_8025ED00(struct _PLW* self);
void fn_802642D0(struct _PLW* self);
void fn_8024676C(struct _PLW* self);
void fn_8025DE38(struct _PLW* self);
u32 fn_8025E298(struct _PLW* self, s32 a, s32 b);

/* ---- Pl-band callees with no registered owner (`unsplit/Pl.h`'s home) ---- */
u32 fn_80276800(struct _PLW* self, s32 v);
u32 fn_80278C7C(struct _PLW* self);
u32 fn_80278CD0(struct _PLW* self);
s32 pl_ground_check(struct _PLW* self, void* a, void* b, f32* out, s32 slot);
u32 pl_coll_shape_set(struct _PLW* self, u8 mode, u16 flags);
u32 fn_8027DCA8(struct _PLW* self);

/* ---- callees another registered unit owns whose header does not declare them ---- */
u32 em_land_data_init(void* out);
u32 em_player_spotted_set(u8 index);
u32 pl_rig_model_move(void* physics);
u32 fn_80262940(struct _PLW* self);
u32 fn_802657F8(struct _PLW* self);
u32 fn_8026F7B4(void);
u32 pl_item_page2_ck(struct _PLW* self);
u32 pl_gauge_step_a(struct _PLW* self);
u32 pl_gauge_step_b(struct _PLW* self);
u32 pl_condition_step(struct _PLW* self);
u32 fn_80277EC0(struct _PLW* self);
u32 fn_80278578(struct _PLW* self, s32 v);
u32 fn_80278D1C(struct _PLW* self);
u32 fn_80279C20(struct _PLW* self);
u32 fn_8027AF34(struct _PLW* self);
s32 pl_item_room_get(struct _PLW* self, u16 item_id, s16 value);

/* ---- library callees (the owners' headers do not declare these) ---- */
u32 calc_xz_angle(f32 a, f32 b, f32 c, f32 d);
u32 vec3_move_damp(f32 a, void* b, void* c, void* d, void* e);
u32 mhchar_model_mtx_update(void* p);
u32 event_demo_running_ck(void);
u32 se_slot_req(s32 value);
u32 pl_mode8_alive_ck(void* p);

/* ---- mangled callees at C++ scope, so the front-end reproduces the map's spelling (rule 9) ---- */
#ifdef __cplusplus
}
void* get_move_work_adrs(u8 kind);
u8* GetItemData(u16 item_id); /* -> GetItemData__FUs */
u32 get_move_work_max(u8 kind);
u32 Pl_suimen_ck(struct _PLW* self); /* -> Pl_suimen_ck__FP4_PLW */
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_8025F088_H */
