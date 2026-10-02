/*
 * enemy/em033_prog.cpp - phase 4 unit, `.text` 0x8035BAB4..0x8035F2B4 (55 functions, 14336 bytes).
 *
 * PHASE 4 (docs/splits/phase4, window d).  Fold of 2 registered units: eft052.cpp, fn_8035E034.cpp.  The functions
 * below are the ones those sources define, in address order; every other function of the range keeps its original
 * bytes.  16 of 55 functions have a body here.
 *
 * FLAGS.  `cflags_main`; the bodies come from the old enemy/fn_8035E034.cpp (the same group).
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.data, .rodata, .sdata, .sdata2, .text, extab, extabindex).
 */
/* ---- header inherited from src/ef/eft052.cpp (written against its pre-phase-4 range) ---- */
/* ef/eft052.cpp - the `eft052` effect family (the `_EFT` tag 52) and the cockpit item-page band it
 * draws, `.text` 0x80358624..0x8035E034 (92 functions / 23056 B).
 *
 * WHAT IT IS.  An effect family plus the cockpit hold/item band it draws from, exactly the shape the
 * registered neighbour `ef/eft050.cpp` documents: `eft052_set` pools the effect record
 * (`eft_res_slot_get(76)`, the 76-byte block `ef/eft035.cpp` measured) and installs its
 * `dispatch_0x34`/`release_0x40` hooks, `eft052_dispatch` is the `state_0x05` machine whose arms are
 * the model-create pass (`res_eft_model_create_light`), the placement/alive body and the pool release
 * (`eft_res_slot_release`), and the rest of the range is the cockpit item layer: the page counters
 * (`eft052_page_counts_get`, `eft052_page_take`, `eft052_page_put`), the hold block at
 * `.bss:0x806BF368`, the item/monster strings (`LbStr`, `ItemName`, `GetItemData`) and the sprite
 * sheet (`draw_sprite_*`, `get_lsp_data`, `PutPageArrow`, `font_*`).  Three bodies read the enemy work
 * record (`em_parts_damage_level_get`, `fn_80126278`, `calcDistanceSqXZ`), which is what the effect
 * family reports on.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable: every
 * `lbl_` reference in the 92 split objects resolves to the `.sdata2` float pool 0x8079B640..0x8079B704,
 * to the `.data` run 0x805ED0C0..0x805ED938 or to a call - never to a source-file-name literal.
 * 2. `dumpmap.py lookup` answers only `zz_` placeholders (two stubs carry Ghidra's library-signature
 * match, `DBClose` at 0x8035BC24 and `J3DColorBlockLightOff::getColorChanNum` at 0x8035DD40, which is
 * signature noise on a 4- and an 8-byte stub, not a name).  3. Class 3, the module: `ef` - the range
 * is the `eft` family shape above (the `+0x34` dispatch / `+0x40` release hook pair, the `state_0x05`
 * machine, `_EFT::area_0x44`, the pooled-model handle) with the cockpit band's callee profile, and its
 * callers are the `ef` and `lobby` bands (`ef/ef_emitter.cpp` calls `fn_8035B998`, `ef/eft050.cpp`
 * calls `fn_8035A7D8`, `lobby/fn_801E7530.cpp`/`lobby/fn_801EC9F8.cpp` call
 * `eft052_hold_entry_set`/`eft052_item_value_get`, `lobby/fn_801E0ADC.cpp` calls
 * `eft052_page_count_add`/`eft052_hold_cursor_step`/`eft052_hold_row_get`).  4. The family's own tag
 * named it: `eft052_set` seeds `_EFT::field_0x03 = 52` (in the target object, byte-identical to ours:
 * `li r0,52` / `stb r0,3(r30)`), and that tag is the file name of every registered sibling whose name
 * the dump knows - `ef/eft001.cpp` seeds 1, `eft002` 2, `eft009` 9, `eft019` 19 ("the eft019
 * family tag"), `eft035` 35 and `eft050` 50 (0x32).  The file name `eft052` is therefore DERIVED from
 * the unit's own tag and stays a GUESS (no dump name exists for the family - the runtime dump has only
 * `zz_XXXXXXXX_` for 0x80358624 and no `__FILE__` string is reachable), so a later pass may refine it.
 *
 * NAMES.  Every symbol this file DEFINES is named from its own body - the family tag `eft052` plus
 * what the body does - and each is a guess a later pass may refine (the evidence is the body, not the
 * map); the 72 entries this file does not reconstruct yet keep the map's `fn_` stems, because they are
 * absent or referenced, never defined:
 *   * the family: `eft052_set` (pool the record with `eft_res_slot_get(76)`, install the two hooks, seed
 *     both part slots), `eft052_release` (the `+0x40` hook - hand every part slot's handle back),
 *     `eft052_dispatch` (the `+0x34` hook - one handler per `state_0x05`), `eft052_place` (the
 *     placement pass - pick the per-area table, create both models, advance), `eft052_state_step`
 *     (advance the state) and `eft052_pool_release` (`eft_res_slot_release`);
 *   * the enemy part report the family draws from: `eft052_part_damage_ck` (one flag per queried part
 *     index), `eft052_part_level_even_ck` (whether the part's damage level is even) and
 *     `eft052_part_gauge_add` (step the part gauge and clamp it to 0..500);
 *   * the item page (`lobby_world_block`): `eft052_item_value_get`/`eft052_item_half_get` (the item
 *     record's +0x010 value / its +0x00C value halved, floored at 1), `eft052_page_counts_get` (the
 *     three counts of one id, through optional out pointers), `eft052_page_count_add` (move one id's
 *     count by `delta`), `eft052_page_take`/`eft052_page_put` (the two moves between the page and the
 *     caller's hand) and `eft052_page_count_ck` (one of the page's two counts for an id - the cabinet
 *     or the hand);
 *   * the hold block (`.bss:0x806BF368`): `eft052_hold_row_set` (re-point the block at a row - the one
 *     owned symbol whose body is still unwritten, so its declaration below stays a reference),
 *     `eft052_hold_cursor_step` (re-point at the clamped cursor row and clear the dirty byte),
 *     `eft052_hold_row_get` (the cursor row's table value and the block's +0x0A word),
 *     `eft052_hold_entry_set` (hand the caller's entry to the block and re-seed it) and
 *     `eft052_hold_entry_copy` (copy one entry field by field).
 *
 * Naming note: every remaining `fn_XXXXXXXX` in this file is a REFERENCE to a symbol ANOTHER unit
 * owns (the rule's tolerated half, checked with `python tools/symbols/symedit.py refs` on each of the
 * file's names): the effect pool/manager (`eft_res_slot_get`, `eft_res_model_get`, `eft_res_slot_release`, `eft_state_flags_set`,
 * `fn_800F8A44`), the enemy band (`fn_80126278`, `em_parts_damage_level_get`) and the lobby item API
 * (`fn_8004Axxx`/`fn_8004Bxxx`).  `symedit.py range` shows the map spells none of those ranges with
 * anything but their `fn_` stems, so this file cannot name them.  No symbol this unit owns stays
 * generated.
 *
 * SEAM.  The brief's range is one `attribute.py` `--max-bytes` cut of the unclaimed
 * 0x8034C1D0..0x8035E034 run and no `__FILE__` string exists anywhere in it, so the decisive class-1
 * test cannot fire.  Both edges are recorded as candidate re-draws, not as settled: the `.sdata2`
 * pool run continues in referrer order across the lower edge (0x8079B638 -> 0x8079B640) and
 * `em033_prog_tbl` (0x805ED370) sits in the previous proposal's own `.data` span while listing eight
 * of this range's functions; at the upper edge the vtable-like table `lbl_805ED808` (inside this
 * range's `.data` span) mixes this range's functions with `fn_8035F004`/`fn_8035EF50`/`fn_8035EF58`
 * of the registered `enemy/fn_8035E034.cpp`.  Neither edge can be proven false in-session.
 *
 * LANGUAGE AND SECTIONS.  C++ (the map's mangled callees `GetItemData__FUs`, `LbStr__FUcUs`,
 * `res_eft_model_create_light__FP6MHcharUsUll`, `calcDistanceSqXZ__FP...`, and the class whose
 * constructor `fn_8035BBE8` installs `lbl_805ED808`).  Every plain `fn_` definition is `extern "C"` so
 * it keeps the map's name (playbook 42).  dtk appended the unit's `extab` 0x8001736C..0x80017574 and
 * `extabindex` 0x80036B88..0x80036E94 lines to the splits block itself, from the gaps the bracketing
 * registrations leave; they are not yet reproduced byte for byte.  The `.data` run
 * 0x805ED0C0..0x805ED938, the `.sdata2` pool and the tables are declared, never defined (playbook 29).
 *
 * FLAGS: the `ef` lib's `cflags_main` (Wii/1.3, `-inline noauto`) plus `#pragma peephole off` for this
 * file - the file keeps the unfused forms retail has, exactly like the `menu` and `hud` libs
 * (`cflags_menu`/`cflags_hud` are `cflags_main` + `-opt nopeephole`).  Measured: with the peephole on,
 * `eft052_part_damage_ck` reads 93.82 % and `eft052_part_level_even_ck` 91.92 % (the fused
 * `clrlslwi`/`clrlwi.` forms where retail keeps `clrlwi`+`slwi`/`clrlwi`+`cmpwi`); with it off, both
 * are byte-identical.  A lib-level `-opt nopeephole` needs two agreeing units (policy 8.2) - this is
 * the first, and it is scoped to the file until a second unit of the band is written.
 *
 * STATUS / RESIDUALS (measured 2026-09-27, official report metric; 23056 B total).  The rename to
 * `eft052` moved no row (all 92 rows, and every other unit's, are identical before and after).
 *   * 20 of the 92 functions are reconstructed: 2568 `.text` bytes, unit 10.37 % fuzzy, 94.05 % mean
 *     over the 20 scored rows.  Byte-identical: `eft052_part_damage_ck`, `eft052_part_level_even_ck`,
 *     `eft052_part_gauge_add`, `eft052_set`, `eft052_dispatch`, `eft052_state_step`,
 *     `eft052_pool_release`, `eft052_item_value_get`, `eft052_page_count_add`,
 *     `eft052_hold_entry_set`.
 *   * 89-99 %: `eft052_item_half_get` 99.29, `eft052_release` 98.60, `eft052_page_counts_get` 95.72,
 *     `eft052_page_count_ck` 93.33, `eft052_hold_entry_copy` 91.30, `eft052_place` 89.30 (the
 *     `lbl_805ED0C0`/`lbl_805ED120`/`lbl_805ED168` table selection's colouring), `eft052_page_put`
 *     88.15.
 *   * 56-86 %: `eft052_page_take` 85.43, `eft052_hold_cursor_step` 83.77, `eft052_hold_row_get` 56.07
 *     (the +0x10 table row lookup's stride addressing).
 *   * the other 72 functions (20488 B) are unwritten, not residual - the two biggest bodies
 *     (`fn_8035A034` 1684 B, `fn_80358B40` 1144 B) are drafted from `tools/m2c` but not yet
 *     transcribed, and `eft052_hold_row_set`'s body is unwritten too.
 *   * DATA: `datagap.py --unit ef/eft052` reports **no ours-extra row** and the expected target-extra
 *     rows for the unwritten part (`.text` 23056/2568 B, extab 520/104 B, extabindex 780/156 B,
 *     `.rela.text` 10392/1176 B, `.relaextabindex` 1560/312 B).  The 4-byte `.sdata2` ours-extra this
 *     unit had at first write was cleared by loading the pooled `0.0f` through its own label
 *     (`lbl_8079B640`, declared never defined) instead of emitting a literal.
 */
