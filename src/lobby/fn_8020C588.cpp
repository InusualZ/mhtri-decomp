/* lobby/fn_8020C588.cpp - the lobby player-character control band.
 *
 * `.text` 0x8020C588..0x80212810 (112 functions, 25224 B), extab 0x80011184..0x80011434 (86 records),
 * extabindex 0x8002D930..0x8002DD38 (86 x 12 B) and one `.data` jump table
 * `jumptable_805B9770` 0x805B9770..0x805B97BC (19 words, the arms of `fn_80212760`'s switch - the
 * table at 0x805B96D8 in front of it is `fn_80211E68`'s and is unclaimed while that body is not
 * written).  Registered from `proposal/8020C588_fn_8020C588.cpp`.
 *
 * Module `lobby`: both registered units bracketing the range in the address band are `lobby`
 * (`lobby/lb_npc.cpp` ends at 0x802029B4, `lobby/fn_80212810.cpp` starts at 0x80212810), and this
 * range both **defines** `LbStr__FUcUs` - the lobby string helper `include/unsplit/lobby.h` declares
 * and `lobby/fn_801E7530.cpp`/`fn_80212810.cpp` call - and reads `lobby_w` and the lobby UI tables.
 *
 * Name.  No `__FILE__` string is reachable from the range and the runtime dump answers only
 * `zz_XXXXXXXX_` placeholders, so the file keeps the map's `fn_8020C588` stem (brief section 2,
 * classes 3+4).  Siblings: `lobby/fn_80212810.cpp`, `lobby/fn_8021E1EC.cpp`.
 *
 * Seam.  Unproven (this is one maximal unclaimed run).  The left edge 0x8020C588 is a proposal
 * boundary, not a TU boundary; the range's extab/`extabindex` runs agree with the right edge exactly
 * (86 records, `fn_8020C588` first, `fn_8021261C` last, and the run ends where
 * `lobby/fn_80212810.cpp`'s extab begins at 0x80011434).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `dumpmap.py lookup` over the range's inventory: every name but `LbStr__FUcUs` and
 * `glplatTextureGetHeight` is a bare `.text` entry in config/RMHE08/symbols.txt and the runtime dump
 * has only `zz_XXXXXXXX_` placeholders for them)
 *
 * Residuals (official report metric; the bar is 80 %):
 *  - 21 of the 112 functions are reconstructed and every one of them is 100.000000 (1660 / 25224
 *    `.text` bytes, 6.58 % of the unit; the `.data` table above is 100 % too).  Measured with
 *    `build/tools/objdiff-cli.exe diff -p . -u main/lobby/fn_8020C588` against this worktree's own
 *    split target object (MAIN has no `obj/lobby/fn_8020C588.o`, so `recompile.py --measure` falls
 *    back to the retired single-symbol objects and cannot pair these).
 *  - the other 91 functions are unwritten.  Every one of them takes the shared `_PLW` player record
 *    and drives it with `Pl_act_ck`/`Pl_master_ck`/`Pl_frame_check`, so the blocker is `include/pl.h`,
 *    which still spells the offsets this range reads `pad_*`/`unk*` (+0x004/+0x005/+0x006 state,
 *    +0x028 timer, +0x03C/+0x040/+0x044 floats, +0x0B4/+0x0B6, +0x30E, +0x313, +0x354,
 *    +0x656/+0x657, +0x265..0x267, +0x5C8).  Naming them in `pl.h` is the sanctioned ``wave 2'' work
 *    (the header says so) but it renames fields `src/Pl/pl_master.cpp` and
 *    `src/sound/fn_800EF7D8.cpp` already read, so it is a config_request, not this batch's edit.  The
 *    enforce-lint would otherwise flag every `unk` member access in this file (rule 7's unk half is
 *    not covered by the `rule 7 deferred` line above).
 *  - `fn_80212370` (352 B) reads the `Psw` pad record's 0x2C0..0x2DF bytes; the only `PlayerPad`
 *    definition lives in `src/mh3_pad.cpp` (rule 1: a shared type in one header), so naming them here
 *    would copy it.  Config_request: move `PlayerPad` into `include/mh3_pad.h`.
 *  - `fn_80211E68` (504 B, 0x80211E68) is the range's other jump-table switch and needs no `_PLW`
 *    field, only `Pl_act_ck(_PLW*, u8, u16)` + `fn_80274810()` passed straight through.  Its arms are
 *    in the DOL's table order `0, 28|33|37, 27, 7, 17, 23, 24, 2, 21, 22, 3, 15, 4, 11, 5, 6, 8, 12,
 *    29, 13, 14, 16, 18, 19, 10, 20, 26, 30, 31, 32, 34, 25, 35` (read out of `jumptable_805B96D8`),
 *    and cases 1, 9 and 36 fall straight through to the end.  Left for the next session rather than
 *    half-written, because it needs 35 callee declarations that belong in other units' headers.
 *  - largest unwritten, largest first: fn_8020EE14 1840 B, fn_8020FE18 1428 B, fn_8020F880 764 B,
 *    fn_8020F544 680 B, fn_8020EBF4 544 B, fn_80211E68 504 B, fn_8020D1FC 488 B.
 *
 * Flags.  The unit needs the auto-inliner and the peephole pass off, and both are file-scoped pragmas
 * (neither setting is one the other `lobby` lib units want): with `-inline auto` MWCC inlined the
 * 220 B `fn_80212060` into each of its five callers (fn_8021213C came out 276 B against a 92 B
 * target, 0 %), and with the peephole on it folds fn_80211DCC's `clrlwi` + `slwi` into one `rlwinm`
 * (16 B against 20).  `#pragma inline off`, `#pragma inline_depth 0` and `#pragma dont_inline on` all
 * work; `dont_inline on` is the one kept.  Measured alternative: `-inline noauto` on `cflags_lobby`
 * fixes this unit too (2.71 -> 5.98 % before the bodies below existed) and moves four other lobby
 * units up (fn_801E7530 27.65 -> 31.76, lb_npc 13.37 -> 13.75, fn_8021E1EC 9.22 -> 9.66) with
 * `lobby_scene` unchanged at 100 - recorded as a config_request rather than applied, since it is a
 * lib-wide change.
 */
