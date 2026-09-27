/* enemy/fn_801B0010.cpp - the em030 (enemy #30) program translation unit.
 *
 * `.text` 0x801B0010..0x801B4458 (60 functions, 17480 B), extab 0x8000F54C..0x8000F6B4 (45 records),
 * extabindex 0x8002AEDC..0x8002B0F8 (45 x 12 B).  Registered once, at its final home, from
 * `proposal/801B0010_fn_801B0010.cpp`.
 *
 * MODULE AND NAME (brief section 2, evidence order).
 *   * Option 1 (a `__FILE__` string) fails: no `.string` in the image names this TU.  The only
 *     `em030` string is `lbl_80791550 = "em030"` (the enemy-id name table at 0x80791540..0x80791560,
 *     one entry per enemy), and the range references no file-name string at all.
 *   * Option 2 (a real runtime-dump name) fails at the unit's own address:
 *     `python tools/symbols/dumpmap.py lookup 0x801B0010` answers `zz_01b0010_`, which is not
 *     evidence.  The range's INTERIOR does carry two real dump names (`em030_condition_ck`,
 *     `em030_homing_range_ck`) and the range's own `.data` carries one real global name
 *     (`em030_prog_tbl` at 0x805B0FD0, a 0x6C-byte table whose slots are this range's functions
 *     0x801B08BC/0x801B0A28/0x801B2D34/0x801B0968/0x801B096C/0x801B2F30/0x801B4240/0x801B4244/
 *     0x801B42A0/0x801B42C4/0x801B42C8), so the module is `enemy` and the TU is the em030 program.
 *     They name the CONTENT, not the source file: the original file could have been `em030.cpp`,
 *     `em030_prog.cpp` or `enemy_em030.cpp`, and the project's rule forbids inventing a file name.
 *   * Option 3 (what the code does plus the neighbours' scheme) fixes the module: every bracketing
 *     registered unit is `enemy/` (`enemy/fn_80191598.cpp` below, the next occupied band above is
 *     `auto_03_801B4E38`), and every callee the range names is enemy-band or the shared enemy work
 *     API (`em_die_ck`, `em_get_mot_no`, `em_frame_check`, `get_move_work_adrs(3)`).
 *   * Option 4 therefore decides the FILE NAME: the neighbours' scheme is `fn_XXXXXXXX.<ext>`
 *     (`src/enemy/` is 27 such files plus the one unit whose `__FILE__` string survived,
 *     `enemy_control.cpp`), and no evidence names the original source file, so the file keeps the
 *     map's own stem.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range except the two `em030_*`
 * runtime-dump names (checked with `python tools/symbols/dumpmap.py lookup` over the range's
 * inventory: 58 of the 60 rows answer `zz_XXXXXXXX_` placeholders, and the two that do not,
 * `em030_condition_ck__FP11_ENEMY_WORK` / `em030_homing_range_ck__FP11_ENEMY_WORK`, are written as
 * the C++ functions their manglings spell).
 *
 * Language C++: every callee out of the range that is not a bare `fn_` placeholder is a mangled
 * symbol (`em_die_ck__FP11_ENEMY_WORK`, `get_move_work_adrs__FUc`, `Pl_Skill_ck__FP4_PLWUs`,
 * `GetItemData__FUs`), and the two dump-named functions are C++ free functions whose manglings the
 * target's `.data` records (`em030_prog_tbl` holds their addresses).
 *
 * STATUS (official `build/RMHE08/report.json`, full `ninja` in this worktree, `main.dol: OK`).
 * 50 of the 60 functions are written and every one of them is above the 80 % bar; 16 are
 * byte-identical.  Unit: 56.920364 % fuzzy, 1432 / 17480 `.text` bytes matched, 16 / 60 functions
 * matched.  Sections: `.text` 0x4448, extab 0x168 (93.82353 %), extabindex 0x21C, `.ctors` 4.
 * The 10 unwritten functions (6616 B, 38 % of the range) are the follow-up queue at the end of this
 * header.
 *
 * TRANSPORT (landed on `main` 2026-09-26 from `worker/801b0010-fn-801b0010-ab10`).  The registration
 * and the bodies came across as a delta; the header set was reconciled with the units that landed
 * while the branch waited, and the two functions whose rows moved moved UP (`fn_801B0230` 95.09 ->
 * 95.64 and `fn_801B03E8` 91.35 -> 93.65: the branch had declared `Pl/pl_skill.h`'s `fn_802731B4`/
 * `fn_80272E30` at C++ scope, while their owner defines them `extern "C"`, so their call sites were
 * emitting a mangled reloc the target does not have).  Everything else that changed here:
 *   * `fn_80267270` moved from `include/unsplit/Pl.h` (a fallback band, and a `rule 2` finding once
 *     its owner existed) to the owner's header `include/Pl/fn_80262940.h`; `fn_801E01BC` likewise to
 *     `include/enemy/fn_801DB8E0.h`, and this unit's own three band symbols to
 *     `include/enemy/fn_801B0010.h` (`fn_801B0010` was parked in `unsplit/enemy.h` with the note
 *     "owned by the still-unregistered proposal/801B0010 range"; `fn_801B4348`/`fn_801B4398` were
 *     declared in `enemy/fn_801B4458.cpp`).
 *   * `EmGroundRec` is now one definition, in `include/enemy/ENEMY_WORK.h`: this unit's own copy was
 *     0x18 bytes where the landed view (`enemy/fn_801B4458.cpp`, whose fields +0x18/+0x1C it reads)
 *     is 0x20, i.e. the scratch buffer `fn_801B4348` hands `fn_80125F54`/`fn_801421E4` was 8 bytes
 *     short of what the same record needs elsewhere.
 *   * this unit's 8-byte lookup entry is `EmCodeListEntry` here: `enemy/fn_801CA004.cpp` has an
 *     `EmLookupEntry` of its own for the different 0x805B3CD8 table (rule 1 - two records, two
 *     names).
 *   * `get_move_work_adrs`/`get_move_work_max` stay declared in this file (see the note above their
 *     declarations): MAIN carries four incompatible spellings of the pair, so neither can move into
 *     the owner's header yet.  Unifying them is booked in this unit's outbox (`shared-file`).
 *
 * Residuals, by measurement (all near-misses are codegen shapes, not comprehension):
 *   * RANGE-CASE TREE - `fn_801B0010` 92.94 (the group {1,2} is tested as two compares where the
 *     target uses the `(x-1) <= 1` range idiom; the `(u32)(team - 1) <= 1` spelling reproduces the
 *     exact instruction multiset and size - 252 B both sides - but reorders the blocks and scores
 *     82.22), `fn_801B096C` 88.72 (same shape on the `{10,11}` group of its `state` switch; the
 *     `if`-chain spelling is 4 B short and does not pair).
 *   * BLOCK ORDER AFTER A SHARED TAIL - `fn_801B4244` 91.09 and `fn_801B42DC` 84.93: MWCC places the
 *     `return 0` block inline where the target keeps it last, and the `.ctors` body's two
 *     `setVec3` calls colour their float registers differently.
 *   * `clrlwi` FUSION - the peephole fuses the `clrlwi rX,rY,24` + `cmpwi rX,0` pair of a byte test
 *     into the record form `clrlwi.`; `fn_801B4244` needs it off (`#pragma peephole off` scoped to
 *     that one body: 59.78 -> 91.09).  Every other body in the unit keeps the peephole on.
 *   * ARGUMENT SET ORDER - `fn_801B2514` 91.74: its `fn_80133DB0`-family rows and the `setVec3`
 *     float arguments are materialised in a different order.
 *   * `fn_801B0230` 95.64 and `fn_801B03E8` 93.65 (both improved by the transport): the remaining
 *     rows are the `Pl_Skill_ck`/`fn_802731B4` argument setup and `fn_801B03E8`'s two
 *     `lbl_806BD360` byte reads.
 *
 * Follow-up queue (the 10 unwritten functions, biggest first; sizes in bytes):
 *   fn_801B2F30 (4880), fn_801B28C0 (600), fn_801B2B6C (436), fn_801B2D94 (412), fn_801B2684 (264),
 *   fn_801B278C (192), fn_801B284C (116), fn_801B2D34 (96), fn_801B2B18 (84), fn_801B2D20 (20).
 *   All ten are the same two shapes the written bodies already establish - a `switch` on
 *   `_ENEMY_WORK::state` (`fn_801B2684`, `fn_801B278C`, `fn_801B284C`, `fn_801B28C0`, `fn_801B2B18`,
 *   `fn_801B2B6C`, `fn_801B2D20`, `fn_801B2D34`) or the third `state_sub` jump-table dispatcher
 *   (`fn_801B2D94`, `fn_801B2F30`, whose `.data` tables sit at 0x805B11C0 and 0x805B14F0).
 */