/* ---- header inherited from src/enemy/fn_8035E034.cpp (written against its pre-phase-4 range) ---- */
/* enemy/fn_8035E034.cpp - the em033/em035 enemy-program band, `.text` 0x8035E034..0x8035F2B4
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup`: every address here resolves to a `zz_XXXXXXXX_` dump
 * name and a bare `.text` entry in config/RMHE08/symbols.txt, so no real function name survives).
 *
 * WHAT IT IS.  The enemy-side program handlers of the em033 monster (0x8035E034..0x8035F060) and the
 * head of em035 (0x8035F060..0x8035F2B4).  Every function takes the shared `_ENEMY_WORK` record and
 * drives its action/id state; the `.data` program tables `em033_prog_tbl` (0x805ED370) and
 * `em035_prog_tbl` (0x805ED838) list this range's entry points (`fn_8035E034`/`fn_8035E578`/
 * `fn_8035E1E0` in em033, `fn_8035F060`/`fn_8035F174`/`fn_8035F178` in em035), and `lbl_805ED808`
 * mixes `fn_8035F004`/`fn_8035EF50`/`fn_8035EF58` with the registered enemy-band functions
 * `fn_801394D4`..`fn_8013A650`.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable from the
 * range: every `lis`/`addi` pair and every `lbl_` reference in the 16 objects resolves to the
 * `.sdata2` float pool (0x8079B704..0x8079B718) or to a call, never to a source-file-name literal.
 * 2. `dumpmap.py lookup` gives only `zz_XXXXXXXX_` placeholders.  3. The `em033_prog_tbl` /
 * `em035_prog_tbl` names and the `_ENEMY_WORK` record place the unit in the `enemy` module (the
 * program tables sit in the `.data` auto object with the enemy ones, and the range's records are the
 * enemy work block).  The file therefore keeps the map's `fn_8035E034` stem (brief option 4); no
 * name was invented and no module was guessed - the surrounding proposals (0x80358624, 0x8035F2B4)
 * are menu screens, but they are unregistered proposals, not a naming scheme.
 *
 * SEAM.  The brief's range is one `attribute.py` `--max-bytes` run.  `tudiscover.py at 0x8035E034`
 * puts a strong seam at 0x8035F060 (`.sdata2` run jump lbl_8079B710 -> lbl_8079B718) and reports the
 * 2-function match set 0x8035E034..0x8035E578 with the 13-function extension to 0x8035F060; the
 * `em035_prog_tbl` starts at 0x8035F060, so the true boundary is 0x8035F060 (em033|em035), not the
 * range's cap edge.  This file is registered for the whole brief range and both programs are written;
 * a re-cut at 0x8035F060 is a `config_requests` follow-up.
 *
 * LANGUAGE AND SECTIONS.  C++: the map's `mangled_undefined` set is `findInterSection__F...`,
 * `get_now_areano__Fv`, `get_now_mapno__Fv`, `ran_suu__Fl`, `__dl__FPv`, all C++-linkage
 * declarations at global scope.  Every plain `fn_` definition is `extern "C"` so it keeps the map's
 * name (playbook 42); the mangled callees are called through their real signatures (rule 9).  The
 * lib is `enemy` (`cflags_main`, `-Cpp_exceptions on`), and the object carries extab/extabindex
 * (13 records) plus the `.sdata2` pool.
 *
 * STATUS / RESIDUALS (measured with `recompile.py --measure`, official report metric).  12 of the 16
 * bodies are at or above the 80 % bar - `fn_8035E578`, `fn_8035EF50`, `fn_8035F174` are byte-identical;
 * `fn_8035F004` 95.4, `fn_8035EAA0` 95.2, `fn_8035E034` 94.5, `fn_8035E81C` 86.5, `fn_8035F178` 85.8,
 * `fn_8035F060` 85.4, `fn_8035E984` 83.4, `fn_8035EE40` 81.3, `fn_8035EB80` 80.3.  Four are short and are
 * the honest residual:
 *   * `fn_8035E1E0` 79.1 % - the outer switch's case bodies each end in `fn_8012B380(self,5,2,10)` in
 *     retail (920 B); our build shares that tail (756 B), so the diff is switch-body duplication.
 *   * `fn_8035E580` 75.6 % - the roster scan.  Retail's `prev` byte is read before any store
 *     (uninitialised in the object); ours starts it at 0, and the `findInterSection` call's stack
 *     vector is reconstructed, so the loop colours differently (676 vs 668 B).
 *   * `fn_8035ECD8` 78.8 % - retail keeps `(self->act_id & 0xF)` (`clrlwi r0,r4,28`) in every case
 *     body; under our `(u8)(act_id-1)<=3` guard MWCC proves the range and folds the mask, so each id
 *     build is four instructions short (320 vs 360 B).
 *   * `fn_8035EF58` 52.1 % - the branch tree is right; the prologue's callee-saved set and the
 *     `p = *(a+4)` pointer local colour differently (180 vs 172 B), and objdiff reports the whole
 *     straight-line region as a replace.
 */