#include "types.h"

#include "lobby/fn_8020C588.h"

/* The range was built with the auto-inliner and the peephole pass off: retail keeps the `bl
 * fn_80212060` the accessors below make (with `-inline auto` MWCC inlines its 220-byte body into each
 * of them - fn_8021213C came out 276 B instead of 92) and keeps the unfused `clrlwi` + `slwi` of the
 * species-id index in fn_80211DCC (the peephole folds it into one `rlwinm`).  Both are file-scoped
 * pragmas because neither setting is something the rest of the `lobby` lib needs: `-inline noauto` for
 * the lib does fix this unit, but it also moves four other lobby units' numbers and the pragma keeps
 * the change inside this unit. */
#pragma dont_inline on
#pragma peephole off

/* The four species-id string tables, indexed by the id byte (`fn_80211DCC`..`fn_80211E08`). */
extern "C" s32 fn_80211DCC(u32 id) {
    return lb_chacha_skill_str[(u8)id];
}

extern "C" s32 fn_80211DE0(u32 id) {
    return lb_chacha_mask_str[(u8)id];
}

extern "C" s32 fn_80211DF4(u32 id) {
    return lb_cat_name[(u8)id];
}

extern "C" s32 fn_80211E08(u32 id) {
    return lb_pig_name[(u8)id];
}

/* Returns group `kind`'s string `idx`, or group 1's first string when either index is out of range. */
void* LbStr(u8 kind, u16 idx) {
    if ((u32)kind >= 5 || (s32)idx >= lbl_805B9450[kind]) {
        return (void*)lb_str_tbl[1][0];
    }
    return (void*)lb_str_tbl[kind][idx];
}

/* Nonzero while the lobby screen is owned by a transition or a fade. */
extern "C" u32 fn_80212060(void) {
    if (lobby_w.busy_0x172 != 0) {
        return 1;
    }
    if (get_fade_stat(0) == 1 || get_fade_stat(0) == 2) {
        return 1;
    }
    if (get_fade_stat(1) == 1 || get_fade_stat(1) == 2) {
        return 1;
    }
    if (get_fade_stat(2) == 1 || get_fade_stat(2) == 2) {
        return 1;
    }
    if (get_fade_stat(3) == 1 || get_fade_stat(3) == 2) {
        return 1;
    }
    return fn_803768F8();
}

/* The four player-0 command-mask testers; all report "clear" while the screen is not owned. */
extern "C" s32 fn_8021213C(u16 mask) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return (lobby_w.cmd_mask_0x084[0][0] & mask) != 0;
}

