/* The stage loader unit `stage/stg_w.cpp`.
 *
 * Declarations owned by that unit that other translation units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_STAGE_STG_W_H
#define MHTRI_STAGE_STG_W_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B0598 - the stage's area/kind classifier the player act band (`Pl/fn_80273B14.cpp`), the
 * player act helpers (`Pl/pl_act.cpp`) and the stage units read; the owner defines it `extern "C"`
 * at `stage/stg_w.cpp:489`.  The consumer narrows the result to a byte, so the declaration here is
 * `u8` where the definition's own return is `u32`. */
u8 pl_act_kind_get(u8 id);

/* 0x802AFF00 - the number of areas of map `id` (the head byte of its record group; 0 for the unmapped id 0xFF);
 * the map's icon lists are walked at most this far.  GUESS name: read off its one consumer, `hud/cockpit_quest.cpp`. */
u32 stage_map_area_count_get(u8 id);

/* 0x802B0668 - the stage kind of map `mapno` (table lookup; 0xFF maps to itself); the pit map is kind 9. */
u8 stage_map_kind_get(u8 mapno);

/* 0x802B0230 - the stage's screen projection record (two scales and two offsets, `QuestScreen` in
 * `hud/cockpit_quest.h`). */
struct QuestScreen;
const struct QuestScreen* screen_projection_get(void);

/* 0x802B06A0 - the area's entry point `idx` (0 or 1): writes its position and an extra word (the pit map copies
 * the stage record's, the others a fixed table's). */
#ifdef __cplusplus
void stage_area_point_get(u8 area, u8 idx, nw4r::math::VEC3* pos, u32* extra);
#endif

/* 0x802B0688 - the stage resource query the light unit's `camera_kill_cut_start_split` hands a block to; added with
 * the `light/light.cpp` registration (rule 2: this range owns the address).  The owner defines it
 * `extern "C" u32 fn_802B0688(void* self)` at `stage/stg_w.cpp:275`. */
u32 fn_802B0688(void* self);

/* 0x802B0A98 - the stage table selector `enemy/em020_handlers.cpp`'s `em020_area_model_set` calls
 * for the two em020 area models: `idx` indexes the two-byte records at 0x805DEF38, whose bytes are
 * handed to `fn_802B0A3C`, and `value` is stored into the stage work block's +0x2EAC byte
 * (`0x806BB7AC`) once per call.  Added with that registration (rule 2: this range owns the address;
 * the signature is the callee's own body - `clrlwi r3,24` for the index, a `stb` for the value). */
void fn_802B0A98(u8 idx, u8 value);

#ifdef __cplusplus
namespace nw4r { namespace math { struct VEC3; } }
/* 0x802B07B4 - the start position and facing of the stage's area `area` (`quest/quest_entry.cpp`'s warp
 * writes them into the player).  GUESS name from that use. */
void stage_area_start_get(u8 area, nw4r::math::VEC3* pos, s32* angle);
/* 0x802AFD50 - the start position and facing of the stage the quest phase `phase` names.  GUESS name from
 * that use. */
void stage_start_get(u8 phase, nw4r::math::VEC3* pos, s32* angle);
#endif

/* 0x802AEC00 - the stage pack reset `light/light.cpp`'s `fn_802C2314` calls; added with that
 * registration (rule 2: this range owns the address).  The owner's body does not exist yet, so the
 * signature is the call site's view: no arguments, no result. */
void fn_802AEC00(void);

/* 0x802B2E2C - copies the current area's staged colour record out of the stage block and drives it (no arguments). */
void fn_802B2E2C(void);

/* 0x802B4C5C - re-drives every live area object's own per-frame update (no arguments). */
void fn_802B4C5C(void);

/* 0x802AFF38 / 0x802AFFF4 - the water tests for a shell's two area bits; each answers 1 while the point is under
 * water (no arguments, the answer in r3).  `stage_water_enabled_ck` is `s32` because `stage/fn_802B2AA0.h` declares it so. */
s32 stage_water_enabled_ck(void);
u32 stage_water_area_ck(void);

#ifdef __cplusplus
}

/* 0x802AFC84 - the current area number, a C++ free function (the map spells it `get_now_areano__Fv`),
 * so the declaration sits at C++ scope (rule 9).  Every effect setter gates its spawn on it; added
 * with `ef/eft035.cpp` (rule 2: this range owns the address). */
u8 get_now_areano(void);

/* 0x802B0100 - the world-space position of the area-local point `pos` in area `area`, returned by value (the hidden
 * result pointer is `r3`, which is why the map's `get_worldworld_pos__FPQ34nw4r4math4VEC3Uc` - the retail name, carried
 * by the `.sel` export table - spells two parameters for a three-register body). */
nw4r::math::VEC3 get_worldworld_pos(nw4r::math::VEC3* pos, u8 area);
#endif

/* Declarations moved here from `unsplit/stage.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

extern u8 stage_w[]; /* the block itself, viewed as `StageMapView` by a cast (the band's own idiom) */

#ifdef __cplusplus
}
#endif

/* Quest entry points and gates (GUESS names from the quest callers).  0x802AFC94 / 0x802AFE08: entry point
 * `entry` of map `map` - its position (a static VEC3) and its angle; 0x802B20A4 / 0x802B47B0: the stage gates the
 * special quest elements 2 and 3 wait on; 0x802B085C: the stage's own bonus pick for name index `name`. */
#ifdef __cplusplus
extern "C" {
#endif
nw4r::math::VEC3* stage_entry_pos_get(u8 map, u8 entry);
u32 stage_entry_angle_get(u8 map, u8 entry);
u32 stage_gate_a_ck(void);
u32 stage_gate_b_ck(void);
u8 stage_bonus_pick(u8 name);
/* 0x802AD9CC .. 0x802B2318 - the stage work's reset, the area load and entry, the area slot (0, 1, -1), a cell
 * word of the area grid, and the map's resource and object loads (GUESS names). */
void stage_work_init(void);
s32 stage_area_load(u32 mode, u8 map, u8 area);
s32 stage_area_enter(u8 map, u8 area);
s32 stage_area_slot_get(u8 map, u8 area);
u32 stage_cell_get(u8 i, u8 j);
/* The cell word `stage_cell_get` returns, read byte by byte by its callers (0xFA/0xFF/0xFF marks the gallery
 * cell).  size: 0x4 */
typedef union StageCell {
    /* +0x0 */ u32 word;
    /* +0x0 */ u8 bytes[4];
} StageCell;
void stage_map_res_load(u8 map);
void stage_map_obj_load(u8 map);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STG_W_H */
