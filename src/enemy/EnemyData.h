/* The per-enemy data record `get_enemy_data` returns, shared by `enemy.h` and the net sync (`hud/pl_frame_sync.cpp`)
 * that reads its kind byte (+0x00F).
 */
#ifndef MHTRI_ENEMY_ENEMYDATA_H
#define MHTRI_ENEMY_ENEMYDATA_H

#include "types.h"

typedef struct EmDataRecord EmDataRecord;   /* enemy/fn_80138074.c */
typedef struct UserDataItem UserDataItem;   /* enemy/fn_80138074.c */
typedef struct EnemyExtraData EnemyExtraData; /* enemy/em_common.cpp */

/* One ailment effect record of the enemy data (`EnemyData::ailment_fx_0x9C`): the joint and the offset.
 * size: 0x14 */
struct EmAilmentFx {
    /* +0x00 */ u32 joint;
    /* +0x04 */ f32 x;
    /* +0x08 */ f32 y;
    /* +0x0C */ f32 z;
    /* +0x10 */ f32 scale;
};

/* The enemy data's ailment effect block (`EnemyData::ailment_fx_0x9C`); `enemy/em_common.cpp` and
 * `enemy/em027_prog.cpp` read it.
 * size: 0x24 (lower bound) */
struct EmAilmentFxBlock {
    /* +0x00 */ f32 base_scale;        /* the effect scale `em_fx_scale_get` returns */
    /* +0x04 */ f32 paralysis_scale;   /* the paralysis spark scale `em_paralysis_fx_scale_get` returns */
    /* +0x08 */ struct EmAilmentFx* poison;
    /* +0x0C */ struct EmAilmentFx* sleep;
    /* +0x10 */ struct EmAilmentFx* stun;
    /* +0x14 */ u8 unused_0x14[0x20 - 0x14];
    /* +0x20 */ f32 hit_scale;         /* the hit effect scale `em_hit_fx_scale_get` returns */
};

/* The per-enemy data record `get_enemy_data` returns.  Union of `enemy/em_common.cpp` and
 * `enemy/fn_80138074.c`; the two views are compatible (a pointer table at +0x04, the value list at
 * +0x64, the user-data items at +0x94 and the extra block at +0xA0). size: 0xA4 */
typedef struct EnemyData {
    union {
        struct { /* size: 0x4 */
            /* +0x000 */ u8 field_0x00;
            /* +0x001 */ u8 pad_0x1[0x3];
        };
        /* +0x000 */ u32 flags_0x00;   /* bit 0 a large monster, copied into `_ENEMY_WORK::field_0x1C8` */
    };
    union {
        /* +0x004 */ EmDataRecord* records;
        /* +0x004 */ u32 model_flags_0x04;  /* the model flags `enemy_work_setup` hands `mhchar_model_create` */
    };
    union {
        struct { /* size: 0x4 */
            /* +0x008 */ u16 field_0x08;
            /* +0x00A */ u16 field_0x0A;
        };
        /* +0x008 */ f32 base_scale_0x08;   /* the base model scale `get_em_chg_scale` divides by (`em_common.cpp`) */
    };
    /* +0x00C */ u8 kind_0x0C;              /* the byte `em_data_kind_get` returns */
    /* +0x00D */ u8 rank_0x0D;              /* the rank `em_get_rank` returns */
    /* +0x00E */ u8 field_0x0E;
    /* +0x00F */ u8 kind_0x0F;      /* the enemy kind the net sync packs special cases for */
    /* +0x010 */ struct EmEntryProcs* entry_procs_0x10;  /* the marker callbacks `em_entry_proc_get` picks (`em_common.cpp`) */
    /* +0x014 */ u32 field_0x14;
    /* +0x018 */ u8 pad_0x18[0x4];
    /* +0x01C */ u32 field_0x1C;
    /* +0x020 */ u32 field_0x20;
    /* +0x024 */ u32 field_0x24;
    /* +0x028 */ u32* words_0x28;        /* the per-index words `em_data_word_get` reads (`em_common.cpp`) */
    /* +0x02C */ u8 pad_0x2C[0x4];
    /* +0x030 */ u32 bits_0x30;     /* copied into `_ENEMY_WORK::bits_0x824` by em_status_bits_init */
    /* +0x034 */ u8* hit_dirs_0x34;  /* per reaction kind, the hit-direction mode and turn mode (`em_hit_dir_get`) */
    /* +0x038 */ struct EmMapEntry* area_maps_0x38;  /* per map, the area reach table (`em_map_table_get`; `em_common.cpp`) */
    /* +0x03C */ struct EmMapEntry* allowed_areas_0x3C;  /* per map, the areas the kind may enter (`em_data_area_allowed_ck`) */
    /* +0x040 */ u8* water_exclude_0x40;  /* `{map kind, area, water area}` triples, closed by 0xFF, that `em_in_water_ck` ignores */
    /* +0x044 */ struct EmMapEntry* area_recs_0x44;  /* per map, the 8-byte area records `em_area_rec_get` returns */
    /* +0x048 */ struct EmRouteMap* routes_0x48;  /* the per-map area routes `em_route_find` walks (`em_common.cpp`) */
    /* +0x04C */ struct EmSpecialPartBlock* special_parts_0x4C;  /* the special-part sets (`em_special_part_map_get`) */
    /* +0x050 */ f32* move_params_0x50;  /* three VEC3 move-mode parameters `em_move_mode_set` copies, plus the dive one at +0x18 */
    /* +0x054 */ u8 pad_0x054[0x058 - 0x054];
    /* +0x058 */ struct EmStatusLimits* status_limits_0x58;  /* the +0x43D status frame limits `em_status_update` reads */
    /* +0x05C */ s32 frames_bias_0x5C;  /* the bias `em_data_frames_get` adds (`em_common.cpp`) */
    /* +0x060 */ u8 pad_0x060[0x064 - 0x060];
    /* +0x064 */ u8* values;
    /* +0x068 */ struct EmHitBuff* hit_buff_0x68;  /* the hit buff `em_hit_buff_apply` reads (`em_common.cpp`) */
    /* +0x06C */ u8 pad_0x06C[0x070 - 0x06C];
    /* +0x070 */ f32 fall_height_0x70;  /* `em_fall_height_get` */
    /* +0x074 */ f32 water_depth_0x74;  /* `em_float_depth_get` */
    /* +0x078 */ f32 lift_height_0x78;  /* `em_dive_depth_get` */
    /* +0x07C */ f32 ceiling_depth_0x7C;  /* `em_ceiling_depth_get` */
    /* +0x080 */ f32 model_height_0x80;  /* `em_model_height_get` (`fn_8012F39C` scales it by 0.9) */
    /* +0x084 */ s16** motion_frames;      /* per hundred motion numbers, a table of s16 frame values (`em_common.cpp`) */
    /* +0x088 */ struct EmPartBreakRec* part_breaks_0x88;  /* the part-break records `em_parts_refresh` walks */
    /* +0x08C */ struct EmMotionAttr* motion_attrs;  /* `{motion, value}` pairs, closed by motion 0xFFFFFFFF */
    /* +0x090 */ struct EmMotionCond* motion_conds;  /* `{motion, kind, mode, arg, level}` gates, closed the same way */
    /* +0x094 */ UserDataItem* items;
    /* +0x098 */ s32* table_0x98;
    /* +0x09C */ struct EmAilmentFxBlock* ailment_fx_0x9C;  /* the poison/sleep effect offsets (`em_common.cpp`) */
    /* +0x0A0 */ EnemyExtraData* extra;
} EnemyData;

#endif /* MHTRI_ENEMY_ENEMYDATA_H */