/* The unit's foreign callees are declared where their owner is (docs/plan.md 6.5 rule 2): the
 * unsplit enemy-band symbols in `unsplit/enemy.h`, the registered units' in their own headers, and
 * this unit's own band symbols in `include/enemy/fn_801B0010.h`.
 *
 * `pl.h` (which brings `ef.h`) and `mh3_pad.h` both declare the owner's `VEC3_ctor`/`setVec3`; the
 * macro workaround that used to guard the include is gone because the two headers now spell them
 * identically (the `(10197)` clash they were working around is closed). */
#include "types.h"

#include "nw4r/math.h"

#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801B0010.h"
#include "enemy/fn_801D428C.h"
#include "enemy/fn_801DB8E0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_801251D0.h" /* EmGroundRec + fn_80125F54 (rule 1/2: their owner) */
#include "enemy/fn_8012EC74.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_80137604.h"
#include "ef/fn_800CDB2C.h"
#include "fn_8004CAD8.h"
#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/pl_master.h"
#include "Pl/pl_skill.h"
#include "unsplit/enemy.h"
#include "Pl/fn_80262940.h" /* fn_80267270 (rule 2: its owner's header) */

#include "mh3_pad.h"

/* ------------------------------------------------------------------------------------------------ */
/* declarations this unit needs whose owner's header does not carry them yet                          */
/* ------------------------------------------------------------------------------------------------ */

/* 0x8029F6DC - the item-table accessor, owned by the not-yet-registered band between
 * `Pl/pl_act.cpp` and `stage/fn_802B2978.c` (rule 2's named gap: the bracketing registered units
 * name different modules, so there is no sound header to move the declaration to).  Declared at C++
 * scope so the call site spells the owner's real signature (`GetItemData__FUs`), not the mangling
 * (rule 9); the return type is not part of the mangling, so this unit's view of the row is what it
 * declares. */
/* size: 0x4 */
struct EmItemRow {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 rank_0x01; /* the "kind" byte the em030 picker filters on (`< 3`) */
    /* +0x02 */ u8 unused_0x02[2];
};
EmItemRow* GetItemData(u16 id);
/* 0x803AAB80 - the not-yet-registered band between `hud/fn_80324F7C.c` and
 * `Network/NetworkWiiMediator.c` (named gap).  r3 the player work, r4 the id, r5 the value. */
extern "C" void fn_803AAB80(_PLW* plw, u16 id, s32 value);
/* 0x802D8414 - `ai_torch_ck(_AINPC_W*)`; the owner band sits between `ai/fn_802D0DCC.c` and
 * `ef/fn_803066F0.c` (named gap).  Declared at C++ scope with its real signature (rule 9). */
struct _AINPC_W;
u32 ai_torch_ck(_AINPC_W* npc);

/* The game-state block `lbl_806BD360` (0x4A8 bytes of `.bss`): this range reads its +0x000 liveness
 * byte and the +0x1A4 area byte, nothing else, so it is declared as the byte blob it is (no field
 * layout of its own is evidence here). */
extern "C" u8 lbl_806BD360[];

/* The `void (*)(_ENEMY_WORK*, u32)` the stance action reads out of `shell_set_func_ptr`'s +0x58 slot. */
typedef void (*ShellSetFn)(_ENEMY_WORK* work, u32 id);
/* The block `shell_set_func_ptr` points at (the map types it `void*`); its +0x58 slot is the shell-set
 * function the stance action calls, so it is reached as a field rather than by arithmetic (rule 6). */
/* size: 0x5C */
struct ShellSetBlock {
    /* +0x00 */ u8 unused_0x00[0x58];
    /* +0x58 */ ShellSetFn field_0x58;
};

/* The 8-byte `lbl_805B1B08` lookup entry: a key byte, a value byte and the list pointer (the list
 * holds more records of this same shape, keyed by the kind).  It is NOT `enemy/fn_801CA004.cpp`'s
 * same-named entry, which models the different 0x805B3CD8 table (`{key, count, EmLookupSub*}`); the
 * two records are modelled separately and the names must not collide (docs/plan.md 6.5 rule 1), so
 * this unit's view is named after its own table.
 * size: 0x8 */
struct EmCodeListEntry {
    /* +0x0 */ u8 code;
    /* +0x1 */ u8 value_0x01;
    /* +0x2 */ u8 unused_0x02[2];
    /* +0x4 */ void* list_0x04;
};
extern "C" EmCodeListEntry lbl_805B1B08[];

/* The 0x20-byte ground record `fn_80125F54` prepares and `fn_801421E4` fills is `EmGroundRec`, and
 * it now lives with its producer (`include/enemy/fn_801251D0.h`, docs/plan.md 6.5 rule 1: one
 * definition; `enemy/fn_801B4458.cpp` includes the same home).  This unit only passes its address
 * and reads its `pos_0x08`. */

