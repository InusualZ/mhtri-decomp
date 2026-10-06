/*
 * Pl/pl_motion.cpp - the player actor's motion layer: the act-style `_PLW` state handlers (0x80288CEC-0x8028B330),
 *   the player scene/quest/move-work driver (0x8028B330-0x8028EF30) and the root-work accessors (0x8028EF30 on).
 * RANGE. .text 0x80288CEC-0x8028F44C (74 functions); .ctors 0x8056F36C, .data 0x805CBFC0-0x805CDC44, .bss
 *   0x806AB830-0x806AB848, .sdata 0x807921B0-0x80792270, .sbss 0x80794B38-0x80794B58, .sdata2 0x8079A270-0x8079A310,
 *   extab, extabindex.  Left seam: the unit's `.sdata2` run starts with `fn_80288CEC`'s first constant
 *   (`lbl_8079A270` = 0.0f) where the preceding unit's run ends.  `Pl/pl_hit_sphere.cpp` follows it.
 * NAMES. The file name is a GUESS from the motion-step entry points (`fn_80288CEC` steps the player's motion,
 *   `pl_motion_set` loads the motion resources); `pl_motion_set` and `get_gm_daynight` are the map's names.  The root
 *   work `get_move_work_adrs(0)` hands back is `_PL_ROOT` (`Pl/fn_80288CEC.h`), the record `enemy/em_common.cpp`
 *   defines again as `_PLAYER_ROOT` (rule 1).
 * RESIDUALS. 53 functions unwritten (objdiff scores them 0) in 6 runs: 0x80288E68-0x8028BCF4, 0x8028BD98-0x8028C570,
 *   0x8028C5B4-0x8028D574, 0x8028D5F4-0x8028DF24, 0x8028DF58-0x8028E4F0, 0x8028E528-0x8028EF30; most are jump-table
 *   state machines of 100-450 instructions, and `pl_motion_set` needs the local string-object shape.  4 partial,
 *   none with a recorded cause: `fn_80288CEC`, `fn_8028DF24`, `fn_8028EF7C`..`fn_8028F0B4`.
 *  - flipcheck: the object emits no `.bss` (0x18 claimed), `.ctors` (0x4), `.sbss` (0x20) or `.sdata` (0xC0); `.text`
 *    0x868, `.data` 0xAC, `.sdata2` 0x1C, extab 0x88 and extabindex 0xCC against the claims 0x6760, 0x1C84, 0xA0,
 *    0x228 and 0x33C; every compared section differs.
 * SHAPES. The two 0xC-byte records `fn_8028F400` fills stay `lbl_806AB830`/`lbl_806AB83C`: a `VEC3[]` view folds the
 *   second into `lbl_806AB830 + 0xC` and loses its relocation.
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_80288CEC.h" /* `PlBox` and `_PL_ROOT`, shared with `Pl/pl_hit_sphere.cpp` and `Pl/pl_coll.cpp` (rule 1) */
#include "ef/fn_800CDB2C.h"
#include "enemy/em_pop.h" /* quest_flag_8_ck, quest_flag_80_ck (the owner's header, rule 2) */
#include "fn_80047398.h" /* `arena_userdata_apply` (rule 2) */
#include "quest/arenatask.h" /* `arena_user_data_buf` (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "quest/quest_entry.h" /* quest_monsters_release (the owner's header, rule 2) */

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

/* ef/system_core.cpp owns the pool accessors (`PlayMode_ck` is declared in its header `ef/fn_800CDB2C.h`). */
void* get_move_work_adrs(u8 index);

/* `Pl_zanzo_set` and `Pl_act_ck` are `Pl/pl_act.cpp`'s (`Pl/pl_act.h`, `Pl/Pl_master_ck.h`). */


/* No registered unit covers these addresses; declared with the view this unit's call sites take. */
u32 Pl_frame_check(_PLW* plw, u32 frame, f32 a, f32 b);
void player_init_data_load(void);

extern "C" {




void fn_800553B4(u8 idx);
void fn_800D8E44(void* handle);
void fn_800F6710(void);

u32 fn_8027CB1C(void* self);
u32 stage_map_kind_get(u8 idx);
void fn_802BE568(void* self, u32 sub);
u32 quest_arena_item_count_get(void);
void fn_803BA814(u8 idx);

}

/* The two small-data seeds `fn_8028F400` fills (see the unit header); `setVec3` is `ef.h`'s `(Vec*, f32, f32, f32)`,
 * so the call site casts. */
extern u8 lbl_806AB830[];
extern u8 lbl_806AB83C[];

/* ------------------------------------------------------------------------------------------------ *
 * Bodies, in address order.
 * ------------------------------------------------------------------------------------------------ */

/* Every bare `fn_XXXXXXXX` is the map's own (unmangled) name, so the definitions carry C linkage;
 * `get_gm_daynight` is mangled in the map and stays at C++ scope below. */
extern "C" {

/* 0x80288CEC - the player's primary-act update: `act_no` selects the primary-act flag through a jump table,
 * `fn_8027CB1C` latches `+0x565` and `Pl_frame_check` arms `+0x566` and hands the player to `fn_802BE568` once. */
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
 * the area record `quest_arena_item_count_get()` names, then the single extra pointer at +0x2258. */
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

    index = quest_arena_item_count_get();
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
