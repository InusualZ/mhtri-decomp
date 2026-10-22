/* The declarations `enemy/em_model.cpp` owns (the roster spawn and model band, 0x803B936C-0x803BE30C); the record
 * types stay in `enemy/em_pop.h`, which this header includes.  The names are GUESSES from the bodies.
 */
#ifndef MHTRI_ENEMY_EM_MODEL_H
#define MHTRI_ENEMY_EM_MODEL_H

#include "types.h"
#include "nw4r/math.h"
#include "enemy/em_pop.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* The one work pointer the whole module goes through (`.sbss` 0x80794C58, defined by `enemy/em_model.cpp`). */
extern EmPopWorkSlot em_pop_w;

/* The roster accessor (0x803BDECC) - the one the table searches go through. */
EmPopRec* em_roster_record_get(u32 index);
/* 0x803B9588 - the per-slot effect/joint binding the range's entry points walk: r3 the work
 * record, r4 the slot index, r5 the slot pointer, r6/r7 two scalars. */
void em_roster_slot_effect_set(struct _ENEMY_WORK* self, u16 index, s32* slot, s32 a, s32 b);
s32 em_roster_record_slot_id_get(u32 index);
u32 em_roster_record_alive_ck(u32 index);
void em_roster_record_pos_set(s32 index, nw4r::math::VEC3* pos, u8 area);
void em_roster_record_release(u32 index);
/* 0x803B99C0 - releases the record's roster entries (+0x888 and the two carried ones), setting them to -1. */
void em_roster_records_release(struct _ENEMY_WORK* self);
EmPopRec* em_roster_record_clear(EmPopRec* rec);
u32 em_roster_sub_record_clear(EmPopSubRec* rec);
EmPopRec* em_roster_free_record_get(void);
EmPopSubRec* em_roster_sub_free_get(void);
void em_roster_record_copy(EmPopRec* dst, const EmPopRec* src);
void em_roster_record_unlink(EmPopRec* rec);
u32 em_roster_record_result_get(const EmPopRec* rec);

/* 0x803BA69C - tests the local slot's move-work sub index.  GUESS name. */
u32 quest_move_area_sub_ck(u8 sub);
/* 0x803BAE80 - the weighted random pick over a 0xFFFF-terminated `EmWeightEntry` table.  GUESS name. */
u16 em_weight_table_pick(const EmWeightEntry* table);

/* 0x803BDF0C / 0x803BDFF4 / 0x803BE08C - the kind searches over the roster: collect the matching indices, and
 * read one record's aim position or +0x005 byte under the same acceptance test.  GUESS names. */
u8 em_roster_kind_collect(u8 kind, u8* out, u8 max);
nw4r::math::VEC3* em_roster_kind_aim_pos_get(u8 index, u8 kind);
s32 em_roster_kind_field5_get(u8 index, u8 kind);

/* 0x803BA5A0 / 0x803BA814 - steps the population work's state, and loads the map's population resources
 * (GUESS names). */
void em_pop_work_step(void);
void em_pop_res_load(u8 map);

/* 0x803BA9B0 - the last live roster record of `area` whose cylinder (`+0x1D0` centre, `+0x1F0` radius, `+0x1F4`
 * height) holds `pos`: its id, or 0xFFFF when none does; the kind, centre, rotation and the `+0x1EC` word go out
 * through the pointers (GUESS name).  The `u16` return is what retail's callers compare (`clrlwi r0,r3,16`). */
struct _CP_VECTOR;
u16 em_roster_area_hit_find(u8 player, u8 area, nw4r::math::VEC3* pos, s16* out_kind, nw4r::math::VEC3* out_pos,
                            struct _CP_VECTOR* out_rot, s32* out_word);

/* 0x803B9BA0 - rolls `percent` against the random draw and spawns the enemy's drop item at `pos` in a free drop
 * slot (GUESS name). */
s32 em_drop_item_spawn(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u8 percent);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x803B9FC8 - resets the enemy model pool (the system core's full reset). */
void em_model_pool_reset(void);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_MODEL_H */