#include "enemy/fn_8012B380.h" /* fn_8012B380 (rule 2: the owner's header) */
#include "enemy/fn_8012CEB4.h" /* fn_8012CEB4 (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "enemy/enemy_control.h" /* em_spawn_request (the owner's header, rule 2) */
#include "fn_8004CAD8.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "enemy/em_pop.h" /* the roster records and kind searches (the owner's header, rule 1/2) */
#include "enemy/em_model.h"
#include "ef/fn_800CDB2C.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "stage/stg_w.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_8012CEB4_c1 ((void (*)(_ENEMY_WORK*, s16, u32))fn_8012CEB4)

/* ---------------------------------------------------------------------------------------------- *
 * Callees outside this unit.
 * ---------------------------------------------------------------------------------------------- */

/* The three vector helpers the game-root draw layer owns; `include/mh3_pad.h` and `include/ef.h`
 * now spell them with the same record type (`nw4r::math::VEC3*`, docs/plan.md 6.5 rule 11), so no
 * local copy of the declaration is needed. */
extern "C" void fn_8008E8D0(void* a, void* b);
extern "C" void fn_8008DA10(void* a, void* b);
void fn_800513F0(VEC3* v, f32 s);

/* The enemy action helpers whose owner units are unclaimed; their signatures are the call sites'
 * (this range's bracketing registered units name different bands, the rule 2 named gap). */

