/*
 * 0x8004C9A0..0x8004CAD8 (312 B) - the user-data equipment-slot selector.
 *
 * One function. It is handed the game's user-data block, an equipment-pool index and a one-byte out
 * flag, and writes that index into the per-category "selected equipment" slot the record's kind
 * belongs to.  The sibling 0x8004CAD8 (`set_slot_none`) is the exact inverse (it clears the same slots
 * to 0xFFFF); `userdata_equip_box_add_new` allocates the pool record whose index this function stores, and
 * `Gunner_opt_ok_ck` (Pl band, 0x8027F0C4) gates the kind-0xB (bowgun) case.
 *
 * Naming / module - which evidence class decided it
 *   Class 1 fails: the range's `.data` run is the switch jump table `jumptable_80581544` only (no bare
 *     source-file name; the function calls no OSPanic/Panic, so no assert string exists).
 *   Class 2 fails: `tools/symbols/dumpmap.py lookup 0x8004C9A0` answers `zz_004c9a0_` (a placeholder).
 *   Class 3/4 decide it: nothing supports a descriptive name, so the map's `fn_8004C9A0` stem is kept,
 *     and the unit is registered in the game-root `main` lib at `src/` - the link band whose
 *     neighbours are `main.cpp`, `sys_mem.cpp`, `fn_80040598.cpp`, `mh3_pad.cpp`, `fn_80047398.cpp`
 *     and `nw_resource.cpp`.  Its physical neighbours `set_slot_none` (0x8004CAD8) and `get_userdata`
 *     (0x8004D120) are plainly one subsystem (the user-data pool) and belong in the same module.
 *
 * Residual (measured 97.88 %, target 312 B vs ours 308 B, the one instruction):
 *   the pool index is `u32` with an explicit `(u16)` narrowing, which is what makes MWCC emit retail's
 *   `clrlwi r0,r4,16` (the same idiom `enemy/fn_801679B0.cpp` and `ef/fn_800FD864.cpp` record).  The
 *   remaining difference is one prologue instruction: retail materialises the element address
 *   (`add r6,r3,r0; addi r7,r6,0xE00; lbz r0,0(r7)`) while this compiler fuses it into `lbzu r0,0xE00(r7)`.
 *   That is a compiler-build artefact, not a source difference: the target object's `.comment` version
 *   byte is 0x0E and the configured Wii/1.3 `mwcceppc` writes 0x0F (the same 0x0E/0x0F gap
 *   `src/fn_80040598.cpp` records).  Every other instruction is byte-identical.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `tools/symbols/dumpmap.py lookup 0x8004C9A0` -> zz_004c9a0_, and the range's .data run is only the
 * switch jump table, so there is no `__FILE__` string to name the original TU either).
 */
#include "pl.h"

/* The game's user-data block, `get_userdata()` (0x8004D120, unregistered band) returns
 * `system_w.field_0x95C` as this.  Only the fields this unit touches are modelled.  The equipment pool
 * at +0xE00 holds `fn_8004AE08() * 100` records (4/6/8 item boxes x 100); `sound/fn_800D7F54.cpp`'s
 * `_UserData` view reaches +0x3E03, so the block continues past the pool.  The two views should be
 * merged into one header (rule 1 follow-up; see the outbox).
 * size: 0x3380 (at least; the sound view extends it to 0x3E04) */
typedef struct UserData {
    /* +0x000 */ u8 pad_0x000[0x8C];
    /* +0x08C */ u16 equip_sel[9]; /* per-category selected pool index; 0xFFFF = nothing selected */
    /* +0x09E */ u8 pad_0x09E[0xE00 - 0x09E];
    /* +0xE00 */ _EQUIP equip[0x320]; /* the equipment pool `userdata_equip_box_add_new` allocates from */
} UserData;

/* `Gunner_opt_ok_ck` (Pl band, 0x8027F0C4): true when the record is a kind-0xB (bowgun) whose +0x2
 * id is 0x11 or 0x13.  Rule 9: declared through its real signature, never the mangled spelling. */
int Gunner_opt_ok_ck(_EQUIP* equip);

/* Selects `index` into the slot the record's kind belongs to.  Returns 0 when the kind selects
 * nothing (0 or > 15), 1 otherwise; `*out` reports the transition the caller reacts to (1 = a kind-0xB
 * record left the gunner slot, 2 = a kind-0xB record entered it). */
extern "C" int fn_8004C9A0(UserData* self, u32 index, u8* out) {
    *out = 0;
    _EQUIP* equip = &self->equip[(u16)index];

    switch (equip->kind) {
    case 0:
        return 0;
    case 1:
        self->equip_sel[3] = index;
        break;
    case 2:
        self->equip_sel[4] = index;
        break;
    case 3:
        self->equip_sel[5] = index;
        break;
    case 4:
        self->equip_sel[6] = index;
        break;
    case 5:
        self->equip_sel[7] = index;
        break;
    case 6:
        self->equip_sel[8] = index;
        break;
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        if (self->equip[self->equip_sel[0]].kind == 0xb) {
            *out = 1;
        }
        self->equip_sel[0] = index;
        self->equip_sel[1] = 0xffff;
        self->equip_sel[2] = 0xffff;
        break;
    case 11:
        if (self->equip[self->equip_sel[0]].kind != 0xb) {
            *out = 2;
        }
        self->equip_sel[0] = index;
        if (!Gunner_opt_ok_ck(equip)) {
            self->equip_sel[1] = 0xffff;
            self->equip_sel[2] = 0xffff;
        }
        break;
    case 12:
        self->equip_sel[1] = index;
        break;
    case 13:
        self->equip_sel[2] = index;
        break;
    default:
        return 0;
    }
    return 1;
}
