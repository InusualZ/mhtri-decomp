/* The quest-note / note-pane record types of `enemy/em_prog_support.cpp` (0x80385A54..0x80385CA0) and of the band at
 * 0x803A3A50 that drives the same 0x1F8-byte records: one definition, included by both.
 */
#ifndef MHTRI_ENEMY_NOTE_WORK_H
#define MHTRI_ENEMY_NOTE_WORK_H

#include "types.h"
#include "nw4r/math.h"
#include "sound/mhchar.h"

struct _QNPC_W;
struct _g3d_work;

/* The +0x110 pointer the pane's scene-model teardown reads: it lives over the `MHchar`'s padding
 * (`MHchar` is 0x140 bytes, `Pl/pl_se_rig.h`'s model record 0x164: this view spans the 0x164),
 * so it is the model view's own field.
 * size: 0x164 */
struct NoteModelView {
    /* +0x000 */ u8 pad_0x000[0x10C];
    /* +0x10C */ _g3d_work* g3d_0x110;
    /* +0x110 */ u8 pad_0x110[0x164 - 0x110];
};

/* The rest position and the two talk bytes after it, as `note_pane_init` resets them through one pointer. */
struct NoteRestBlock {
    /* +0x00 */ nw4r::math::VEC3 pos_0x00;
    /* +0x0C */ u8 talk_step_0x0C;
    /* +0x0D */ u8 field_0x0D;
    /* +0x0E */ u8 pad_0x0E[0x2];
}; /* size: 0x10 */

/* The note-pane object set (the 0x80385xxx half of the band).  Every body at 0x80385A54..0x80385CA0
 * works on ONE 0x1F8-byte record; `note_works` is the retail 5-record array of them and
 * `note_slots` a 3-record slot set.
 * size: 0x1F8 */
struct NoteWork {
    /* +0x000 */ u8 field_0x000;
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 field_0x002;     /* the slot index `note_work_alloc` marks on a free record */
    /* +0x003 */ u8 field_0x003;
    /* +0x004 */ union {
        MHchar model;                   /* the actor model the note pane poses (0x140 of the 0x164) */
        NoteModelView view;             /* the same bytes, read as the +0x110 scene-model pointer */
    };
    /* +0x168 */ u8 field_0x168;
    /* +0x169 */ u8 field_0x169;
    /* +0x16A */ u8 field_0x16A;
    /* +0x16B */ u8 field_0x16B;
    /* +0x16C */ s32 field_0x16C;    /* the per-state countdown `fn_80386028`/`fn_80386160` tick */
    /* +0x170 */ nw4r::math::VEC3 vec_0x170;
    /* +0x17C */ nw4r::math::VEC3 vec_0x17C;
    /* +0x188 */ u32 field_0x188;
    /* +0x18C */ u32 field_0x18C;
    /* +0x190 */ u32 field_0x190;
    /* +0x194 */ u8 map_0x194;      /* the map whose home spot `note_pane_init` places the pane on */
    /* +0x195 */ u8 field_0x195;
    /* +0x196 */ u8 pad_0x196[0x198 - 0x196];
    /* +0x198 */ u32 field_0x198;   /* the note id `note_npc_create` records */
    /* +0x19C */ u8 field_0x19C;
    /* +0x19D */ u8 field_0x19D;
    /* +0x19E */ u8 field_0x19E;
    /* +0x19F */ u8 field_0x19F;
    /* +0x1A0 */ u8 field_0x1A0;
    /* +0x1A1 */ u8 field_0x1A1;
    /* +0x1A2 */ u8 pad_0x1A2[0x1A4 - 0x1A2];
    /* +0x1A4 */ f32 effect_y_ofs_0x1A4; /* added to 5.0 for the y of the joint effects `enemy/em_prog_tail.cpp` places */
    /* +0x1A8 */ u32 field_0x1A8;   /* the position `note_pane_pos_step` (this band) eases +0x18C onto */
    /* +0x1AC */ u8 flag_0x1AC;     /* set by `note_pane_init` */
    /* +0x1AD */ u8 pad_0x1AD[0x1B0 - 0x1AD];
    /* +0x1B0 */ f32 field_0x1B0;   /* the per-state step `fn_80386028`/`fn_803863C8` set */
    /* +0x1B4 */ union {
        struct {
            /* +0x1B4 */ union {
                nw4r::math::VEC3 vec_0x1B4;   /* the rest position `note_pane_init` copies from +0x170 */
                struct {   /* the NPC talk program's view of the same 0x0C bytes */
                    /* +0x1B4 */ u8 pad_0x1B4;
                    /* +0x1B5 */ u8 talk_step_0x1B5;   /* 0 opens the talk, 1/2 wait for the window (`lobby/lb_quest_ui.cpp`) */
                    /* +0x1B6 */ u8 pad_0x1B6[0x1C0 - 0x1B6];
                }; /* size: 0x0C */
            };
            /* +0x1C0 */ u8 field_0x1C0;    /* the talk step `npc_talk_step` switches on */
            /* +0x1C1 */ u8 field_0x1C1;
            /* +0x1C2 */ u8 pad_0x1C2[0x2];
        }; /* size: 0x10 */
        NoteRestBlock rest_0x1B4;   /* the same 0x0E bytes as one block: `note_pane_init` resets it through one pointer */
    };
    /* +0x1C4 */ u8 pad_0x1C4[0x1F4 - 0x1C4];
    /* +0x1F4 */ s32 field_0x1F4;  /* the voice handle `note_npc_create` stores */

    /* The record constructor `__construct_array` runs over `note_works` (`enemy/em_prog_support.cpp`). */
    NoteWork();
};

/* One 0x0C-byte entry of the 3-slot seat set `note_slots`.
 * size: 0x0C */
struct NoteSlot {
    /* +0x00 */ u8 index_0x00;     /* the slot's own index */
    /* +0x01 */ u8 state_0x01;     /* 0 free, 1 registered, 2 resolved */
    /* +0x02 */ u8 kind_0x02;      /* the record kind whose model file it holds (0xFF: none) */
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ s32 handle_0x04;
    /* +0x08 */ s32 handle_0x08;
};

/* The quest-NPC record `qn_get_motion_no` reads (the +0x54 halfword).  The name is the map's own
 * (`qn_get_motion_no__FP7_QNPC_W`), so the parameter type keeps it exact.
 * size: 0x1F8 */
struct _QNPC_W {
    /* +0x000 */ u8 pad_0x000[0x054];
    /* +0x054 */ u16 motion_no_0x54;
    /* +0x056 */ u8 pad_0x056[0x0F6 - 0x056];
    /* +0x0F6 */ u8 field_0xF6;
    /* +0x0F7 */ u8 pad_0x0F7[0x1F8 - 0x0F7];
};

#endif /* MHTRI_ENEMY_NOTE_WORK_H */