/* The object the aim writer's first argument points at, and the source block its +0x4 member names
 * (`fn_8035EF58`).  Only the two fields the body touches are named. */
struct EmAimOwner {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ u8* field_0x004;
}; /* size: 0x8 */
struct EmAimSource {
    /* +0x000 */ u8 pad_0x000[0xA];
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 pad_0x00B[0x348 - 0xB];
    /* +0x348 */ nw4r::math::VEC3 field_0x348;
}; /* size: 0x354 */

extern "C" u32 fn_802B0998(u8 index);

/* 0x80295924 - the segment/line intersection test the aiming helpers use; its map name is the C++
 * mangling of exactly this signature (rule 9), so it is declared at C++ scope. */
int findInterSection(VEC3* a, VEC3* b, VEC3* c, u8 d, u32 e, u8 f, u16 g, u8* h);

/* The runtime's scalar deleter; `__dl__FPv` is its mangling. */
void operator delete(void* ptr) throw();

/* This unit's own forward declarations (definitions follow in address order). */
extern "C" void fn_8035EE40(_ENEMY_WORK* self, u8 a, VEC3* out);
extern "C" u8 fn_8035E580(_ENEMY_WORK* self, u8 a);
extern "C" u8 fn_8035E984(u8 a, u8 b);

/* Pool literals (declared, never defined - playbook 29). */
extern "C" f32 lbl_8079B704;
extern "C" f32 lbl_8079B708;
extern "C" f32 lbl_8079B70C;
extern "C" f32 lbl_8079B710;
extern "C" f32 lbl_8079B718;

