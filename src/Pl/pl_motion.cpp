/*
 * Pl/pl_motion.cpp - the player actor's motion layer: the per-frame motion step, the root-mode accessors and the area/root entry
 * lookups.
 *
 * `.text` 0x80288CEC..0x8028F44C (74 functions), `.bss` 0x18 B, `.ctors` 4 B, `.data` 0x1C84 B, `.sbss` 0x20 B, `.sdata` 0xC0 B, `.sdata2` 0xA0 B,
 * extab 0x228 B and extabindex 0x33C B.  Phase 4 recut of `Pl/fn_80288CEC` (docs/splits/phase4): the registered range
 * 0x80288CEC..0x8028F66C is two TUs of the candidate - this one and `Pl/pl_hit_sphere`.
 *
 * Name: GUESS, from the motion-step entry points the range holds (`fn_80288CEC` steps the player's motion); no `__FILE__`
 * string covers the range.
 * Flags: `cflags_pl` as the former unit.
 */

/* ==== recut from Pl/fn_80288CEC.cpp (0x80288CEC..0x8028F44C) ==== */
/*
 * Naming note: the symbol map spells 75 of this range's 78 functions as bare `fn_XXXXXXXX`
 * (checked with tools/symbols/dumpmap.py - every address of the range answers `zz_XXXXXXXX_` or
 * `FUN_XXXXXXXX`, never a real runtime name - and the range's `config/RMHE08/symbols.txt` entries
 * carry no signature).  The three the map does name keep the map's spelling
 * (`pl_motion_set__Fv`, `get_gm_daynight__Fv`,
 * `hit_point_sphr__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3f`).
 *
 * Rule 5/7 on the `_PLW` record: the two fields this unit reads arrived as `pl.h`'s `unk009`/`unk00C`
 * and are named there now - `+0x009` is `kind_0x09` (compared against 3 here; the sibling
 * `ef/eft019.cpp` names the same `_PLW` byte `kind_0x09`) and `+0x0C` is `act_no` (the action number
 * `Pl_act_ck` compares as its `u16` argument and the value this unit's act dispatchers switch on).
 *
 * Pl/fn_80288CEC.cpp - the player-mode / move-work TU of the `Pl` module.  `.text`
 * 0x80288CEC-0x8028F66C (78 functions, 0x697C B), extab 0x80012FFC-0x8001323C (72 records) and
 * extabindex 0x80030678-0x800309D8 (72 records); all three ranges are registered in splits.txt.
 * Nothing is claimed out of `.data`/`.sdata`/`.sdata2`: the jump tables and the two resource tables
 * this unit's dispatchers use are still emitted by the `auto_*` objects (invariant 8.4 - do not claim
 * a range our object does not emit).
 *
 * Extent - one TU, not a tiler guess.  The unit's own `.sdata2` run is 0x8079A270-0x8079A314 and it
 * *starts* at the left edge's first function (`fn_80288CEC` loads `lbl_8079A270` = 0.0f) while the
 * preceding proposal's run ends exactly there - a `.sdata2` run boundary is a TU boundary.  Reading
 * the run's first-use offsets in source order gives one strictly monotone authoring sequence (0x270,
 * 0x274 ... 0x2BC, 0x2C0, 0x2C8, 0x2D4 for the act half; then 0x2D8, 0x2DC, 0x2E8, 0x2F0, 0x2F8 ...
 * 0x310 for the rest), and `tools/units/attribute.py`'s `tu.open_seams` for this proposal records no
 * interior cut - only the right-edge jump `lbl_8079A310 -> lbl_8079A314`.
 *
 * Module (`Pl`) - class 3 of the brief's evidence order.  Class 1 fails: there is no `__FILE__`
 * string for this band at all - scanning `orig/RMHE08/sys/main.dol` for `[\\x20-\\x7e]{3,}\\.cpp`
 * yields 99 source names (g3d_*, ef_*, menu_*, enemy_control, cockpit, ...) and not one `pl_*`, so the
 * Pl files carry no assert strings.  Class 2 fails: `dumpmap.py lookup` answers a `zz_XXXXXXXX_`
 * placeholder for every address of the range except 0x8028F2C0, whose real name
 * (`get_gm_daynight`) the symbol map already carries.  Class 3 is conclusive: the unit operates on
 * the player work - `Pl_frame_check__FP4_PLWUlff`, `Pl_zanzo_set__FP4_PLWlUc`,
 * `Pl_act_ck__FP4_PLWUcUs` and `eft029_set_scale__FP4_PLWUcf` all take `_PLW*`, it *defines*
 * `pl_motion_set` (the player motion resource loader), and every accessor at its tail reads the
 * record `get_move_work_adrs(0)` hands back, which `src/enemy/fn_8012BDF4.cpp` documents as the
 * player-side root (`_PLAYER_ROOT`).  The nearest registered unit brackets the band on the `Pl` side:
 * `Pl/pl_act.cpp` ends at 0x8027D684, and the whole unclaimed band 0x8027D684-0x802B2978 is Pl
 * act/equip/skill/camera work.
 *
 * File name - class 4.  Nothing supports a *file* name: no `__FILE__` string (above), no
 * runtime-dump name, and the siblings' scheme (`pl_act`/`pl_master`/`pl_skill`, each from its
 * dominant `Pl_*_ck` symbol) has no dominant prefix here.  The unit is a mixture - act-style `_PLW`
 * state handlers (0x80288CEC-0x8028B330), the player scene/quest/move-work driver
 * (0x8028B330-0x8028EF30) and the root-work accessors plus the `hit_point_sphr` math helper
 * (0x8028EF30-0x8028F66C) - so naming it `pl_motion.cpp` from the single `pl_`-prefixed definition
 * would be inventing a name for a 78-function file.  The stem therefore stays the map's
 * `fn_80288CEC`, matching the two sibling class-4 registrations `Pl/fn_80229ECC.cpp` and
 * `Pl/fn_80241558.cpp`.
 *
 * Language: C++ - the map carries the mangled definitions `pl_motion_set__Fv`,
 * `get_gm_daynight__Fv` and `hit_point_sphr__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3f`, and the retail
 * object carries 72 extab/extabindex records (the Pl lib's `cflags_pl` sets -Cpp_exceptions on).
 *
 * Flags: `cflags_pl` (the lib's) - the act half reads `_PLW+0x0C`/`+0x565`/`+0x566` exactly like
 * `Pl/pl_act.cpp`, which settled `-O3 -inline noauto -opt nopeephole -Cpp_exceptions on` for the lib.
 * The scene half's `PlayMode_ck()` dispatchers were not re-probed.
 *
 * RESIDUAL (this pass).  Function-by-function scores are in the report; what is deliberately NOT
 * written is the set of dispatchers whose bodies are 100-450 instructions of jump-table state
 * machine: fn_80288E68, fn_8028912C, fn_8028921C, fn_802893D4, fn_80289518, fn_80289654,
 * fn_802896F8, fn_8028992C, fn_80289A60, fn_80289BD8, fn_80289CD4, fn_80289FF0, fn_8028A118,
 * fn_8028A1E8, fn_8028A2F8, fn_8028A400, fn_8028A62C, fn_8028A760, fn_8028A8D8, fn_8028A9D4,
 * fn_8028AC14, fn_8028ADB8, fn_8028AEB8, fn_8028AF50, fn_8028B0F4, fn_8028B198, fn_8028B298,
 * fn_8028B330, fn_8028B524, fn_8028BAA8, fn_8028BD98, fn_8028BF1C, fn_8028C168, fn_8028C468,
 * fn_8028C6CC, fn_8028CC7C, fn_8028CD3C, fn_8028CDB0, pl_warp_start, fn_8028D2DC, fn_8028D5F4,
 * fn_8028D778, fn_8028DDCC, fn_8028DF58, fn_8028E0B8, fn_8028E24C, fn_8028E3EC, fn_8028E528,
 * fn_8028E694, fn_8028E718, fn_8028EA84, fn_8028EC28, fn_8028EF7C's tail-call partner
 * `pl_motion_set` (needs the local string-object shape) and the remaining box builders
 * fn_8028F4B4/`558`'s exact float forms.  They are the next pass's work rather than guesses.
 * fn_8028F44C is byte-identical (100.0 %, 104 B): its target allocation is the typed-parameter
 * spelling (`PlBox* a`/`b`, `&b->vec_0x0C`), which spells the three field reaches through the
 * `PlBox` members and lets MWCC emit them as immediate offsets; the earlier `void*`-plus-cast
 * form (the one this pass originally landed) CSE'd the reaches into a saved register and left the
 * body at 116 B / 77.96154 %.
 * Two rule-1 follow-ups are named, not fixed: `_PL_ROOT` here and `_PLAYER_ROOT` in
 * `src/enemy/fn_8012BDF4.cpp` are the same record (the second user should move one definition into a
 * header), and `Pl_frame_check` has no registered owner, so its declaration belongs in
 * `include/unsplit/Pl.h` (not edited here to keep this batch to its own files).
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_80288CEC.h" /* `PlBox`, shared with `Pl/fn_8028F66C.cpp` (rule 1) */
#include "ef/fn_800CDB2C.h"
#include "enemy/em_pop.h" /* quest_flag_8_ck, quest_flag_80_ck (the owner's header, rule 2) */
#include "fn_80047398.h" /* `arena_userdata_apply` (rule 2) */
#include "quest/arenatask.h" /* `arena_user_data_buf` (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "quest/quest_entry.h" /* quest_monsters_release (the owner's header, rule 2) */

