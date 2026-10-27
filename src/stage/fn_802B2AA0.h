/* The `stage` band unit `stage/stage_light_blend_set.cpp` - its types, its own entry points and the
 * declarations of the symbols it calls (docs/plan.md 6.5 rules 1-5).
 *
 * `.text` 0x802B2AA0-0x802B5C58.  The unit's own notes, extent, flag evidence and residuals
 * live in the source file's header comment.
 */
#ifndef MHTRI_STAGE_FN_802B2AA0_H
#define MHTRI_STAGE_FN_802B2AA0_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "sound/mhchar.h"
#include "sound/fn_800D7F54.h"
#include "unsplit/ef.h"
#include "unsplit/g3d.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "Pl/Pl_master_ck.h"
#include "fn_8004CAD8.h"
#include "ef/fn_800CDB2C.h"
#include "stage/shell_set_func_ptr.h" /* `ShellSetFuncs` and the pointer this unit defines */

/* The band's `#pragma peephole off` is NOT here: a codegen pragma in a shared header leaks into every
 * including TU, so a lane matched a file only because of the leak and another lost rows until the
 * pragma was restated.  It belongs to the `.c`/`.cpp` that measured the dependency, and every including
 * TU that depends on it now states it there itself: `stage/stage_light_blend_set.cpp`, `enemy/em019_prog.cpp`,
 * `enemy/em020_ai.cpp` and `enemy/em020_prog.cpp`.  `tools/units/stylelint.py` rule 10 fails a codegen
 * pragma in a shared header. */

/* ------------------------------------------------------------------------------------------------
 * the shared types of the band
 * ------------------------------------------------------------------------------------------------ */

/* The `stage_w` block itself is `StageWork` (`stage/stage_work.h`). */

/* One 0x4F8-byte per-area runtime object (two of them at `lbl_806BB7E0`). */
/* size: 0x4F8 */
typedef struct StageAreaObj {
    /* +0x000 */ nw4r::math::VEC3 seat_pos;
    /* +0x00C */ nw4r::math::VEC3 seat_pos2;
    /* +0x018 */ u8 pad_0x018[0x018];
    /* +0x030 */ f32 pitch;
    /* +0x034 */ f32 span;
    /* +0x038 */ u8 pad_0x038[0x02C];
    /* +0x064 */ u32 ang_x; /* calcVecAngXY's first output */
    /* +0x068 */ u32 ang_y; /* calcVecAngXY's second output */
    /* +0x06C */ s32 field_0x06C;
    /* +0x070 */ s32 field_0x070;
    /* +0x074 */ s32 field_0x074;
    /* +0x078 */ s32 field_0x078;
    /* +0x07C */ u8 field_0x07C; /* the camera mode `get_cfg` selects */
    /* +0x07D */ u8 pad_0x07D[3];
    /* +0x080 */ s32 field_0x080;
    /* +0x084 */ u8 field_0x084;
    /* +0x085 */ u8 pad_0x085[0x00D];
    /* +0x092 */ u8 field_0x092;
    /* +0x093 */ u8 pad_0x093[0x00B];
    /* +0x09E */ u8 field_0x09E;
    /* +0x09F */ u8 field_0x09F;
    /* +0x0A0 */ u8 field_0x0A0;
    /* +0x0A1 */ u8 field_0x0A1;
    /* +0x0A2 */ u8 field_0x0A2;
    /* +0x0A3 */ u8 field_0x0A3;
    /* +0x0A4 */ u8 field_0x0A4;
    /* +0x0A5 */ u8 field_0x0A5;
    /* +0x0A6 */ u8 field_0x0A6;
    /* +0x0A7 */ u8 pad_0x0A7;
    /* +0x0A8 */ u16 field_0x0A8;
    /* +0x0AA */ u8 field_0x0AA;
    /* +0x0AB */ u8 field_0x0AB;
    /* +0x0AC */ u8 field_0x0AC;
    /* +0x0AD */ u8 field_0x0AD;
    /* +0x0AE */ u16 field_0x0AE;
    /* +0x0B0 */ u8 field_0x0B0;
    /* +0x0B1 */ u8 pad_0x0B1;
    /* +0x0B2 */ u16 field_0x0B2;
    /* +0x0B4 */ u8 field_0x0B4;
    /* +0x0B5 */ u8 field_0x0B5;
    /* +0x0B6 */ u8 field_0x0B6;
    /* +0x0B7 */ u8 field_0x0B7;
    /* +0x0B8 */ nw4r::math::VEC3 field_0x0B8;
    /* +0x0C4 */ nw4r::math::VEC3 field_0x0C4;
    /* +0x0D0 */ nw4r::math::VEC3 field_0x0D0;
    /* +0x0DC */ nw4r::math::VEC3 field_0x0DC;
    /* +0x0E8 */ nw4r::math::VEC3 field_0x0E8;
    /* +0x0F4 */ nw4r::math::VEC3 field_0x0F4;
    /* +0x100 */ u8 field_0x100[4];
    /* +0x104 */ u8 pad_0x104[8];
    /* +0x10C */ u8 field_0x10C;
    /* +0x10D */ u8 field_0x10D;
    /* +0x10E */ u8 field_0x10E;
    /* +0x10F */ u8 field_0x10F;
    /* +0x110 */ u16 field_0x110;
    /* +0x112 */ u8 pad_0x112[2];
    /* +0x114 */ u32 field_0x114;
    /* +0x118 */ u8 field_0x118;
    /* +0x119 */ u8 pad_0x119;
    /* +0x11A */ u16 field_0x11A;
    /* +0x11C */ u16 field_0x11C;
    /* +0x11E */ u8 pad_0x11E[0x26];
    /* +0x144 */ u8 field_0x144;
    /* +0x145 */ u8 field_0x145;
    /* +0x146 */ u8 pad_0x146[0x042];
    /* +0x188 */ u8 field_0x188; /* this area's stage-state byte */
    /* +0x189 */ u8 pad_0x189[0x0A7];
    /* +0x230 */ u8 field_0x230;
    /* +0x231 */ u8 pad_0x231[0x053];
    /* +0x284 */ u8 field_0x284;
    /* +0x285 */ u8 pad_0x285[0x103];
    /* +0x388 */ u8 field_0x388;
    /* +0x389 */ u8 pad_0x389[0x0B3];
    /* +0x43C */ u8 field_0x43C;
    /* +0x43D */ u8 pad_0x43D[0x057];
    /* +0x494 */ u32 field_0x494; /* the area's 0xB20-byte move work */
    /* +0x498 */ u8 field_0x498; /* kind index into `get_cfg`'s table */
    /* +0x499 */ u8 field_0x499;
    /* +0x49A */ u8 field_0x49A;
    /* +0x49B */ u8 field_0x49B; /* the resolved kind, 0xFF while unresolved */
    /* +0x49C */ u8 pad_0x49C[0x18];
    /* +0x4B4 */ u8 field_0x4B4;
    /* +0x4B5 */ u8 pad_0x4B5[0x13];
    /* +0x4C8 */ u8 field_0x4C8;
    /* +0x4C9 */ u8 pad_0x4C9[0x13];
    /* +0x4DC */ u8 field_0x4DC;
    /* +0x4DD */ u8 pad_0x4DD[7];
    /* +0x4E4 */ u32 field_0x4E4;
    /* +0x4E8 */ u8 field_0x4E8;
    /* +0x4E9 */ u8 pad_0x4E9[7];
    /* +0x4F0 */ u32 field_0x4F0;
    /* +0x4F4 */ u16 field_0x4F4;
    /* +0x4F6 */ u8 pad_0x4F6;
    /* +0x4F7 */ u8 field_0x4F7;
} StageAreaObj;