/* 0x800CFA90 / 0x800CFAD0 - the move-work record base and its count.  Their owner's header
 * (`include/ef/fn_800CDB2C.h`) cannot carry either one yet: MAIN spells the pair four incompatible
 * ways - the owner's own bodies return `void*` and `u16`, `include/unsplit/ef.h` carries
 * `extern "C" void*`/`u32`, `Pl/fn_8025F088.h` and `enemy/fn_80165FC8.h` carry the C++ `void*`/`u32`
 * pair, and `src/ef/eft_res.cpp`/`src/sound/fn_800EF7D8.cpp` re-declare both locally as `u16` -
 * and MWCC rejects any fifth spelling in every TU that includes two of them (`(10505) illegal
 * overloading`).  Measured: `u32` in the owner's header breaks `ef/eft_res.cpp`,
 * `sound/fn_800EF7D8.cpp` and `Pl/fn_80273B14.cpp`; `u16` breaks `Pl/fn_8025F088.cpp`,
 * `Pl/fn_802489D4.cpp`, `Pl/fn_802430E8.cpp` and `Pl/fn_80273B14.cpp`.  Choosing one spelling is a
 * change to four landed units' declarations, so both accessors stay declared HERE, at C++ scope with
 * the owner's own return types (the map names are the manglings `get_move_work_adrs__FUc` and
 * `get_move_work_max__FUc`, rule 9), and the unification is booked in this unit's outbox
 * (`shared-file`).  The owner header is left exactly as MAIN has it. */
void* get_move_work_adrs(u8 index);
u16 get_move_work_max(u8 index);

/* The two `fn_8004CAD8.cpp` vector helpers this range calls now come from that unit's owner header
 * (`include/fn_8004CAD8.h`, rule 2): `fn_80050F80` measures the distance between two positions and
 * `fn_80050CA0` subtracts them.  Their signatures there were settled from the callees' own bodies. */

/* The shared `.sdata2` pool constants this range loads (never defined here - redefining them would
 * rebuild the pool instead of addressing the target's, playbook 29). */
extern "C" f32 lbl_80798B3C; /* 2250000.0f - the squared 1500-unit radius `fn_80050EAC` is compared to */
extern "C" f32 lbl_80798B38; /* 40000.0f - the squared 200-unit homing radius */
extern "C" f32 lbl_80798B44; /* 144.0f - the `em_frame_check` window's first bound */
extern "C" f32 lbl_80798B48; /* 0.0f - its second bound */
extern "C" f32 lbl_80798B4C; /* -100.0f - the knockback blend scale */
extern "C" f32 lbl_80798B50; /* -300.0f - the second knockback blend scale */
extern "C" f32 lbl_80798B54; /* 0.7f - the motion blend `em_mot_speed_set` is handed */
extern "C" f32 lbl_80798B58; /* 0.2f - the `fn_801B1814` motion blend */
extern "C" f32 lbl_80798B5C; /* 200.0f - the distance `fn_801B1CF4` subtracts */
extern "C" f32 lbl_80798B60; /* 0.0f - the +0x310 triple component `fn_801B1F94` arms */
extern "C" f32 lbl_80798B64; /* 0.5f - its kind-1 .z */
extern "C" f32 lbl_80798B68; /* 0.7f - its other .z */
extern "C" f32 lbl_80798B6C; /* 0.2f - the +0x320 component it arms */
extern "C" f32 lbl_80798B70; /* 1.2f - the `em_mot_speed_set` blend `fn_801B2298` arms */
extern "C" f32 lbl_80798B74; /* 0.5f - the first `em_frame_check` window bound */
extern "C" f32 lbl_80798B78; /* 0.3f - the second */
/* The `.sdata` shell-set callback pointer the stance action calls through its +0x58 slot. */
extern "C" void* shell_set_func_ptr;
extern "C" f32 lbl_80798B94; /* 100.0f - the first record's y */
extern "C" f32 lbl_80798C58; /* 50.0f - its z */
extern "C" f32 lbl_80798C5C; /* -100.0f - the second record's y */
extern "C" f32 lbl_80798C60; /* -50.0f - its z */
/* The two three-float records the unit's static constructor fills (0x18 bytes of `.bss`). */
extern "C" u8 lbl_806A7A88[];
/* 0x803B9BA0 - the not-yet-registered band between `hud/fn_80324F7C.c` and
 * `Network/NetworkWiiMediator.c` (rule 2's named gap): r3 the work record, r4 the position, r5 the id. */
extern "C" void fn_803B9BA0(_ENEMY_WORK* work, nw4r::math::VEC3* pos, u32 id);
extern "C" f32 lbl_80798B40; /* 2000.0f */

/* ------------------------------------------------------------------------------------------------ */
/* the range's own view of the player work it is handed                                              */
/* ------------------------------------------------------------------------------------------------ */

/* `_PLW` comes from `include/pl.h` (rule 1); the record is the one `em030_prog_tbl`'s caller passes
 * in, and this range only ever reads the slot table at +0x278 (`_SLOTENT`, 4 bytes each, its
 * `item_id` half read as a 16-bit id) and the liveness byte at +0x000. */

/* ------------------------------------------------------------------------------------------------ */
/* bodies                                                                                            */
/* ------------------------------------------------------------------------------------------------ */

/* 0x801B0010 (0xFC).  "Is any live work record of kind 3 in `area` already handling this fight?" -
 * walks the 0xB18-byte records `get_move_work_adrs(3)` hands back and asks the per-team checker
 * (cases 1/2 the shared one, 5 and 7 their own) whether the record is engaged. */