/* The player-mode root work `_PL_ROOT` (`get_move_work_adrs(0)`) is declared in `Pl/fn_80288CEC.h`, shared with
 * `hud/cockpit_quest.cpp` (rule 1). */

/* The object `fn_8028BCF4` releases: three `fn_800D8E44` handles, 4 bytes apart.
 * size: 0x144 (lower bound) */
struct PlHandleSet {
    /* +0x000 */ u8 unused_0x000[0x138];
    /* +0x138 */ void* handle_0x138;
    /* +0x13C */ void* handle_0x13C;
    /* +0x140 */ void* handle_0x140;
};

/* ------------------------------------------------------------------------------------------------ *
 * Callees.
 *
 * The map spelled these with an argument list, so they are manglings and belong at C++ scope with
 * the real signature, so the front-end reproduces the map's name (rule 9).  The bare `fn_XXXXXXXX`
 * names are the map's own placeholders and stay `extern "C"`.
 * ------------------------------------------------------------------------------------------------ */

/* ef/fn_800CDB2C.cpp owns the pool accessors (`PlayMode_ck` is declared in that unit's header). */
void* get_move_work_adrs(u8 index);

/* Pl/pl_act.cpp owns `Pl_zanzo_set`; Pl/pl_master.cpp owns `Pl_act_ck` (`include/Pl/Pl_master_ck.h`). */