/* One 0x10-byte record of the area blend table `stage_blend_table_get` picks and `stage_colour_cycle_weight_get` reads. */
/* size: 0x10 */
/* The two joint-id lists one map/area entry holds: the ids to show and the ids to hide. */
/* size: 0x08 */
typedef struct StageJointLists {
    /* +0x00 */ u8* show;
    /* +0x04 */ u8* hide;
} StageJointLists;

/* One 8-byte entry of the map's demo-marker table: the map it belongs to, how many offsets it has and
 * the offset table itself. */
/* size: 0x08 */
typedef struct StageDemoEntry {
    /* +0x00 */ u8 mapno;
    /* +0x01 */ u8 count;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ u8* offsets;
} StageDemoEntry;

/* size: 0x10 */
typedef struct StageBlendEntry {
    /* +0x00 */ u32 kind;
    /* +0x04 */ u32 colour_a;
    /* +0x08 */ u32 colour_b;
    /* +0x0C */ u32 colour_c;
} StageBlendEntry;

/* The colour blend source `stage_light_blend_set` reads: three 4-word groups, each a base colour, an axis
 * colour and the two targets the base blends towards. */
/* size: 0x10 */
typedef struct StageBlendGroup {
    /* +0x00 */ u32 base;
    /* +0x04 */ u32 axis;
    /* +0x08 */ u32 target_a;
    /* +0x0C */ u32 target_b;
} StageBlendGroup;

/* size: 0x38 */
typedef struct StageBlendRec {
    /* +0x00 */ u8 enabled;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ f32 se_frame;
    /* +0x08 */ StageBlendGroup group[3];
} StageBlendRec;

#include "g3d/g3d_scnroot.h" /* `nw4r::g3d::ScnRoot`, the scene root the two camera calls go through */

/* ------------------------------------------------------------------------------------------------
 * the block symbols
 * ------------------------------------------------------------------------------------------------ */

#include "unsplit/stage.h" /* `StageMapView`, the two-byte view of `stage_w` a `pl.h` consumer can name */
extern "C" u8 camera_work[];

/* ------------------------------------------------------------------------------------------------
 * the callees (C linkage: the map spells every one of these plainly)
 * ------------------------------------------------------------------------------------------------ */

extern "C" s32 screen_split_mode_ck(void);
/* 0x8004723C is `mh3_pad.cpp`'s; spelled as its owner's header does (this call site passes `s32*`,
 * `camera/fn_802B5C58.cpp` a `void**`, hence the erased types). */
