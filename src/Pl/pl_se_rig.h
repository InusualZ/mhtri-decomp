/*
 * The player's SE/motion rig, the object `_PLW.physics_0x13C` points at: `pl_rig_model_move` (`lobby/lb_equip_page.cpp`'s
 * range) walks it and `Pl/pl_act.cpp`'s part accessors read the same records (rule 1).  The rig's constructor
 * `fn_80269DD8` fixes the 0x1560-byte extent (`__construct_array` of 7 x 0x90 at +0xE40 and 3 x 0x110 at +0x1230).
 */
#ifndef MHTRI_PL_PL_SE_RIG_H
#define MHTRI_PL_PL_SE_RIG_H

#include "types.h"
#include "pl.h"

/* The per-model record the rig's three arrays hold: an `MHchar` plus 0x24 B the model layer uses.
 * size: 0x164 */
typedef struct PlSeModel {
    /* +0x000 */ MHchar model;
    /* +0x140 */ u8 field_0x140[0x24];
} PlSeModel; /* size: 0x164 */

/* One element of the 0xE40 array: a 0x90-byte record whose first word is the vtable the array's
 * constructor `fn_80269DD8` installs through `fn_80269EC8` (`.data` record lbl_805BAB74).
 * size: 0x90 */
typedef struct PlSeSlot {
    /* +0x00 */ u32* vtable_0x00;
    /* +0x04 */ _PLW* plw_0x04;          /* the rig's actor (`pl_rig_slot_bind`) */
    /* +0x08 */ MHchar* body_0x08;       /* the rig's first model, whose joints the slot follows */
    /* +0x0C */ u32 index_0x0C;          /* the slot's model index */
    /* +0x10 */ u32 node_map_0x10[32];   /* per node of the slot's model: the body joint it follows, -1 none */
} PlSeSlot; /* size: 0x90 */

/* One 24-byte vector pair of the rig's pose tables: a translation and a rotation.  size: 0x18 */
typedef struct PlSeVecPair {
    /* +0x00 */ f32 x_0x00;
    /* +0x04 */ f32 y_0x04;
    /* +0x08 */ f32 z_0x08;
    /* +0x0C */ f32 x_0x0C;
    /* +0x10 */ f32 y_0x10;
    /* +0x14 */ f32 z_0x14;
} PlSeVecPair; /* size: 0x18 */

/* One joint adjustment of an attachment: scale, translation and rotation (degrees).  size: 0x24 */
typedef struct PlSeJointAdj {
    /* +0x00 */ VEC3 scale_0x00;
    /* +0x0C */ VEC3 trans_0x0C;
    /* +0x18 */ VEC3 rot_0x18;
} PlSeJointAdj; /* size: 0x24 */

/* The third array's element: 0x110 B of per-attachment state.
 * Its first word is the vtable the constructor `fn_80269E8C` installs (`.data` record lbl_805BAB58).
 * size: 0x110 */
typedef struct PlSeAttach {
    /* +0x000 */ u32* vtable_0x00;
    /* +0x004 */ struct PlSeModel* models_0x04;   /* the model array the attachment's kind follows */
    /* +0x008 */ MHchar* model_0x08;              /* the attachment's own model */
    /* +0x00C */ u32 joint_0x0C;                  /* the body joint the attachment's root follows */
    /* +0x010 */ u8 field_0x10;                   /* the attachment kind (`pl_rig_attach_kind_tbl`) */
    /* +0x011 */ u8 allow_0x11;                   /* the motion state `pl_rig_attach_pose_set` posed it for */
    /* +0x012 */ u8 pad_0x012[0x2];
    /* +0x014 */ PlSeJointAdj adj_0x014[7];
} PlSeAttach; /* size: 0x110 */

/* The actor's SE/motion rig: the `_PLW*` at +0, seven model records, the per-model gate bytes, three
 * sub-model records, the seven 0x90-byte slot records and the three attachment records - the offsets
 * `pl_rig_model_move` walks and the ones the rig's constructor `fn_80269DD8` lays out (`__construct_array`
 * of 7 x 0x90 at +0xE40 and 3 x 0x110 at +0x1230, ending exactly at 0x1560).
 * size: 0x1560 */
typedef struct PlSeRig {
    /* +0x000 */ _PLW* plw;
    /* +0x004 */ PlSeModel models_0x004[7];
    /* +0x9C0 */ u8 field_0x9C0[7];   /* one gate per entry of `models_0x004` */
    /* +0x9C7 */ u8 field_0x9C7[3];   /* one gate per entry of `sub_0x0A14` */
    /* +0x9CA */ u8 sub_kind_0x9CA[3];   /* per sub-model: its attachment kind, 0xFF none */
    /* +0x9CD */ u8 color_mat_0x9CD[7][2];   /* per model: the two dyed materials, 0xFF none */
    /* +0x9DB */ u8 skin_mat_0x9DB[7];   /* per model: the skin-coloured material, 0xFF none */
    /* +0x9E2 */ u8 hair_mat_0x9E2;      /* the hair model's coloured material, 0xFF none */
    /* +0x9E3 */ u8 hair_part_0x9E3[0x16];   /* per hair-model node: its part bit, 0xFF none */
    /* +0x9F9 */ u8 face_part_0x9F9[8];  /* per face-model node: its part bit, 0xFF none */
    /* +0xA01 */ u8 sub_mat_0xA01[3];    /* per sub-model: its dyed material, 0xFF none */
    /* +0xA04 */ u8 pad_0xA04[4];
    /* +0xA08 */ _GXColor sub_color_0xA08[3];   /* per sub-model: the material's own colour */
    /* +0xA14 */ PlSeModel sub_0x0A14[3];
    /* +0xE40 */ PlSeSlot slots_0xE40[7];
    /* +0x1230 */ PlSeAttach attachments_0x1230[3];
} PlSeRig; /* size: 0x1560 */

#endif /* MHTRI_PL_PL_SE_RIG_H */