/* No registered unit covers these addresses; declared with the view this unit's call sites take. */
u32 Pl_frame_check(_PLW* plw, u32 frame, f32 a, f32 b);
void player_init_data_load(void);

extern "C" {
/* `arena_userdata_apply` (0x8004A240) is `fn_80047398.cpp`'s and now comes from its owner header. */




void fn_800553B4(u8 idx);
void fn_800D8E44(void* handle);
void fn_800F6710(void);

u32 fn_8027CB1C(void* self);
u32 stage_map_kind_get(u8 idx);
void fn_802BE568(void* self, u32 sub);
u32 fn_803A7E1C(void);
void fn_803BA814(u8 idx);

}

/* The two small-data seeds `fn_8028F400` fills, and the 256-byte-stride table `fn_8028F1E8` indexes.
 * `setVec3` itself is declared by `include/ef.h` (pulled in by `pl.h`) as `(Vec*, f32, f32,
 * f32)`; re-declaring it here with `void*` is an illegal overload, so the call site casts. */
/* Two 0xC-byte records, still referenced by their own `lbl_` names (a `VEC3[]` view would fold
 * `lbl_806AB83C` into `lbl_806AB830 + 0xC` and lose the second relocation, measured 100 -> 77.84
 * on fn_8028F400). */
extern u8 lbl_806AB830[];
extern u8 lbl_806AB83C[];
/* `arena_user_data_buf` (0x806E3E10) is `quest/arenatask.cpp`'s range and comes from its header. */