extern "C" void* word_copy_return_dst(void* out, const void* src);
extern "C" void scene_current_camera_set(u8 value);
extern "C" void filter_panel_upper_set(s32 a, s32 count, u32* colours, f32 param);
extern "C" void filter_panel_lower_set(s32 a, s32 count, u32* colours, f32 param);
extern "C" void camera_posture_info_ctor(s32* out);
extern "C" s32 fn_8007A1B8(const void* a, s32 b, void* c, void* d, s32 e, s32 f, void* g);
extern "C" void my_player_no_set(s8 value);  /* s8 is the owner's spelling (the argument
 * narrows with `extsb` in retail); `ef/fn_800CDB2C.h` declares it the same way. */
extern "C" void fn_800DD38C(void);
extern "C" u32 mhchar_frame_check(void* chr, s32 a, s32 b, f32 c, f32 d);
extern "C" void eft_capsule_ctor(nw4r::math::VEC3* out);
extern "C" void eft_vec_ctor(nw4r::math::VEC3* out);
extern "C" void mhchar_construct(void* out);
extern "C" f32 lb_item_list_step_get(void);  /* f32, the owner's return type (`lobby/lb_menu_pos_tbl.cpp`): retail converts it with `__cvt_fp2unsigned` */
extern "C" f32 lb_item_list_depth_get(u32 index);
extern "C" s32 getItemListSelection(void);
extern "C" u32 Pl_motion_input_ck(void* work);
extern "C" s32 stage_water_surface_get(void* a, u8 area, s32 handle, void* out, s32 limit);
extern "C" void stage_fog_copy(void* out, const void* in);
extern "C" s32 stage_water_enabled_ck(void);
extern "C" u8* stage_area_word_get(u8 kind);
extern "C" void stage_area_colour_update(void);
extern "C" u8 lb_unlock_cond5_get(void);
extern "C" u8 lb_world_upgrade_get(s32 index);
extern "C" u8 lb_unlock_cond22_flag_ck(void);
extern "C" u8 lb_event_state_ck(void);
extern "C" u32 lb_event_gate2_ck(void);
extern "C" s32 stage_colour_blend(u32 from, u32 to, f32 t);
extern "C" s32 stage_torch_unlit_ck(u8 index);
extern "C" s32 camera_angle_y_get(void);
extern "C" void stage_area_event_dispatch(void* plw, u8 index);
extern "C" f32 stage_colour_cycle_weight_get(s16 index, f32 span);
extern "C" StageBlendEntry* stage_blend_table_get(void);
extern "C" void camera_work_setup(StageAreaObj* area, u8 index);
extern "C" void camera_scene_posture_init(u8 index, s32 camera);
extern "C" void camera_water_cell_update(StageAreaObj* area);
extern "C" void camera_follow_reset(StageAreaObj* area);
extern "C" void camera_follow_update(StageAreaObj* area);
extern "C" s32 camera_pad_state_clear(StageAreaObj* area);
extern "C" s32 camera_pad_state_get(StageAreaObj* area);
extern "C" s32 camera_aim_reset(StageAreaObj* area);
extern "C" s32 camera_aim_update(StageAreaObj* area);
extern "C" s32 camera_shoulder_update(StageAreaObj* area);
extern "C" s32 camera_area_cam_reset(StageAreaObj* area);
extern "C" s32 camera_area_cam_update(StageAreaObj* area);
extern "C" s32 camera_lobby_reset(StageAreaObj* area);
extern "C" s32 camera_lobby_frame(StageAreaObj* area);
extern "C" s32 camera_event_reset(StageAreaObj* area);
extern "C" s32 camera_event_update(StageAreaObj* area);
extern "C" void camera_dcm_buffers_clear(void);
extern "C" void get_camera_target_pos(nw4r::math::VEC3* out);
extern "C" u32 camera_underwater_ck(void);
extern "C" void camera_view_apply(s32 a, StageAreaObj* area, void* vec, f32 pitch, f32 span);
extern "C" void camera_view_angles_update(StageAreaObj* area);
extern "C" void camera_quake_req(s32 kind);
extern "C" void camera_quake_update(StageAreaObj* area, u8 kind);
extern "C" void light_dir_set(nw4r::math::VEC3* vec);
extern "C" void eft038_set(void);
extern "C" void lb_sub12_send(u8 index);
extern "C" u8 get_cfg(u8 kind, u8 mode);

/* C++ free functions whose map names are manglings (rule 9). */
u32 LbCheckKujiraEvent(void);
nw4r::math::VEC3 get_camera_pos(void);
void eft028_set_koware(u8 kind, nw4r::math::VEC3* pos, u8 area, long param);

/* ------------------------------------------------------------------------------------------------
 * the unit's own entry points (rule 2: other units include this header, not a local copy)
 * ------------------------------------------------------------------------------------------------ */

extern "C" s32 stage_oneshot_clear_ck(void);
extern "C" void stage_area_timer_arm(u8 index);

#endif /* MHTRI_STAGE_FN_802B2AA0_H */