/* The shared-state accessors. */
u8 get_now_areano();
u8 get_now_mapno();

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035E034 - em033's distance/id query.
 * ---------------------------------------------------------------------------------------------- */
extern "C" u8 fn_8035E034(_ENEMY_WORK* self, u8 mode)
{
    VEC3 a;
    VEC3 b;
    VEC3 c;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    switch (mode) {
    case 0:
        return self->field_0x1E4;
    case 1:
        return self->field_0x33E;
    case 2:
        fn_8035EE40(self, 0, &a);
        subVec3(&c, &a, &self->pos);
        copyVec3(&b, &c);
        return fn_80050F24((const f32*)&b) <= lbl_8079B704;
    case 3:
        fn_8035EE40(self, 1, &a);
        subVec3(&c, &a, &self->pos);
        copyVec3(&b, &c);
        return fn_80050F24((const f32*)&b) <= lbl_8079B708;
    case 4:
        return self->field_0x33F;
    case 5:
        fn_8035EE40(self, 2, &a);
        subVec3(&c, &a, &self->pos);
        copyVec3(&b, &c);
        return fn_80050F24((const f32*)&b) <= lbl_8079B70C;
    case 6:
        return em_roster_kind_aim_pos_get(self->field_0x33B, self->act_id) != 0;
    default:
        return 0;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035E1E0 - em033's action/out-pair selector.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035E1E0(_ENEMY_WORK* self, u8* out_a, u8* out_b)
{
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        switch (self->act_id) {
        case 1:
        case 2:
        case 3:
        case 4:
            *out_a = 0xC;
            *out_b = 0;
            fn_80126324(self, 0, 2, lbl_8079B70C);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            fn_8012B380(self, 5, 2, 10);
            break;
        }
        break;
    case 2:
        switch (self->act_id) {
        case 4:
            *out_a = 0xC;
            *out_b = 0;
            if (fn_802B0998(3) == 1) {
                fn_80126324(self, 1, 3, lbl_8079B70C);
            } else {
                fn_80126324(self, 0, 2, lbl_8079B70C);
            }
            break;
        case 6:
            *out_a = 0xC;
            *out_b = 2;
            fn_80126278(self, (u16)((self->act_id & 0xF) << 8), &self->pos);
            self->pos_0x1BC.y = 0x6C00;
            break;
        case 9:
            *out_a = 0xC;
            *out_b = 0;
            fn_80126324(self, 0, 2, lbl_8079B70C);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            fn_8012B380(self, 5, 2, 10);
            break;
        }
        break;
    case 3:
        switch (self->act_id) {
        case 5:
            *out_a = 0xC;
            *out_b = 0;
            fn_80126324(self, 0, 2, lbl_8079B70C);
            break;
        case 6:
            *out_a = 0xC;
            *out_b = 0;
            fn_80126324(self, 0, 2, lbl_8079B70C);
            break;
        case 9:
            *out_a = 0xC;
            *out_b = 0;
            fn_80126324(self, 0, 2, lbl_8079B70C);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            fn_8012B380(self, 5, 2, 10);
            break;
        }
        break;
    case 5:
        *out_a = 0xC;
        *out_b = 0;
        if (self->act_id == 2) {
            fn_80126324(self, 0, 2, lbl_8079B70C);
        } else {
            fn_8012B380(self, 5, 2, 10);
        }
        break;
    default:
        *out_a = 0xC;
        *out_b = 0;
        fn_8012B380(self, 5, 2, 10);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035E578 - a constant predicate.
 * ---------------------------------------------------------------------------------------------- */
extern "C" u8 fn_8035E578(void)
{
    return 1;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035E580 - the em033 roster scan (select the nearest/valid monster record).
 * ---------------------------------------------------------------------------------------------- */
extern "C" u8 fn_8035E580(_ENEMY_WORK* self, u8 a)
{
    VEC3 v1;
    VEC3 v2;
    u8 buf[0x10];
    u8 best = 0xFF;
    u8 prev = 0;
    u8 n;
    int i;

    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    n = (u8)em_roster_kind_collect(self->act_id, buf, 0x10);
    if (n > 0x10) {
        return 0xFF;
    }
    for (i = 0; i < n; i++) {
        u8 id = buf[i];
        s32 type = (s8)em_roster_kind_field5_get(id, self->act_id);
        if (a == 0) {
            if (type == 5 || type == 1 || type == 0xF) {
                VEC3* p = em_roster_kind_aim_pos_get(id, self->act_id);
                if (p != 0) {
                    u16 ang = em_hit_mask_get(self);
                    int ok = 1;
                    if (findInterSection(&self->pos, p, &v1, 1, 0xFFFF, self->act_id, ang, 0) > 0) {
                        VEC3 v3;
                        subVec3(&v3, &v1, p);
                        copyVec3(&v2, &v3);
                        if (fn_80050F24((const f32*)&v2) > em_roster_record_get(id)->radius_0x1F0) {
                            ok = 0;
                        }
                    }
                    if (ok) {
                        if (best == 0xFF) {
                            best = id;
                        } else if (type == 5) {
                            best = id;
                            prev = (u8)type;
                        } else if (type == 1 && prev != 5 && prev != 1) {
                            best = id;
                            prev = (u8)type;
                        }
                        if (type == 5) {
                            return best;
                        }
                    }
                }
            }
        } else {
            switch ((u8)stage_map_kind_get(get_now_mapno())) {
            case 1:
                if ((u8)(self->act_id - 1) > 3) {
                    return 0xFF;
                }
                break;
            case 2:
                if (self->act_id != 6 && self->act_id != 9) {
                    return 0xFF;
                }
                break;
            case 3:
                if ((u8)(self->act_id - 5) > 1 && self->act_id != 9) {
                    return 0xFF;
                }
                break;
            case 5:
                if (self->act_id != 2) {
                    return 0xFF;
                }
                break;
            default:
                return 0xFF;
            }
            if (type == 4 || type == 0 || type == 0xE) {
                if (em_roster_kind_aim_pos_get(id, self->act_id) != 0) {
                    return id;
                }
            }
        }
    }
    return best;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035E81C - em033's map/area gate over the roster.
 * ---------------------------------------------------------------------------------------------- */
extern "C" u8 fn_8035E81C(u8 a)
{
    u8 buf[0x10];
    u8 n;
    int i;

    switch ((u8)stage_map_kind_get(get_now_mapno())) {
    case 1:
        if ((u8)(get_now_areano() - 1) > 3) {
            return 0;
        }
        break;
    case 2:
        if (get_now_areano() != 6 && get_now_areano() != 9) {
            return 0;
        }
        break;
    case 3:
        if ((u8)(get_now_areano() - 5) > 1 && get_now_areano() != 9) {
            return 0;
        }
        break;
    case 5:
        if (get_now_areano() != 2) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    n = (u8)em_roster_kind_collect(a, buf, 0x10);
    if (n > 0x10) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        s32 type = (s8)em_roster_kind_field5_get(buf[i], a);
        if (type == 5 || type == 1 || type == 0xF) {
            return 1;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035E984 - the same gate for one record id.
 * ---------------------------------------------------------------------------------------------- */
extern "C" u8 fn_8035E984(u8 a, u8 b)
{
    s32 type;

    switch ((u8)stage_map_kind_get(get_now_mapno())) {
    case 1:
        if ((u8)(get_now_areano() - 1) > 3) {
            return 0;
        }
        break;
    case 2:
        if (get_now_areano() != 6 && get_now_areano() != 9) {
            return 0;
        }
        break;
    case 3:
        if ((u8)(get_now_areano() - 5) > 1 && get_now_areano() != 9) {
            return 0;
        }
        break;
    case 5:
        if (get_now_areano() != 2) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    type = (s8)em_roster_kind_field5_get(a, b);
    if (type == 4 || type == 0 || type == 0xE) {
        return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035EAA0 - lock the current target in, or pick a random action.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035EAA0(_ENEMY_WORK* self, u8 a)
{
    u8 id = fn_8035E580(self, 0);

    if (id != 0xFF) {
        self->field_0x33B = id;
        copyVec3(&self->target, em_roster_kind_aim_pos_get(id, self->act_id));
        fn_8012B380(self, 6, 0xFF, 0);
        fn_8013072C(self, 5, 1);
    } else {
        self->field_0x33B = 0xFF;
        if (a == 1) {
            fn_8012CEB4_c1(self, (s16)(ran_suu(0) & 0xF), 0);
            fn_8013072C(self, 0, 0);
        } else {
            fn_8013072C(self, 5, 0);
        }
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035EB80 - keep tracking the cached record, or drop it.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035EB80(_ENEMY_WORK* self)
{
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;

    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    if (fn_8035E984(self->field_0x33B, self->act_id) == 1) {
        VEC3* p = em_roster_kind_aim_pos_get(self->field_0x33B, self->act_id);
        u16 ang = em_hit_mask_get(self);
        int keep = 1;
        if (findInterSection(&self->pos, p, &v1, 1, 0xFFFF, self->act_id, ang, 0) > 0) {
            subVec3(&v3, &v1, p);
            copyVec3(&v2, &v3);
            if (fn_80050F24((const f32*)&v2) > em_roster_record_get(self->field_0x33B)->radius_0x1F0) {
                keep = 0;
            }
        }
        if (keep) {
            copyVec3(&self->target, p);
            fn_8012B380(self, 6, 0xFF, 0);
            self->field_0x33F = 2;
        }
    } else {
        self->field_0x33B = 0xFF;
        fn_8012CEB4_c1(self, (s16)(ran_suu(0) & 0xF), 0);
        self->field_0x33F = 0;
        fn_8013072C(self, 0, 0);
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035ECD8 - sync the aim vector for the current action.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035ECD8(_ENEMY_WORK* self)
{
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        if ((u8)(self->act_id - 1) <= 3) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
        }
        break;
    case 2:
        switch (self->act_id) {
        case 4:
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
            break;
        case 6:
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
            break;
        case 9:
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
            break;
        default:
            break;
        }
        break;
    case 3:
        if ((u8)(self->act_id - 5) <= 1 || self->act_id == 9) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
        }
        break;
    case 5:
        if (self->act_id == 2) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
        }
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035EE40 - the small per-field aim dispatcher.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035EE40(_ENEMY_WORK* self, u8 a, VEC3* out)
{
    switch (a) {
    case 0:
        if ((u8)stage_map_kind_get(self->field_0x1E0) == 3 && self->act_id == 6) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 7), out);
        }
        break;
    case 1:
        if ((u8)stage_map_kind_get(self->field_0x1E0) == 3 && self->act_id == 6) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 8), out);
        }
        break;
    case 2:
        if ((u8)stage_map_kind_get(self->field_0x1E0) == 3 && self->act_id == 6) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 5), out);
        }
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035EF50 - a one-line forwarder.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035EF50(_ENEMY_WORK* self)
{
    fn_8013A654(self, 1);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035EF58 - the effect/target aim writer.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035EF58(void* a, void* b, void* c, void* d, u32 e, u8* f)
{
    VEC3 v;
    struct EmAimSource* p = (struct EmAimSource*)((struct EmAimOwner*)a)->field_0x004;

    VEC3_ctor(&v);
    if (f[4] == 0xFF) {
        if (e - 5 <= 1) {
            if (p->field_0x00A == 1) {
                fn_8008DA10(b, &v);
                fn_800513F0(&v, lbl_8079B710);
                fn_8008E8D0(b, &v);
            }
        } else if (e == 0x17) {
            fn_8008E8D0(b, &p->field_0x348);
        }
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F004 - the em033 record's release hook.
 * ---------------------------------------------------------------------------------------------- */
extern "C" _ENEMY_WORK* fn_8035F004(_ENEMY_WORK* p, u16 a)
{
    if (p != 0) {
        fn_8013918C(p, 0);
        if ((s16)a > 0) {
            operator delete(p);
        }
    }
    return p;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F060 - the em035 run-state constructor.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035F060(_ENEMY_WORK* self, u8 a)
{
    self->init_0x320.field_0x328 = 0x708;
    self->init_0x320.field_0x32A = 0xB4;
    self->bytes_0x32C.field_0x32C = 0;
    self->bytes_0x32C.field_0x32D = 0;
    switch (a) {
    case 2:
        if (self->field_0x00A == 2) {
            em_fall_height_get(self);
            em_fall_start(self);
            fn_80128A8C(self, 3, 0);
        } else {
            fn_80128A8C(self, 1, 0);
        }
        self->field_0x1CC = lbl_8079B718;
        if (self->run_flags_0xB12 & 1) {
            copyVec3(&self->pos, (VEC3*)&self->run_0xB04[0]);
            self->pos_0x1BC.y = self->run_angle_0xB10;
        } else {
            self->run_angle_0xB10 = (u16)self->pos_0x1BC.y;
            copyVec3((VEC3*)&self->run_0xB04[0], &self->pos);
        }
        em_state_refresh(self);
        break;
    case 0:
        if (self->field_0x00A != 0) {
            copyVec3(&self->pos, (VEC3*)&self->run_0xB04[0]);
            self->pos_0x1BC.y = self->run_angle_0xB10;
        }
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F174 - an empty virtual slot.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035F174(void)
{
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F178 - em035's effect-summon hook.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035F178(_ENEMY_WORK* self, u8 a, u8 b)
{
    u32 s[3];

    switch (a) {
    case 1:
        if (b == 1) {
            s[0] = 0;
            s[1] = (u32)((ran_suu(0) & 0xFF) << 8);
            s[2] = 0;
            em_spawn_request(self->field_0x01A, 0x1A, 4, self->act_id, 0xFF, 1, 0x20, 1,
                        0xFF, &self->pos, (s32)s);
            self->init_0x320.field_0x32A = 0xF0;
        }
        break;
    case 3:
        if (b == 1) {
            s[0] = 0;
            s[1] = (u32)((ran_suu(0) & 0xFF) << 8);
            s[2] = 0;
            em_spawn_request(self->field_0x01A, 0x1A, 5, self->act_id, 0xFF, 1, 0x20, 1,
                        0xFF, &self->pos, (s32)s);
            self->init_0x320.field_0x32A = 0xF0;
        }
        break;
    default:
        break;
    }
}