extern "C" s32 fn_80212198(u16 mask) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return (lobby_w.cmd_mask_0x084[0][1] & mask) != 0;
}

extern "C" s32 fn_802121F4(u16 mask) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return (lobby_w.cmd_mask_0x084[0][2] & mask) != 0;
}

extern "C" s32 fn_80212250(u16 mask) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return (lobby_w.cmd_mask_0x084[0][3] & mask) != 0;
}

/* The four player-0 command-mask getters. */
extern "C" u16 fn_802122AC(void) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return lobby_w.cmd_mask_0x084[0][2];
}

extern "C" u16 fn_802122E8(void) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return lobby_w.cmd_mask_0x084[0][0];
}

extern "C" u16 glplatTextureGetHeight(void) {
    return lobby_w.cmd_mask_0x084[0][0];
}

extern "C" u16 fn_80212334(void) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return lobby_w.cmd_mask_0x084[0][3];
}

/* Steps the caller's 4/8 sprite stepper over player 0's first mask. */
extern "C" void fn_80212540(void* self) {
    fn_802A9068(self, fn_802122E8(), 4, 8);
}

/* Steps the caller's 4/8 sprite stepper over the same mask, ignoring the screen guard. */
extern "C" void fn_80212584(void* self) {
    fn_802A9068(self, glplatTextureGetHeight(), 4, 8);
}

/* Toggles the item database's display byte and redraws the lobby when the pad loop is idle. */
extern "C" void fn_802125C8(void) {
    if (game_ready_ck() == 0) {
        lbl_80794880[0x3E00] ^= 1;
        if (fn_8021F238() == 1) {
            fn_801E9888();
        }
        fn_801E9C58();
        fn_802FF2C0();
        fn_80220114();
    }
}

/* Decays the lobby countdown and ramps the +-0x3C cursor slide, then ticks the frame counter. */
extern "C" void fn_8021261C(void) {
    if (lobby_w.countdown_0x028 != 0) {
        lobby_w.countdown_0x028--;
    }
    if (fn_8021F238() == 0) {
        if (lobby_w.slide_0x034 < 0) {
            lobby_w.slide_0x034 = 0;
        }
        if (lobby_w.slide_0x034 < 0x3C) {
            lobby_w.slide_0x034++;
        }
    } else {
        if (lobby_w.slide_0x034 > 0) {
            lobby_w.slide_0x034 = 0;
        }
        if (lobby_w.slide_0x034 > -0x3C) {
            lobby_w.slide_0x034--;
        }
    }
    lobby_w.counter_0x030++;
}

/* The index of the first `count` entries whose running total reaches `value`, over s16 weights. */
extern "C" s16 fn_802126E8(s16* table, s16 count, u32 value) {
    s16 i;
    s16 sum;

    i = 0;
    sum = 0;
    do {
        sum += *table;
        if (value < (u32)sum) {
            break;
        }
        table++;
        i++;
    } while (i < count);
    return i;
}

/* The same walk over byte weights. */
extern "C" s16 fn_80212724(u8* table, s16 count, u32 value) {
    s16 i;
    s16 sum;

    i = 0;
    sum = 0;
    do {
        sum += table[i];
        if (value < (u32)sum) {
            break;
        }
        i++;
    } while (i < count);
    return i;
}

/* Maps a lobby part kind to its string id and returns the string. */
extern "C" void fn_80212760(u32 kind) {
    u16 str_id;

    switch ((u8)kind) {
    default:
        str_id = 0xF0;
        break;
    case 1:
        str_id = 0xF1;
        break;
    case 2:
        str_id = 0xF2;
        break;
    case 3:
        str_id = 0xF3;
        break;
    case 4:
        str_id = 0xF4;
        break;
    case 5:
        str_id = 0xF5;
        break;
    case 6:
        str_id = 0xF6;
        break;
    case 7:
        str_id = 0xFA;
        break;
    case 8:
        str_id = 0xFB;
        break;
    case 9:
        str_id = 0xFC;
        break;
    case 10:
        str_id = 0xFD;
        break;
    case 11:
        str_id = 0xFE;
        break;
    case 12:
        str_id = 0xF7;
        break;
    case 13:
        str_id = 0xFF;
        break;
    case 14:
        str_id = 0x100;
        break;
    case 15:
        str_id = 0x101;
        break;
    case 18:
        str_id = 0xE8;
        break;
    }
    LbStr(0, str_id);
}