extern "C" u32 fn_801B0010(u32 area) {
    u16 max = get_move_work_max(3);
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    s32 i = 0;

    for (; i < max; i++, work++) {
        if (!work->active) {
            continue;
        }
        if (work->area_no != (u8)area) {
            continue;
        }
        switch (work->team) {
        case 1:
        case 2:
            if (fn_80154784(work) == 1) {
                return 1;
            }
            break;
        case 5:
            if (fn_801D6694(work) == 1) {
                return 1;
            }
            break;
        case 7:
            if (fn_801E01BC(work) == 1) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* 0x801B010C (0x1C).  The em030 program's "condition" hook: the action 7 branch reports the record's
 * +0x330 byte, every other action reports -1. */
s32 em030_condition_ck(_ENEMY_WORK* work) {
    if (work->action != 7) {
        return -1;
    }
    return work->field_0x330;
}

/* 0x801B0128 (0x40).  The em030 program's homing-range hook: the work is in homing range while its
 * distance to its own +0x36C target point is inside the squared 200-unit radius. */
s32 em030_homing_range_ck(_ENEMY_WORK* work) {
    return fn_80050EAC(&work->pos, &work->vec_0x36C) < lbl_80798B38;
}

/* 0x801B0168 (0xC8).  The em030 program's action guard: state 0 always runs, state 1 runs only while
 * the game-state block's area is still the work's area. */
extern "C" u32 fn_801B0168(_ENEMY_WORK* work) {
    switch (work->field_0x380) {
    case 1:
        if (work->field_0x382 != 0xFF) {
            _ENEMY_WORK* target = (_ENEMY_WORK*)fn_801377D0(work->state_0x381, work->field_0x382);
            if (target->active) {
                if (fn_8012D0B4(work, target) == 1) {
                    return 0;
                }
            }
        }
        return 1;
    case 2:
        if (work->field_0x382 != 0xFF && lbl_806BD360[0] != 0 && work->area_no == lbl_806BD360[0x1A4]) {
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 0x801B0230 (0x1B8).  Picks the player's counter-move for em030: the two "already fighting" skills
 * first (0x238 then 0xD9), then the highest-value usable slot, and latches it on the work record
 * together with the player work it came from. */
extern "C" u32 fn_801B0230(_ENEMY_WORK* work, _PLW* plw) {
    if (plw == NULL) {
        return 0;
    }
    if (plw->slot_active == 0) {
        return 0;
    }
    if (Pl_master_ck(plw) != 1) {
        return 0;
    }
    if (fn_8027BC48(0) == 1) {
        return 0;
    }
    if (Pl_Skill_ck(plw, 0x5A) == 1) {
        return 0;
    }
    if (fn_802731B4(plw, 0x238) > 0) {
        work->field_0x328 = 0x238;
    } else if (fn_802731B4(plw, 0xD9) > 0) {
        work->field_0x328 = 0xD9;
    } else {
        u8 usable[0x18];
        u8 count = 0;
        u8 i;

        for (i = 0; i < 0x18; i++) {
            u16 id = plw->slot_id[i].item_id;
            if (id != 0 && id != 0xDF && id != 0x2B) {
                if (GetItemData(id)->rank_0x01 < 3) {
                    usable[count] = i;
                    count++;
                }
            }
        }
        if (count == 0) {
            return 0;
        }
        work->field_0x328 = plw->slot_id[usable[ran_suu(0) % count]].item_id;
    }
    work->plw_0x32C = plw;
    fn_80272E30(plw, work->field_0x328, -1);
    fn_80267270(plw, 2, 0x1B, work->field_0x328);
    return 1;
}

/* 0x801B03E8 (0x68).  Cancels the move `fn_801B0230` latched: the work record's item 1, then the
 * player's 0x1C command. */
extern "C" void fn_801B03E8(_ENEMY_WORK* work) {
    if (work->field_0x328 == 0) {
        return;
    }
    if (work->plw_0x32C == NULL) {
        return;
    }
    fn_80272E30(work->plw_0x32C, work->field_0x328, 1);
    fn_80267270(work->plw_0x32C, 2, 0x1C, work->field_0x328);
    work->field_0x328 = 0;
    work->plw_0x32C = NULL;
}

/* 0x801B0450 (0x54).  The teardown of the same latch (the player's 0xFF command, no item restore). */
extern "C" void fn_801B0450(_ENEMY_WORK* work) {
    if (work->field_0x328 == 0) {
        return;
    }
    if (work->plw_0x32C == NULL) {
        return;
    }
    fn_803AAB80(work->plw_0x32C, work->field_0x328, -1);
    work->field_0x328 = 0;
    work->plw_0x32C = NULL;
}

/* 0x801B04A4 (0x5C).  Arms the 0x20 "condition" bit of `field_0x761` in state 0 and the 0x40 one in
 * state 1, then toggles `field_0x333`; returns 1 only when the state-1 branch ran. */
extern "C" u32 fn_801B04A4(_ENEMY_WORK* work) {
    switch (work->field_0x333) {
    case 0:
        work->field_0x761 = (work->field_0x761 & 0x2F) | 0x20;
        work->field_0x333 = work->field_0x333 + 1;
        break;
    case 1:
        work->field_0x761 = (work->field_0x761 & 0x4F) | 0x40;
        work->field_0x333 = 0;
        return 1;
    }
    return 0;
}

/* 0x801B0500 (0x5C).  The same toggle for the 0x20/0x10 bits. */
extern "C" u32 fn_801B0500(_ENEMY_WORK* work) {
    switch (work->field_0x333) {
    case 0:
        work->field_0x761 = (work->field_0x761 & 0x2F) | 0x20;
        work->field_0x333 = work->field_0x333 + 1;
        break;
    case 1:
        work->field_0x761 = (work->field_0x761 & 0x1F) | 0x10;
        work->field_0x333 = 0;
        return 1;
    }
    return 0;
}

/* 0x801B055C (0x178).  The em030 action-state machine's 4-state ring (0 -> 3 -> 1 -> 2 -> 0), plus
 * the motion-id refinement of the 0x40/0x80 bits of `field_0x761`. */
extern "C" void fn_801B055C(_ENEMY_WORK* work) {
    switch (work->field_0x332) {
    case 0:
        if ((work->field_0x761 & 0x10) == 0) {
            work->field_0x761 = (work->field_0x761 & 0x1F) | 0x10;
        }
        if (work->field_0x762 == 1) {
            work->field_0x332 = 3;
        }
        break;
    case 1:
        if ((work->field_0x761 & 0x40) == 0) {
            work->field_0x761 = (work->field_0x761 & 0x4F) | 0x40;
        }
        if (work->field_0x762 == 0) {
            work->field_0x332 = 2;
        }
        break;
    case 2:
        if (fn_801B0500(work) == 1) {
            if (work->field_0x762 == 1) {
                work->field_0x332 = 3;
            } else {
                work->field_0x332 = 0;
            }
        }
        break;
    case 3:
        if (fn_801B04A4(work) == 1) {
            if (work->field_0x762 == 0) {
                work->field_0x332 = 2;
            } else {
                work->field_0x332 = 1;
            }
        }
        break;
    }

    switch ((u16)em_get_mot_no(work)) {
    case 0x27:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x69:
    case 0x6A:
    case 0x6B:
    case 0x6C:
    case 0x6D:
    case 0x6E:
        work->field_0x761 = (work->field_0x761 & 0x8F) | 0x80;
        break;
    case 0x14:
    case 0x1E:
        work->field_0x761 = (work->field_0x761 & 0x4F) | 0x40;
        break;
    }
}

/* 0x801B06D4 (0x13C).  "Is any live area-work record of kind 2 (or the live game-state block's own
 * record) inside the 1500-unit radius?" - `fn_80050EAC` answers the squared distance. */
extern "C" u32 fn_801B06D4(_ENEMY_WORK* work) {
    u16 max = get_move_work_max(2);
    EmAreaWork* area = (EmAreaWork*)get_move_work_adrs(2);
    u8 i;

    for (i = 0; i < max; i++, area++) {
        if (!area->active) {
            continue;
        }
        if (fn_8012D0B4(work, area) == 0) {
            continue;
        }
        if (fn_8012D7FC((_ENEMY_WORK*)area) == 0) {
            continue;
        }
        if (fn_80050EAC(&work->pos, &area->vec_0x3C) <= lbl_80798B3C) {
            return 1;
        }
    }

    if (lbl_806BD360[0] != 0 && work->area_no == lbl_806BD360[0x1A4]) {
        if (ai_torch_ck((_AINPC_W*)lbl_806BD360) == 1) {
            if (fn_80050EAC(&work->pos, (nw4r::math::VEC3*)(lbl_806BD360 + 0x178)) <= lbl_80798B3C) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x801B0810 (0xAC).  "Is any live work record of kind 3 in the same area whose +0x1C8 is armed?" -
 * the matching record's position is copied onto the work's +0x1B0. */
extern "C" u32 fn_801B0810(_ENEMY_WORK* work) {
    u16 max = get_move_work_max(3);
    _ENEMY_WORK* other = (_ENEMY_WORK*)get_move_work_adrs(3);
    u8 i;

    for (i = 0; i < max; i++, other++) {
        if (!other->active) {
            continue;
        }
        if ((other->field_0x1C8 & 0x7FFFFFFF) == 0) {
            continue;
        }
        if (other->area_no != work->area_no) {
            continue;
        }
        copyVec3(&work->aim, &other->pos);
        return 1;
    }
    return 0;
}

/* 0x801B08BC (0x2C0... 0xAC).  The em030 program's reset/exit hook: the action-2 teardown runs the
 * motion helpers, then the record's em030 block is cleared and the "active" bit re-armed. */
extern "C" void fn_801B08BC(_ENEMY_WORK* work, u8 kind) {
    if (kind == 2) {
        fn_80130248(work);
        fn_801305C4(work);
        fn_80128A8C(work, 0x0C, 0);
        fn_80133BC0(work);
    }
    work->field_0x328 = 0;
    work->plw_0x32C = NULL;
    work->field_0x330 = 0;
    work->field_0x331 = 0;
    work->field_0x332 = 0;
    work->field_0x333 = 0;
    work->field_0x334 = 0;
    work->field_0x761 = (work->field_0x761 & 0x1F) | 0x10;
    if (work->team == 0x1E || work->field_0x00A == 1) {
        work->field_0x1C8 |= 0x80;
    }
}

/* 0x801B0968 (0x4).  The empty slot `em030_prog_tbl` keeps for the unused state. */
extern "C" void fn_801B0968(void) {}

/* 0x801B096C (0xBC).  The em030 per-state hook: the states 10/11 branch on the motion kind, state 1
 * dispatches on the sub-kind. */
extern "C" void fn_801B096C(_ENEMY_WORK* work, u32 state, u32 kind) {
    switch ((u8)state) {
    case 1:
        switch ((u8)kind) {
        case 0x13:
            if (work->field_0x43B == 2 && work->field_0x43C == 1) {
                fn_8013072C(work, 4, 0);
            }
            break;
        case 0x10:
            if (work->field_0x43B == 3) {
                fn_8013072C(work, 0, 0);
            }
            break;
        }
        break;
    case 10:
    case 11:
        if (work->field_0x331 == 1) {
            if (fn_801339AC(work) == 0) {
                work->field_0x331 = 0;
            }
        }
        break;
    }
}

/* 0x801B0A28 (0x32C).  The em030 program's per-frame update: the death/approach checks, the
 * per-state action block, the shared tail flags and the +0x761 bit updates. */
extern "C" void fn_801B0A28(_ENEMY_WORK* work) {
    u32 zero = 0;

    work->field_0x330 = zero;
    if (em_die_ck(work) == 0) {
        if (work->field_0x334 == 0) {
            if (fn_801B0810(work) == 1) {
                if (work->field_0x43B != 1) {
                    fn_8013072C(work, 3, 0);
                }
                work->field_0x334 = 1;
            }
        } else {
            if (fn_801B0810(work) == 0) {
                work->field_0x334 = zero;
            }
        }
        if (work->field_0xA07 == 1) {
            if (work->field_0x43B == 2 && work->field_0x43C == 0) {
                fn_8013072C(work, 2, 1);
            }
            work->field_0xA07 = 0;
        }
    }

    switch (work->field_0x43B) {
    case 0:
        if (work->action != 0x0A && em_die_ck(work) == 0 && fn_801B06D4(work) == 1) {
            fn_8013072C(work, 2, 0);
        }
        break;
    case 2:
        switch (work->field_0x43C) {
        case 0:
            if (work->field_0x833 == 1) {
                fn_8012C3C8(0x1E, work->area_no, &work->pos, lbl_80798B40);
                fn_8012C3C8(0x1F, work->area_no, &work->pos, lbl_80798B40);
                fn_8013072C(work, 2, 1);
            } else {
                if (fn_801B06D4(work) == 0) {
                    fn_8013072C(work, 0, 0);
                }
            }
            break;
        case 1:
            if (em_die_ck(work) == 0 && work->field_0x440 > 300) {
                fn_8013072C(work, 4, 0);
            }
            break;
        }
        break;
    case 3:
        if (em_die_ck(work) == 0) {
            if (work->field_0x440 > 1800) {
                fn_8013072C(work, 0, 0);
            } else if (work->field_0x00A == 1) {
                fn_8013072C(work, 0, 0);
            }
        }
        break;
    case 4:
        if (em_die_ck(work) == 0) {
            if (work->field_0x440 > 1800) {
                fn_8013072C(work, 0, 0);
            } else if (work->field_0x00A == 1) {
                fn_8013072C(work, 0, 0);
            }
        }
        break;
    }

    if (work->field_0x00A != 1 && work->field_0x833 == 1) {
        fn_8012C300(0x1E, work->area_no);
        fn_8012C300(0x1F, work->area_no);
    }
    if (fn_801339AC(work) == 1 && work->field_0x331 == 0) {
        work->field_0x331 = 1;
    }
    if (work->field_0x331 == 1) {
        if ((work->field_0x761 & 0x01) == 0) {
            work->field_0x761 |= 0x01;
        }
    } else {
        if ((work->field_0x761 & 0x01) != 0) {
            work->field_0x761 &= 0xFE;
        }
    }
    fn_801B055C(work);
}

/* 0x801B0D54 (0x90).  The two-step approach action: arm the motion, then wait for `em_mot_end_ck`. */
extern "C" void fn_801B0D54(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set_ck(work, fn_801339AC(work) == 1 ? 0x24 : 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B0DE4 (0x30).  The shared step of the three "approach" actions: all three states run the same
 * 0x801B0D54 step (the tail calls are per case, as the target's three `b` instructions show). */
extern "C" void fn_801B0DE4(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        fn_801B0D54(work);
        break;
    case 1:
        fn_801B0D54(work);
        break;
    case 2:
        fn_801B0D54(work);
        break;
    }
}

/* 0x801B0E14 (0xEC).  The kinded approach action: state 0 arms the motion the kind selects (the
 * six variants 0x14..0x21, default 0x13), state 1 waits for it. */
extern "C" void fn_801B0E14(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        {
            u32 motion;

            switch ((u8)kind) {
            case 1:
                motion = 0x14;
                break;
            case 2:
                motion = 0x15;
                break;
            case 3:
                motion = 0x1A;
                break;
            case 4:
                motion = 0x1B;
                break;
            case 5:
                motion = 0x20;
                break;
            case 6:
                motion = 0x21;
                break;
            default:
                motion = 0x13;
                break;
            }
            em_mot_set(work, motion, 4, 0);
        }
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B0F00 (0xBC).  The motion-0x16 approach: arm it with the 0x78-frame countdown, run it down,
 * then arm the 0x19/6 recovery. */
extern "C" void fn_801B0F00(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x16, 4, 0);
        work->timer_0x020 = 0x78;
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            work->state = work->state + 1;
            em_mot_set(work, 0x19, 6, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B0FBC (0xBC).  The motion-0x17 approach, 0x96 frames, then the 0x19/6 recovery with the 4 flag. */
extern "C" void fn_801B0FBC(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x17, 4, 0);
        work->timer_0x020 = 0x96;
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            work->state = work->state + 1;
            em_mot_set(work, 0x19, 6, 4);
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1078 (0xBC).  The motion-0x18 approach, 0x96 frames, then the 0x19/4 recovery. */
extern "C" void fn_801B1078(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x18, 4, 0);
        work->timer_0x020 = 0x96;
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            work->state = work->state + 1;
            em_mot_set(work, 0x19, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1134 (0x90).  The motion-0x1C approach, its countdown seeded from the record's 5-bit
 * `bits_0x1EC` field. */
extern "C" void fn_801B1134(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x1C, 4, 0);
        work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x3C;
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B11C4 (0x150).  The motion-0x1D approach, whose countdown and follow-up depend on the kind. */
extern "C" void fn_801B11C4(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x1D, 4, 0);
        if ((u8)kind == 1) {
            work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x96;
        } else {
            work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0xFA;
        }
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            if ((u8)kind == 1) {
                work->state = work->state + 1;
                em_mot_set(work, 0x1E, 4, 0);
            } else {
                work->state = work->state + 2;
                em_mot_set(work, 0x1F, 4, 0);
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            work->state = work->state + 1;
            em_mot_set(work, 0x1F, 4, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1314 (0x90).  The motion-0x24 approach with the 0x96/6 recovery. */
extern "C" void fn_801B1314(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x24, 6, 0);
        work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x96;
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B13A4 (0x1A4).  The motion-0x25 approach: the kind picks the countdown (0x96/0x5A/0x32), the
 * countdown expiry picks the follow-up (kind 1 the 0x26 motion plus the position helper, kind 2 and
 * the default the 0x67 one). */
extern "C" void fn_801B13A4(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x25, 0, 0);
        switch ((u8)kind) {
        case 0:
            work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x96;
            break;
        case 1:
            work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x5A;
            break;
        case 2:
            work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x32;
            break;
        }
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            switch ((u8)kind) {
            case 1:
                work->state = work->state + 1;
                em_mot_set(work, 0x26, 0, 0);
                fn_803B9BA0(work, &work->pos, 0x64);
                break;
            case 2:
                work->field_0x331 = 0;
                /* fall through */
            default:
                work->state = work->state + 2;
                em_mot_set(work, 0x67, 4, 0);
                break;
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            work->state = work->state + 1;
            em_mot_set(work, 0x67, 4, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1548 (0xB0).  The motion-0x8 retreat: the frame window drives the two scene helpers, and the
 * motion's end cancels the latch and finishes the work. */
extern "C" void fn_801B1548(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x08, 4, 0);
        break;
    case 1:
        if (em_frame_check(work, 1, lbl_80798B44, lbl_80798B48) == 1) {
            fn_80131DB4(work);
            fn_80131DF4(work);
        }
        if (em_mot_end_ck(work) == 1) {
            fn_801B0450(work);
            fn_8012E694(work);
        }
        break;
    }
}

/* 0x801B15F8 (0xAC).  The em030 action dispatcher: `state_sub` selects the action function (the
 * `.data` jump table `jumptable_805B103C` holds the 20 entries), and the `fn_801B0E14` entries carry
 * their kind in r4. */
extern "C" void fn_801B15F8(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        fn_801B0E14(work, 0);
        break;
    case 1:
        fn_801B0E14(work, 1);
        break;
    case 2:
        fn_801B0E14(work, 2);
        break;
    case 3:
        fn_801B0F00(work);
        break;
    case 4:
        fn_801B0FBC(work);
        break;
    case 5:
        fn_801B1078(work);
        break;
    case 6:
        fn_801B0E14(work, 3);
        break;
    case 7:
        fn_801B0E14(work, 4);
        break;
    case 8:
        fn_801B1134(work);
        break;
    case 9:
        fn_801B11C4(work, 0);
        break;
    case 10:
        fn_801B0E14(work, 5);
        break;
    case 11:
        fn_801B0E14(work, 6);
        break;
    case 12:
        fn_801B1314(work);
        break;
    case 13:
        fn_801B13A4(work, 0);
        break;
    case 14:
        fn_801B1548(work);
        break;
    case 15:
        fn_801B13A4(work, 2);
        break;
    case 16:
        fn_801B0F00(work);
        break;
    case 17:
        fn_801B11C4(work, 1);
        break;
    case 18:
        fn_801B13A4(work, 1);
        break;
    case 19:
        fn_801B0E14(work, 3);
        break;
    }
}

/* 0x801B16A4 (0x170).  The knockback/recovery action: the kind picks the motion and the blend scale,
 * the flag the countdown, and the state-1 half re-runs the 0x800-flag check and clamps the countdown
 * once the action guard says the hit is over. */
extern "C" void fn_801B16A4(_ENEMY_WORK* work, u32 kind, u32 flag) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        {
            u32 motion;
            f32 scale;

            switch ((u8)kind) {
            case 0:
                motion = 2;
                scale = lbl_80798B4C;
                break;
            case 1:
                motion = 3;
                scale = lbl_80798B4C;
                break;
            case 2:
            case 3:
                motion = 3;
                scale = lbl_80798B50;
                break;
            }
            em_mot_set(work, motion, 4, 0);
            if ((u8)kind == 3) {
                em_mot_speed_set(work, lbl_80798B54);
            }
            fn_80134004(work, 0, scale);
        }
        if ((u8)flag == 1) {
            work->timer_0x020 = 0x3C;
        } else {
            work->timer_0x020 = 0x96;
        }
        break;
    case 1:
        if (fn_80134114(work, 0, 0x800) == 1 || --work->timer_0x020 <= 0) {
            em_action_finish(work);
        }
        if (work->timer_0x020 > 0x0F) {
            if (fn_801B0168(work) == 1) {
                work->timer_0x020 = 0x0F;
            }
        }
        break;
    }
}

/* 0x801B1814 (0x144).  The motion-0x18/0x05 recovery: the 0x100-flag check (or the 0x12C-frame
 * countdown) picks the 0x19 tail, and the countdown is clamped to 0x0F once the guard says the hit is
 * over. */
extern "C" void fn_801B1814(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x18, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            work->state = work->state + 1;
            em_mot_set(work, 0x05, 4, 0);
            fn_80134004(work, 0, lbl_80798B58);
            work->timer_0x020 = 0x12C;
        }
        break;
    case 2:
        if (fn_80134114(work, 0, 0x100) == 1 || --work->timer_0x020 <= 0) {
            work->state = work->state + 1;
            em_mot_set(work, 0x19, 4, 0);
        }
        if (work->timer_0x020 > 0x0F) {
            if (fn_801B0168(work) == 1) {
                work->timer_0x020 = 0x0F;
            }
        }
        break;
    case 3:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1958 (0xE8).  The motion-0x06 stagger: the 0x800-flag check (or the 0x12C-frame countdown)
 * picks the 0x19 tail. */
extern "C" void fn_801B1958(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x06, 4, 0);
        fn_80134004(work, 0, lbl_80798B4C);
        work->timer_0x020 = 0x12C;
        break;
    case 1:
        if (fn_80134114(work, 0, 0x800) == 1 || --work->timer_0x020 <= 0) {
            work->state = work->state + 1;
            em_mot_set(work, 0x19, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1A40 (0xF0).  The motion-0x06 turn: the record's own 4-bit `bits_0x1EC` field is latched into
 * `state_0x006` as the turn step, and the second angle is stepped through `fn_80133DB0` while the
 * 0x78-frame countdown runs. */
extern "C" void fn_801B1A40(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x06, 4, 0);
        work->timer_0x020 = 0x78;
        work->state_0x006 = work->bits_0x1EC & 0x0F;
        break;
    case 1:
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            work->state = work->state + 1;
            em_mot_set(work, 0x19, 4, 0);
        } else {
            work->field_0x1C0 = fn_80133DB0((u16)(work->state_0x006 << 12), (u16)work->field_0x1C0, 0x800);
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1B30 (0xA0).  The motion-0x04 recovery: the float-armed `fn_80130008` check, then the
 * position latch and the three teardown helpers. */
extern "C" void fn_801B1B30(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        fn_80130248(work);
        fn_801305C4(work);
        em_mot_set(work, 0x04, 4, 0);
        break;
    case 1:
        if (fn_80130008(work) != 0) {
            work->pos.y = work->field_0x20C;
            em_move_mode_set(work, 0);
            fn_801353F8(work);
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1BD0 (0x90).  The motion-0x02 approach: the per-frame `fn_80132154` step, then the
 * 0x400-flag wait. */
extern "C" void fn_801B1BD0(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x02, 4, 0);
        fn_80132154(work);
        break;
    case 1:
        fn_80132154(work);
        if (fn_80133C50(work, 0x400) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1C60 (0x94).  The motion-0x22 recovery with the 0x80-flag wait. */
extern "C" void fn_801B1C60(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x22, 6, 0);
        fn_80134004(work, 0, lbl_80798B48);
        break;
    case 1:
        if (fn_80134114(work, 0, 0x80) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1CF4 (0x194).  The motion-0x06 turn-to-target: the angle between the record's +0x36C target
 * point and its position is turned into a per-frame step (clamped at 0), the position is stepped
 * through `fn_80133DB0`, and the 0x78-frame countdown picks the 0x19 tail. */
extern "C" void fn_801B1CF4(_ENEMY_WORK* work, u32 kind) {
    VEC3 vec;
    VEC3 diff;
    u32 angA;
    u32 angB;

    VEC3_ctor(&vec);

    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x06, 4, 0);
        work->timer_0x020 = (work->bits_0x1EC & 0x3F) + 0x78;
        break;
    case 1:
        {
            f32 v = fn_80050F80(&work->vec_0x36C, &work->pos) - lbl_80798B5C;
            s32 step;

            if (v < lbl_80798B48) {
                v = lbl_80798B48;
            }
            step = 0x2000 - ((s32)v << 4);
            if (step < 0) {
                step = 0;
            }
            fn_80050CA0(&diff, &work->vec_0x36C, &work->pos);
            copyVec3(&vec, &diff);
            calcVecAngXY(&vec, &angA, &angB);
            if ((u8)kind == 1) {
                step = -step;
            }
            work->field_0x1C0 = fn_80133DB0((u16)(angB + step), (u16)work->field_0x1C0, 0x800);
            work->timer_0x020--;
            if (work->timer_0x020 <= 0) {
                work->state = work->state + 1;
                em_mot_set(work, 0x19, 4, 0);
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1E88 (0x7C).  The motion-0x02 recovery. */
extern "C" void fn_801B1E88(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x02, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B1F04 (0x90).  The second em030 action dispatcher (`state_sub` 0..13), the `.data` jump table
 * `jumptable_805B108C` holds the 14 entries and the `fn_801B16A4` entries carry their kind/flag in
 * r4/r5. */
extern "C" void fn_801B1F04(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        fn_801B16A4(work, 0, 0);
        break;
    case 1:
        fn_801B16A4(work, 1, 0);
        break;
    case 2:
        fn_801B1814(work);
        break;
    case 3:
        fn_801B1958(work);
        break;
    case 4:
        fn_801B1A40(work);
        break;
    case 5:
        fn_801B1B30(work);
        break;
    case 6:
        fn_801B1BD0(work);
        break;
    case 7:
        fn_801B1C60(work);
        break;
    case 8:
        fn_801B1CF4(work, 0);
        break;
    case 9:
        fn_801B1CF4(work, 1);
        break;
    case 10:
        fn_801B16A4(work, 1, 1);
        break;
    case 11:
        fn_801B16A4(work, 2, 0);
        break;
    case 12:
        fn_801B1E88(work);
        break;
    case 13:
        fn_801B16A4(work, 3, 0);
        break;
    }
}

/* 0x801B1F94 (0xA8).  The "armed" motion: the two teardown helpers, the 0x0E motion, the position
 * helper, and the +0x310 angle triple the kind arms before `rotVecY` turns it by the record's
 * second angle. */
extern "C" void fn_801B1F94(_ENEMY_WORK* work, u32 kind) {
    fn_80130248(work);
    fn_801305C4(work);
    em_mot_set(work, 0x0E, 0, 0);
    fn_80129668(work, 0, 1);
    fn_801353F8(work);
    work->offset_0x30C.vec_0x310.y = lbl_80798B60;
    if ((u8)kind == 1) {
        work->offset_0x30C.vec_0x310.z = lbl_80798B64;
    } else {
        work->offset_0x30C.vec_0x310.z = lbl_80798B68;
    }
    work->field_0x320 = lbl_80798B6C;
    rotVecY(&work->offset_0x30C.vec_0x310, work->field_0x1C0);
}

/* 0x801B203C (0x25C).  The kinded "recovery/counter" action: state 0 arms the motion the kind picks,
 * state 2 runs the counter-attack latch (`fn_801B0230` on the player work at +0xA34) and the
 * `UpdateValue` wait, state 3 finishes it. */
extern "C" void fn_801B203C(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state_0x006 = 0;
        switch ((u8)kind) {
        case 1:
            work->state = work->state + 2;
            fn_801B1F94(work, 0);
            break;
        case 2:
            work->state = work->state + 2;
            fn_801B1F94(work, 1);
            break;
        default:
            work->state = work->state + 1;
            em_move_mode_set(work, 0);
            em_mot_set(work, 0x0D, 4, 0);
            break;
        }
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            work->state = work->state + 1;
            fn_801B1F94(work, 1);
        }
        break;
    case 2:
        CancelFade(work);
        if ((u8)kind == 2 && work->state_0x006 == 0 && work->field_0xA16 != 0xFF && work->field_0xA16 != 5
            && work->field_0xA38 == 0) {
            if (fn_801B0230(work, work->plw_0xA34) == 1) {
                fn_8013072C(work, 1, 0);
            }
            fn_80129724(work, 0);
            work->state_0x006 = 1;
        }
        if (em_mot_end_ck(work) == 1) {
            fn_80130248(work);
            if (UpdateValue(work) != 0) {
                u16 motion;

                work->state = work->state + 1;
                em_move_mode_set(work, 0);
                if (work->field_0xA16 != 0xFF && work->field_0xA16 != 5 && work->field_0xA38 == 0) {
                    work->state_0x006 = 1;
                }
                if ((u8)kind == 1) {
                    motion = 0x10;
                } else {
                    motion = work->state_0x006 == 1 ? 0x0F : 0x10;
                }
                em_mot_set(work, motion, 0, 0);
            }
        }
        break;
    case 3:
        if (em_mot_end_ck(work) == 1) {
            if ((u8)kind == 2) {
                em_action_finish(work);
            } else if (work->state_0x006 == 1) {
                em_action_finish(work);
            } else {
                em_state_set(work, 1, 1);
            }
        }
        break;
    }
}

/* 0x801B2298 (0x27C).  The kinded turn action: state 0 latches the target angle with `calcVecAngXY`,
 * state 1 waits (or steps the second angle through `fn_80133DB0`), and the team/kind gates pick the
 * follow-up. */
extern "C" void fn_801B2298(_ENEMY_WORK* work, u32 kind) {
    VEC3 vec;
    VEC3 diff;
    u32 angA;
    u32 angB;

    VEC3_ctor(&vec);

    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        work->state_0x006 = 0;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x06, 4, 0);
        fn_8012933C(work, 0, 2, 3);
        if ((u8)kind == 1) {
            work->timer_0x020 = 0x78;
            em_mot_speed_set(work, lbl_80798B70);
        } else {
            work->timer_0x020 = 0x3C;
        }
        fn_80050CA0(&diff, &work->vec_0x36C, &work->pos);
        copyVec3(&vec, &diff);
        calcVecAngXY(&vec, &angA, &angB);
        work->field_0x37C = angB;
        break;
    case 1:
        if (work->field_0xA16 != 0xFF && work->field_0xA38 == 0) {
            work->state = work->state + 1;
            work->state_0x006 = 1;
            fn_80129724(work, 0);
            em_mot_set(work, 0x19, 4, 0);
            break;
        }
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            work->state = work->state + 1;
            fn_80129724(work, 0);
            em_mot_set(work, 0x19, 4, 0);
        } else if ((u8)kind == 1) {
            fn_80133C50(work, 0x800);
        } else {
            work->field_0x1C0 = fn_80133DB0((u16)work->field_0x37C, (u16)work->field_0x1C0, 0x800);
        }
        if (work->timer_0x020 > 0x0F) {
            if (fn_801B0168(work) == 1) {
                work->timer_0x020 = 0x0F;
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(work) == 1) {
            if (work->team == 0x1F) {
                em_action_finish(work);
            } else if (work->state_0x006 == 1) {
                work->state = work->state + 1;
                em_mot_set(work, 0x1C, 4, 0);
            } else {
                em_action_finish(work);
            }
        }
        break;
    case 3:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B2514 (0x170).  The motion-0x11/0x12 stance: the two frame windows drive the shell-set
 * callback the kind selects, and the +0x330 latch is armed when the second/third window closes. */
extern "C" void fn_801B2514(_ENEMY_WORK* work, u32 kind) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 0x11, 2, 0);
        work->timer_0x020 = 0x50;
        break;
    case 1:
        if (em_frame_check(work, 0, lbl_80798B74, lbl_80798B48) == 1) {
            ShellSetFn set = ((ShellSetBlock*)shell_set_func_ptr)->field_0x58;

            if ((u8)kind == 1) {
                set(work, 3);
            } else {
                set(work, 2);
            }
        }
        if (em_frame_check(work, 1, lbl_80798B74, lbl_80798B48) == 1) {
            work->field_0x330 = 1;
        }
        work->timer_0x020--;
        if (work->timer_0x020 <= 0) {
            work->state = work->state + 1;
            em_mot_set(work, 0x12, 4, 0);
        }
        break;
    case 2:
        if (em_frame_check(work, 2, lbl_80798B78, lbl_80798B48) == 1) {
            work->field_0x330 = 1;
        }
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* 0x801B4240 (0x4).  The empty slot `em030_prog_tbl` keeps. */
extern "C" void fn_801B4240(void) {}

/* 0x801B4244 (0x5C).  The em030 "condition" hook the program table installs: kind 0 and the +0x331
 * latch admit the check, and the answer is 1 or 2 by whether the action guard is still on.
 *
 * `#pragma peephole off` for this one body: the target keeps the `clrlwi r0,r4,24` + `cmpwi r0,0`
 * pair, while the peephole fuses them into the record form `clrlwi.` + `beq` (measured: on 59.78,
 * off 100.00).  The pragma is turned back on immediately after, so no other body moves. */
#pragma peephole off
extern "C" u32 fn_801B4244(_ENEMY_WORK* work, u32 kind) {
    if ((u8)kind == 0) {
        if (work->field_0x331 == 1) {
            return fn_801339AC(work) == 1 ? 1 : 2;
        }
    }
    return 0;
}
#pragma peephole on

/* 0x801B42A0 (0x24).  The em030 teardown hook: cancel the latched move and report the exit state. */
extern "C" u32 fn_801B42A0(_ENEMY_WORK* work) {
    fn_801B03E8(work);
    return 2;
}

/* 0x801B42C4 (0x4).  The em030 exit hook's tail call. */
extern "C" void fn_801B42C4(_ENEMY_WORK* work) {
    fn_801B0450(work);
}

/* 0x801B42C8 (0x14).  The em030 state-report hook: the current state id and a zero flag. */
extern "C" void fn_801B42C8(_ENEMY_WORK* work, u8* state, u8* flag) {
    *state = 0x0C;
    *flag = 0;
}

/* 0x801B42DC (0x6C).  The unit's static constructor (the `.ctors` word at 0x8056F344): it fills the
 * two three-float records of the `lbl_806A7A88` table. */
extern "C" void fn_801B42DC(void) {
    VEC3 rec;

    setVec3(&rec, lbl_80798B48, lbl_80798B94, lbl_80798C58);
    fn_80051490((Vec*)lbl_806A7A88, (Vec*)&rec);
    setVec3(&rec, lbl_80798B48, lbl_80798C5C, lbl_80798C60);
    fn_80051490((Vec*)(lbl_806A7A88 + 0x0C), (Vec*)&rec);
}

/* 0x801B4348 (0x50).  The em030 ground-position hook: the ground record `fn_80125F54` builds for the
 * work's +0x1A id is copied onto the work's +0x1B0 when the enemy-control lookup finds it. */
extern "C" void fn_801B4348(_ENEMY_WORK* work) {
    EmGroundRec rec;

    fn_80125F54(&rec);
    if (fn_801421E4(work->field_0x01A, &rec) == 1) {
        copyVec3(&work->aim, &rec.pos_0x08);
    }
}

/* 0x801B4398 (0xC0).  The em030 program-table lookup: the `lbl_805B1B08` entry whose +0x1E0 key
 * matches the work record's, then the 8-byte entry list under it, keyed by the kind. */
extern "C" u32 fn_801B4398(_ENEMY_WORK* work, u32 kind, u32* out) {
    u8 i;

    for (i = 0; lbl_805B1B08[i].code != 0xFF; i++) {
        EmCodeListEntry* entry;

        if (fn_80125FF0(work->field_0x1E0, 0) != 1) {
            continue;
        }
        entry = (EmCodeListEntry*)lbl_805B1B08[i].list_0x04;
        if (entry == NULL) {
            break;
        }
        for (; entry->code != 0xFF; entry++) {
            if (entry->code == (u8)kind) {
                *out = (u32)entry->list_0x04;
                return entry->value_0x01;
            }
        }
        break;
    }
    return 0;
}
