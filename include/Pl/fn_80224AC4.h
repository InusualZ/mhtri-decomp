/*
 * Types owned by `Pl/fn_80224AC4.cpp` (docs/plan.md 6.5 rule 1).  They were defined in that unit
 * until `Pl/fn_802693C4.cpp` became a second consumer, which is the point the convention moves a
 * shared type to a header.
 *
 * The rig is the object `_PLW.physics_0x13C` points at: `Pl/fn_80224AC4.cpp` walks it as an SE/motion
 * rig, and the 0x802693C4 proposal's part accessors read the same records.  The 0x1560-byte extent is
 * confirmed by the rig's constructor `fn_80269DD8` (`__construct_array` of 7 x 0x90 at +0xE40 and
 * 3 x 0x110 at +0x1230, ending exactly at 0x1560), which still needs the two element types modelled.
 */
#ifndef MHTRI_PL_FN_80224AC4_H
#define MHTRI_PL_FN_80224AC4_H

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
    /* +0x04 */ u8 pad_0x04[0x8C];
} PlSeSlot; /* size: 0x90 */

/* The third array's element: 0x110 B of per-attachment state; only the +0x10 id byte is read here.
 * Its first word is the vtable the constructor `fn_80269E8C` installs (`.data` record lbl_805BAB58).
 * size: 0x110 */
typedef struct PlSeAttach {
    /* +0x000 */ u32* vtable_0x00;
    /* +0x004 */ u8 pad_0x004[0xC];
    /* +0x010 */ u8 field_0x10;
    /* +0x011 */ u8 pad_0x011[0xFF];
} PlSeAttach; /* size: 0x110 */

/* The actor's SE/motion rig: the `_PLW*` at +0, seven model records, the per-model gate bytes, three
 * sub-model records, the seven 0x90-byte slot records and the three attachment records - the offsets
 * `fn_80224AC4` walks and the ones the rig's constructor `fn_80269DD8` lays out (`__construct_array`
 * of 7 x 0x90 at +0xE40 and 3 x 0x110 at +0x1230, ending exactly at 0x1560).
 * size: 0x1560 */
typedef struct PlSeRig {
    /* +0x000 */ _PLW* plw;
    /* +0x004 */ PlSeModel models_0x004[7];
    /* +0x9C0 */ u8 field_0x9C0[7];   /* one gate per entry of `models_0x004` */
    /* +0x9C7 */ u8 field_0x9C7[3];   /* one gate per entry of `sub_0x0A14` */
    /* +0x9CA */ u8 pad_0x9CA[0x4A];
    /* +0xA14 */ PlSeModel sub_0x0A14[3];
    /* +0xE40 */ PlSeSlot slots_0xE40[7];
    /* +0x1230 */ PlSeAttach attachments_0x1230[3];
} PlSeRig; /* size: 0x1560 */

#endif /* MHTRI_PL_FN_80224AC4_H */
