/*
 * The 0x100 B VS user block `src/fn_8004CAD8.cpp` owns (the two slots at 0x8066A620/0x8066A720 that `get_vsUser_work`
 * indexes), split out of `fn_8004CAD8.h` so the owner's source can include it without that header's consumer-side
 * declarations.  The block's own name is the mangling's (`ck_mydata_vs__FP13_vs_user_dataP13_vs_user_data`).
 */
#ifndef MHTRI_VS_USER_DATA_H
#define MHTRI_VS_USER_DATA_H

#include "types.h"
#include "id_value.h"   /* IdValue (the item slots' halves) */

#ifdef __cplusplus
/* The hunter name the block carries (copied as one 17-byte record).  size: 0x11 */
struct VsUserName {
    /* +0x00 */ char text_0x00[0x11];
};

/* The VS user block: the hunter's look and name, the VS credit, the hunter's appearance bytes, the 16 item slots, the
 * two per-slot flag runs, the result-screen ready bits, the save's name and player, the options, the seed tying it
 * to its save and the byte-sum checksum.  size: 0x100 */
struct _vs_user_data {
    union {
        /* +0x00 */ u8 cfg_0x00[0x18];  /* the per-index option bytes `get_arena_cfg` reads (the
                                         * `0x803BE30C` band's arena twin of `option_w`) */
        struct {  /* the hunter fields the block is filled with.  size: 0x18 */
            /* +0x00 */ u8 hunter_sex_0x00;     /* the save block's `hunter_sex_0x0000` */
            /* +0x01 */ u8 hunter_face_0x01;
            /* +0x02 */ u8 hunter_hair_0x02;
            /* +0x03 */ VsUserName name_0x03;   /* the hunter name */
            /* +0x14 */ u32 hair_color_0x14;
        };
    };
    /* +0x18 */ s32 point_0x18;      /* the credit `score_add_clamped` accumulates for the player */
    /* +0x1C */ u32 reserved_0x1C;   /* cleared when the block is filled */
    /* +0x20 */ u8 hunter_equip_0x20;
    /* +0x21 */ u8 hunter_voice_0x21;
    /* +0x22 */ u8 face_type_0x22;
    /* +0x23 */ u8 face_eye_0x23;
    /* +0x24 */ u8 appearance_0x24;  /* the save block's `appearance_0x39BA` */
    /* +0x25 */ u8 makeup_0x25;
    /* +0x26 */ u8 makeup_color_0x26;
    /* +0x27 */ u8 save_slot_0x27;   /* the save slot the block was filled from */
    /* +0x28 */ u32 inner_color_0x28;
    /* +0x2C */ union {   /* one run, two views (rule 5) */
        /* +0x2C */ u32 item_0x2C[0x10]; /* the 16 four-byte item slots `item_count_find`/`item_take` walk */
        /* +0x2C */ IdValue item_pairs_0x2C[0x10];  /* the same slots as item and count halves */
    };
    /* +0x6C */ u32 slot_a_0x6C[10]; /* the first of the two per-slot flag runs `multi_box_phase_ck` counts (ten words: `arena_result_next` walks them all) */
    /* +0x94 */ u32 slot_b_0x94[10]; /* its sibling; the two are read as a pair per slot */
    /* +0xBC */ u16 ready_mask_0xBC; /* the per-player ready bits the phase gate tests against 0x380 (the save's event bits) */
    /* +0xBE */ char save_name_0xBE[0x11];  /* the save file's player name */
    /* +0xCF */ u8 player_0xCF;      /* the player's index into `system_w`'s per-player flag runs */
    /* +0xD0 */ u8 option_0xD0[0x1F];  /* the player's copy of `option_w` (31 bytes), restored before a save */
    /* +0xEF */ u8 pad_0xEF;
    /* +0xF0 */ u32 seed_0xF0;       /* the save block's `seed_0x53A8` the block was filled with */
    /* +0xF4 */ u8 pad_0xF4[0xFC - 0xF4];
    /* +0xFC */ u32 checksum_0xFC;     /* the byte sum of +0x00..+0xFB (`vs_user_checksum_set`) */
};
#endif

#endif /* MHTRI_VS_USER_DATA_H */