/* ------------------------------------------------------------------------------------------------ *
 * Bodies, in address order.
 * ------------------------------------------------------------------------------------------------ */

/* Every bare `fn_XXXXXXXX` is the map's own (unmangled) name, so the definitions carry C linkage;
 * `get_gm_daynight`/`hit_point_sphr` are mangled in the map and stay at C++ scope below. */
extern "C" {

/* 0x80288CEC - the player's primary-act update.  The `+0x0C` action number (`act_no`) selects the
 * "primary act" flag through the compiler's jump table; `fn_8027CB1C` is the act result that latches
 * `+0x565`, and `Pl_frame_check` is the motion gate that arms `+0x566` and hands the player to
 * `fn_802BE568` once. */
void fn_80288CEC(_PLW* self, u32 sub, f32 dt) {
    u32 primary = 0;
    u32 result;

    if (self->field_0x565 == 0) {
        switch (self->act_no) {
        case 0x10:
        case 0x12:
        case 0x14:
        case 0x17:
        case 0x18:
        case 0x1A:
        case 0x1B:
        case 0x21:
        case 0x23:
        case 0x25:
        case 0x28:
        case 0x29:
        case 0x2B:
        case 0x2C:
        case 0x2F:
        case 0x3A:
            primary = 1;
            break;
        default:
            primary = 0;
            break;
        }

        result = (u8)fn_8027CB1C(self);
        if (result == 2 || result == 3 || result == 4) {
            self->field_0x565 = 1;
        } else if (result <= 1 || result == 5) {
            self->field_0x565 = 1;
            if (primary == 0) {
                if (self->kind_0x09 == 3) {
                    pl_act_enter(self, 4, 0x39, 0x4080);
                } else {
                    pl_act_enter(self, 4, 0xF, 0x4000);
                }
            }
        }
    }

    if (self->field_0x566 == 0 && self->field_0x565 == 0 && dt > 0.0f &&
        Pl_frame_check(self, 0, dt, 0.0f) == 1) {
        self->field_0x566 = 1;
        fn_802BE568(self, 0);
    }
}

/* 0x80288E58 / 0x80288E60 - the two `sub` bindings of the primary-act update. */
void fn_80288E58(_PLW* self, f32 dt) {
    fn_80288CEC(self, 0, dt);
}

void fn_80288E60(_PLW* self, f32 dt) {
    fn_80288CEC(self, 1, dt);
}

/* 0x8028BCF4 - release the three `fn_800D8E44` handles the mode object carries. */
void fn_8028BCF4(PlHandleSet* self) {
    if (self == 0) {
        return;
    }
    if (self->handle_0x138 != 0) {
        fn_800D8E44(self->handle_0x138);
    }
    if (self->handle_0x13C != 0) {
        fn_800D8E44(self->handle_0x13C);
    }
    if (self->handle_0x140 != 0) {
        fn_800D8E44(self->handle_0x140);
    }
}

/* 0x8028BD54 - is the root work in mode 2? */
u32 root_mode2_ck(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->mode_0x00 == 2;
}

/* 0x8028D574 - the `flag_0x114 == 1` predicate, 1 when the work is absent. */
u32 fn_8028D574(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 1;
    }
    return root->flag_0x114 <= 0;
}

/* 0x8028D5B8 - store the flag. */
void fn_8028D5B8(s32 value) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root != 0) {
        root->flag_0x114 = value;
    }
}

/* 0x8028C570 - the mode object's per-scene player data init. */
void fn_8028C570(_PL_ROOT* self) {
    player_init_data_load();
    fn_800F6710();
    quest_monsters_release();
    fn_803BA814(self->area_no_0xED);
    fn_800553B4(self->area_no_0xED);
}

/* 0x8028DF24 - arm the mode object's sequence: state 0 -> 1, then hand over. */
void fn_8028DF24(_PL_ROOT* self) {
    switch (self->seq_0x01) {
    case 0:
    case 1:
        self->seq_0x01 = self->seq_0x01 + 1;
        self->mode_0x00 = 1;
        self->seq_0x01 = 0;
        break;
    }
}

/* 0x8028E4F0 - the root work's `+0x22D9` byte. */
u8 fn_8028E4F0(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->field_0x22D9;
}

/* 0x8028EF30 - the root work's byte array at +0x136, indexed by the caller's byte. */
u8 Pl_area_flag_get(u8 index) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->bytes_0x136[index];
}

/* 0x8028EF7C - find the root entry whose key is `key`: the six fixed slots at +0x2274 first, then
 * the area record `fn_803A7E1C()` names, then the single extra pointer at +0x2258. */
PlRootEntry* fn_8028EF7C(u32 key) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    PlRootEntry* entry;
    u32 index;
    u32 count;
    int i;

    if (root == 0) {
        return 0;
    }

    if (key < 0x20) {
        PlRootEntry* slot = root->sparse_0x2274;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        return 0;
    }

    index = fn_803A7E1C();
    entry = root->recs_0x154[index].entries_0x00;
    count = root->rec_counts_0x2154[index];
    for (i = 0; i < (int)count; i++) {
        if (entry[i].key_0x00 == key) {
            return &entry[i];
        }
    }
    if (root->extra_0x2258 != 0) {
        if (root->extra_0x2258->key_0x00 == key) {
            return root->extra_0x2258;
        }
    }
    return 0;
}

/* 0x8028F0B4 - the same search, but the caller names the area index itself. */
PlRootEntry* fn_8028F0B4(u32 key, u32 index) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    PlRootEntry* entry;
    u32 count;
    int i;

    if (root == 0) {
        return 0;
    }

    if (key < 0x20) {
        PlRootEntry* slot = root->sparse_0x2274;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        return 0;
    }

    entry = root->recs_0x154[index].entries_0x00;
    count = root->rec_counts_0x2154[index];
    for (i = 0; i < (int)count; i++) {
        if (entry[i].key_0x00 == key) {
            return &entry[i];
        }
    }
    if (root->extra_0x2258 != 0) {
        if (root->extra_0x2258->key_0x00 == key) {
            return root->extra_0x2258;
        }
    }
    return 0;
}

/* 0x8028F1E8 - hand the caller's `+0x08` byte to the `arena_user_data_buf` table as a 256-byte stride. */
void fn_8028F1E8(u8* self) {
    arena_userdata_apply(arena_user_data_buf + ((u32)self[8] << 8), self);
}

/* 0x8028F204 - the `+0x22D7` byte == 1 predicate. */
u32 fn_8028F204(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return (s8)root->field_0x22D7 == 1;
}

/* 0x8028F24C - the `+0xE9` area byte through `stage_map_kind_get`. */
u32 fn_8028F24C(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return stage_map_kind_get(root->area_no_0xE9);
}

/* 0x8028F288 - the root work's `+0x22DB` byte. */
u8 fn_8028F288(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->field_0x22DB;
}

/* 0x8028F2F8 - arm the mode sequence: 6 for the "no argument" call, 4 otherwise. */
void fn_8028F2F8(u32 arg) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return;
    }
    if (quest_flag_8_ck(0) == 0) {
        return;
    }
    if (arg == 0) {
        root->seq_0xFB = 6;
    } else {
        root->seq_0xFB = 4;
    }
}

/* 0x8028F368 - hand the mode work over to the network move system; 1 on success. */
u32 fn_8028F368(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    u8* mode;

    if (root == 0) {
        return 0;
    }
    mode = root->mode_work_0xDC;
    if (mode == 0) {
        return 0;
    }
    if (quest_flag_80_ck(0) == 0) {
        return 0;
    }
    mode[0x6A3E] = 4;
    if (isServerSelectState() != 1) {
        root->seq_0xFC = 4;
    }
    return 1;
}

/* 0x8028F400 - seed the two `setVec3` vectors from the small-data literals. */
void fn_8028F400(void) {
    setVec3((nw4r::math::VEC3*)lbl_806AB830, 573.0f, -1265.0f, 3392.0f);
    setVec3((nw4r::math::VEC3*)lbl_806AB83C, 1413.0f, -571.0f, 1740.0f);
}

} /* extern "C" */

/* 0x8028F2C0 - the stage's day/night byte (`get_move_work_adrs(0)` + 0x22D8), 0 when absent. */
u8 get_gm_daynight(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->field_0x22D8;
}
