/* enemy/fn_80147CE0.cpp - the enemy action-handler band, `.text` 0x80147CE0..0x80149D6C (36 functions).
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup`: every address resolves to a `zz_XXXXXXXX_` dump name and a
 * bare `.text` entry in config/RMHE08/symbols.txt, so no real name exists to use).
 *
 * What it is.  `_ENEMY_WORK` action handlers of the enemy band, the same family as the neighbouring
 * units `enemy/fn_8014A1BC.c` (0x8014A1BC above) and the action band below 0x80147CE0:
 *
 *   * `fn_80147CE0(self, arg)` is the per-tick entry of one action (the same opening as
 *     `enemy/fn_80176C58.cpp`'s `fn_80176C58`): it attaches the 12-byte vtable helper through
 *     `fn_801390FC` when `fn_801391E8` reports none, advances the effect timer for the `2` step and
 *     spawns the 0x1C72 effect (`fn_801057A4`).
 *   * the long state-machine family - `fn_80148248`, `fn_801482D0`, `fn_8014834C`, `fn_801483C8`,
 *     `fn_80148448`, `fn_80148528`, `fn_801486A4`, `fn_80148720`, `fn_8014879C`, `fn_80148828`,
 *     `fn_80148BD0`, `fn_80148E6C`, `fn_80148F88`, `fn_801492B8`, `fn_801498C8`, `fn_80149A08`,
 *     `fn_80149AFC`, `fn_80149C58` - each opens with `switch (self->state)` (+0x05), state 0 arming the
 *     action (`fn_80130478` + a motion setter + `field_0x020` timer) and state 1 waiting for the motion
 *     to end (`fn_8012F93C`) before closing it (`fn_80127F48`/`fn_80127FE4`/`fn_801280AC`).
 *   * three sub-state dispatchers keyed on `_ENEMY_WORK::state_sub` (+0x1E6) - `fn_801484D4`,
 *     `fn_80149004`, `fn_80149814` (11- and 14-entry jump tables) - and `fn_80147F48`, the 14-way
 *     dispatch on the action argument with its own `jumptable_805A20E0`.
 *   * the frame helpers `fn_80147E68` (the action-cancel/transition step), `fn_801481D8` (the 50-frame
 *     counter), `fn_801481FC` and `fn_80147F00` (the motion-selection tail).
 *   * the second action's own step family - `fn_80149068`, `fn_80149170`, `fn_80149214`, `fn_801492B8`,
 *     `fn_801493A8` (the six-state spawn body with the shell callback), `fn_80149788` (the spawn
 *     record's position reset), `fn_801497BC` and the joint/effect helpers `fn_801498C8`,
 *     `fn_80149A08`, `fn_80149AFC`, `fn_80149C54`, `fn_80149C58`.
 *
 * Module and name (brief section 2, in evidence order).
 *   1. No `__FILE__` string is reachable from this range: the only data its code builds are the
 *      `.sdata2` pool, the three `jumptable_*`, the two `.rodata` tables `lbl_8056F8E0`/`lbl_8056F920`
 *      and the `.data` labels - no source-file name.
 *   2. `python tools/symbols/dumpmap.py lookup` answers `zz_0147ce0_` for the range's addresses (a
 *      `zz_` placeholder is not evidence).
 *   3. The code is enemy-band: every function takes a `_ENEMY_WORK*` (pinned by the callee
 *      `em_frame_check__FP11_ENEMY_WORKUsff`) and calls the `em_*`/`fn_8012xxxx` enemy helpers; both
 *      bracketing registered units are `enemy` (`enemy/fn_8013F764.cpp` below, `enemy/fn_80149D6C.c`
 *      above) and the band's naming scheme is the map's own `fn_XXXXXXXX` stem.
 *   The file therefore keeps the map stem (brief option 4); no name was invented.
 *
 * Seam (the left edge is unproven, and said so).  The right edge at 0x80149D6C is evidence: the
 * registered `enemy/fn_80149D6C.c` starts exactly there and this unit's `extabindex` run ends exactly
 * at 0x8002871C, where that unit's begins.  The LEFT edge at 0x80147CE0 is NOT evidence - it is the
 * `--max-bytes` cut of the discovery queue (`MAX_BYTES_DEFAULT` = 27436): the maximal unclaimed run is
 * 0x801411B8..0x80149D6C (35764 B), so `tools/units/attribute.py` cut it at the last function boundary
 * under the cap, and the piece below is `proposal/801411B8` (27432 B, its `tu` verdict `merged`).  The
 * two pieces very probably belong to one TU:
 *   * `proposal/801411B8` carries the only `__FILE__` evidence in the run - `enemy_control.cpp`,
 *     referenced from 0x801411DC/0x80141258 - and this piece's code references no source name at all,
 *     which is what a continuation of that file looks like;
 *   * the two `.data` tables this range's constructors install (`lbl_805A1368` for `fn_80147E2C`,
 *     `lbl_805A4478` for `fn_80147DF0`, both 0x30 B) are vtables whose entries point at
 *     0x8013918C/0x801394D4/0x80139858/... - virtual functions of the range BELOW - so the classes
 *     this unit constructs are declared there;
 *   * the `.sdata2` pool runs continuously across the cut (0x80796DC8..0x80796EE0), and
 *     `enemy/fn_80176C58.cpp` (a landed unit 0x2A000 B above) calls this unit's `fn_80147E2C` as the
 *     base constructor of its own 12-byte helper.
 * The registration stays separate because that is what this worker was briefed to register; the merge
 * into one `enemy/enemy_control.cpp` is the orchestrator's re-split decision and is requested in the
 * outbox (`config_requests[0]`).
 *
 * Language.  Four of the unit's callees are mangled (`em_frame_check__FP11_ENEMY_WORKUsff`,
 * `setVector3__FPQ34nw4r4math4VEC3fff`, `calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3`,
 * `__nw__FUl`) and rule 9 forbids spelling a mangling at the call site, so each is declared at C++
 * scope with the signature its mangling encodes and the file is C++ - the same evidence
 * `langcheck`'s region verdict (C++, medium) rests on.  Every definition keeps the map's plain
 * `fn_XXXXXXXX` name, i.e. the `extern "C"` block below.
 *
 * Types.  `_ENEMY_WORK` is the shared record `include/enemy/ENEMY_WORK.h` owns (rule 1); this unit
 * added the fields it measured to that header (+0x1FB, +0x314/+0x318/+0x324, +0x354..+0x358, +0x38B,
 * +0x464).  Two records this unit uses are private copies of records another unit also carries and are
 * a rule-1 follow-up (outbox `config_requests[1]`): the 12-byte vtable helper
 * (`Helper_80147CE0` here, `Helper_80176E50` in `enemy/fn_80176C58.cpp`) and the 0x18-byte spawn
 * record (`EmSpawnRec` here, `ShellParams` in `enemy/fn_8014A1BC.c`) with the shell callback table
 * (`EmShellSetFunc` here, `ShellSetFunc` there).
 *
 * Sections.  Besides `.text` the unit owns `extab` 0x8000D9DC..0x8000DACC and `extabindex`
 * 0x800285B4..0x8002871C, both taken from the target's OWN per-function entry boundaries (30 of the 36
 * functions carry an entry): the piece below ends its `extabindex` exactly at 0x800285B4 and
 * `enemy/fn_80149D6C.c` begins its own exactly at 0x8002871C.  Measured: the object's `extab` is
 * 0xF0 and its `extabindex` 0x168, byte-for-byte the totals of the 36 target objects.  No `.ctors`
 * word, and no data - the `.sdata2`/`.data`/`.rodata` runs the range references are shared pools and
 * are NOT claimed (docs/plan.md 8.4).
 *
 * Status (measured in this worktree with `python tools/units/recompile.py enemy/fn_80147CE0.cpp
 * --measure <symbol>`, against the split's own single-symbol objects).  28 of the 36 bodies are
 * byte-identical (100.00 %); every function is above the 80 % bar; our `.text` is 0x2064 B against the
 * target's 0x208C B.  By size:
 *   * 100.00: fn_80147DF0, fn_80147E2C, fn_80147F00, fn_801481D8, fn_801481FC, fn_80148248,
 *     fn_801482D0, fn_8014834C, fn_801483C8, fn_80148448, fn_801484D4, fn_80148528, fn_801486A4,
 *     fn_80148720, fn_8014879C, fn_80148BD0, fn_80148E6C, fn_80148F88, fn_80149004, fn_80149170,
 *     fn_80149214, fn_80149788, fn_801497BC, fn_80149814, fn_801498C8, fn_80149A08, fn_80149AFC,
 *     fn_80149C54.
 *   * fn_80149C58 99.86, fn_80147E68 98.42, fn_80148828 96.40, fn_801493A8 93.99, fn_80147CE0 92.43,
 *     fn_80149068 90.91, fn_801492B8 90.00, fn_80147F48 89.34.
 *
 * Residuals, by measurement (each is a codegen shape, not comprehension):
 *   * ARGUMENT EVALUATION ORDER (fn_80147CE0 92.43, fn_80149068 90.91, fn_801492B8 90.00).  The
 *     target evaluates the FLOAT argument of `fn_8012FCC4(self, 0, 0.0f)` / `fn_80134004(self, 0, x)`
 *     BEFORE the integer one (`lfs f1,pool` then `li r4,imm`); MWCC evaluates arguments in
 *     declaration order, so that order is what a declaration of `(self, f32, s32)` produces.
 *     Measured directly: with the band header's `(self, s32, f32)` the same call site emits
 *     `li r4,imm` then `lfs f1,pool` (both orders reproduce the same ABI - one FPR and one GPR slot -
 *     so this is a declaration-order artefact of the original TU, not a wrong call).  Fixing it means
 *     flipping the parameter order in `include/unsplit/enemy.h`'s C++ view of `fn_8012FCC4`/
 *     `fn_80134004` AND the call sites in `src/enemy/fn_801550FC.cpp` (another unit's file), so it is
 *     recorded rather than done here.
 *   * MASKED-VALUE CSE (fn_80147CE0 92.43).  `(arg & 0xFF)` used twice CSEs into one register
 *     (`clrlwi r31,r31,24` reused) where the target re-masks into a fresh scratch each time
 *     (`clrlwi r0,r31,24` twice).  `(u8)arg` per use reproduces the fresh masks but flips the
 *     comparisons to `cmplwi`; the target needs `cmpwi` for `== 2` and `cmpwi` for `!= 0`, which only
 *     the signed masked expression produces.  Both spellings measured; the signed one is kept.
 *   * RANGE-TEST LOWERING (fn_80147F48 89.34, target 0x290 B / ours 0x278 B).  Case 3's adjacent
 *     values 22/23 are a register range test in the target (`subi r0,r4,0x16; cmplwi r0,1; ble`) and
 *     this compiler lowers the same `switch` case pair as two compares.  An explicit
 *     `if ((u8)sub == 22 || (u8)sub == 23)` if-chain was measured too (86.60 - the four re-maskings
 *     cost more than the range test gains), so the switch is kept.  The same function's case 10 is a
 *     7-value binary-search tree in the target and comes out equivalent here; the remaining rows are
 *     the `li r6,0`/`r0` zero-source colouring.
 *   * SHELL CALLBACK SETUP ORDER (fn_801493A8 93.99, target 0x3E0 B / ours 0x3D8 B).  The target
 *     materialises the 4th argument (`shell_set_func_ptr`) after r3/r4/r5 and only then loads the
 *     callee from its +0x3C slot; ours loads the table first.  Also measured: `u32 v` (kept - it is
 *     what makes the two range tests `cmplwi`, as the target has them) vs `s32 v` (93.75) vs `u8 v`
 *     (93.31, and it re-masks at each use), and `s32 arg2` + `(arg2 & 0xFF)` for the `== 2`/`== 4`
 *     compares (kept - the target's `cmpwi`).
 *   * UNINITIALISED LOCAL, REPRODUCED (fn_801493A8).  The target reads the case-2 local
 *     (`r30 = (u8)arg2 - 1`) from case 3's dispatch (`clrlwi r0,r30,24`), i.e. the original code reads
 *     an uninitialised automatic in case 3; the compiler says so (`variable 'v' is not initialized
 *     before being used`) and the source keeps the shape because it is what the target's instructions
 *     are.  Declaring `v` at function scope is required for that (`u8 v = 0;` would add an
 *     instruction the target does not have).
 *   * fn_80148828 96.40 (target 0x3A8 B / ours 0x3A0 B) and fn_80147E68 98.42: a handful of rows each,
 *     no shape named - both are above the bar and their bodies are complete.
 *
 * Source shapes worth keeping (each measured):
 *   * `(u8)arg` / `(u8)sub` PER USE, not `(arg & 0xFF)`: `fn_80147F48`'s three `clrlwi r0,r4,24`
 *     masks and the `li r6,0` zero source only appear with the per-use casts; the masked-expression
 *     spelling CSEs into one register and moves the zero to r0 (88.7 -> 89.3 on the same body).
 *   * `(s32)` parameters for `(arg & 0xFF)`: a signed parameter makes `arg & 0xFF` an `int`, which is
 *     what turns `cmplwi` into the target's `cmpwi` (`fn_80149068` 89.09 -> 90.91, `fn_801492B8`
 *     88.00 -> 90.00).
 *   * `switch` cases that share a body are written ONCE and listed (`case 8: case 9:`), and the
 *     sub-state dispatchers list the arguments in the target's order (`fn_801497BC(self, 0, 1)` for
 *     sub-state 6, not `(self, 1, 0)`).
 *   * `#pragma peephole off` is load-bearing for the whole unit: retail keeps the unfused
 *     `clrlwi`/`rlwinm` + `cmpwi` pairs the `-O3` peephole folds into `clrlwi.` (the same finding as
 *     `enemy/fn_8012BA00.c`, `enemy/fn_8012BDF4.cpp` and `enemy/fn_80176C58.cpp`).
 *
 * Declarations.  Rule 2 sent every callee to the header that owns it: this unit's own entry points are
 * published in `include/enemy/fn_80147CE0.h` (moved out of `include/unsplit/enemy.h`, whose old-style
 * spellings the C consumers keep), and `fn_80154CA4`/`fn_801545B8` (enemy band, unowned) went the
 * other way into `include/unsplit/enemy.h`.  The band header's `fn_80128A8C`, `fn_8012933C`,
 * `fn_80127FE4`, `fn_801280AC` and `fn_80128BF8` moved to their owner's header
 * (`include/enemy/fn_801251D0.h`), and `fn_80056A54`'s owner (`src/draw_shape.cpp`) got its first
 * header (`include/draw_shape.h`).  `include/fn_8004CAD8.h` gained `calcVecAng2` (C++ linkage, the
 * mangling the map names) and `include/ef/eft007.h`/`eft009.h` gained the C++ views of
 * `fn_801039B0`/`fn_801049D0` (one view per TU - declaring both spellings is `(10197) illegal
 * function overloading`, measured).
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit enemy/fn_80147CE0.cpp`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80138074.h"
#include "ef.h"
#include "ef/fn_80105314.h"
#include "ef/eft007.h"
#include "ef/eft009.h"
#include "sound/fn_800DD1F0.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "enemy/fn_80147CE0.h"
#include "sys_mem.h"
#include "fn_8004CAD8.h"
#include "draw_shape.h"

#pragma peephole off

/* ------------------------------------------------------------------------------------------------ *
 * the 12-byte vtable helper the action allocates
 * ------------------------------------------------------------------------------------------------ */

/* The helper `fn_80147CE0` allocates and `fn_80147DF0` constructs: a 12-byte record whose +0x00 word
 * is the address of a `.data` function/parameter table.  Same record as `enemy/fn_80176C58.cpp`'s
 * private `Helper_80176E50` (that unit's constructor `fn_80176E50` calls this unit's `fn_80147E2C`,
 * and the allocation is the same `operator new(0xC)`); the shared home for both is a rule-1 follow-up.
 * size: 0xC (traced from the `operator new(0xC)` call in `fn_80147CE0`). */
typedef struct Helper_80147CE0 {
    /* +0x0 */ void* tbl;
    /* +0x4 */ u32 unused_0x4;
    /* +0x8 */ u32 unused_0x8;
} Helper_80147CE0;

/* The two `.data` tables the constructors install (`lbl_805A1368` is the base `fn_80147E2C` sets,
 * `lbl_805A4478` the derived one `fn_80147DF0` overrides it with; both are 0x30 B). */
extern "C" u8 lbl_805A1368[];
extern "C" u8 lbl_805A4478[];

/* ------------------------------------------------------------------------------------------------ *
 * callees whose owning unit is not registered and whose address band names different modules (rule 2's
 * documented gap: the declaration has no sound header to move to), declared here as the landed
 * `enemy/fn_8014A1BC.c` and `enemy/fn_80177608.cpp` do
 * ------------------------------------------------------------------------------------------------ */

/* 0x80304508 - between `ai/fn_802D0DCC.c` and `ef/fn_803066F0.c`: `(self, u32, u32, void*, f32)`. */
extern "C" void fn_80304508(_ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s);
/* 0x802B9574 - between `stage/fn_802B2978.c` and `ai/fn_802D0DCC.c`: one byte argument. */
extern "C" void fn_802B9574(u32 a);
/* 0x803B9BA0 - between `hud/fn_80324F7C.c` and `Network/NetworkWiiMediator.c`: `(self, void*, s32)`. */
extern "C" void fn_803B9BA0(_ENEMY_WORK* self, void* pos, s32 value);

/* ------------------------------------------------------------------------------------------------ *
 * pooled data owned by other units: declared, never defined (playbook 29)
 * ------------------------------------------------------------------------------------------------ */

/* the `.sdata2` pool this band's float work reads (values measured from the DOL) */
extern f32 lbl_80796E18; /* 5000.0 */
extern f32 lbl_80796E1C; /* 0.0 */
extern f32 lbl_80796E20; /* 1.0 */
extern f32 lbl_80796E24; /* -50.0 */
extern f32 lbl_80796E28; /* 15.0 */
extern f32 lbl_80796E2C; /* 0.9 */
extern f32 lbl_80796E30; /* -1.0 */
extern f32 lbl_80796E34; /* 110.0 */
extern f32 lbl_80796E38; /* -20.0 */
extern f32 lbl_80796E3C; /* 80.0 */
extern f32 lbl_80796E40; /* 112.0 */
extern f32 lbl_80796E44; /* 214.0 */
extern f32 lbl_80796E48; /* 52.0 */
extern f32 lbl_80796E4C; /* 92.0 */
extern f32 lbl_80796E50; /* 262.0 */
extern f32 lbl_80796E54; /* -30.0 */
extern f32 lbl_80796E58; /* 70.0 */
extern f32 lbl_80796E5C; /* 96.0 */
extern f32 lbl_80796E60; /* 50.0 */
extern f32 lbl_80796E64; /* 1.4 */
extern f32 lbl_80796E68; /* 136.0 */
extern f32 lbl_80796E6C; /* 152.0 */
extern f32 lbl_80796E70; /* 344.0 */
extern f32 lbl_80796E74; /* 368.0 */
extern f32 lbl_80796E78; /* 156.0 */
extern f32 lbl_80796E7C; /* 190.0 */
extern f32 lbl_80796E80; /* 386.0 */
extern f32 lbl_80796E84; /* 0.6 */
extern f32 lbl_80796E88; /* 398.0 */
extern f32 lbl_80796E8C; /* 426.0 */
extern f32 lbl_80796E90; /* 458.0 */
extern f32 lbl_80796E94; /* 478.0 */
extern f32 lbl_80796E98; /* 0.8 */
extern f32 lbl_80796E9C; /* 434.0 */
extern f32 lbl_80796EA0; /* 12.0 */
extern f32 lbl_80796EA4; /* 16.0 */
extern f32 lbl_80796EA8; /* 40.0 */
extern f32 lbl_80796EAC; /* 46.0 */
extern f32 lbl_80796EB0; /* 0.7 */
extern f32 lbl_80796EB4; /* 66.0 */
extern f32 lbl_80796EB8; /* 72.0 */
extern f32 lbl_80796EBC; /* 76.0 */
extern f32 lbl_80796EC0; /* -500.0 */
extern f32 lbl_80796EC4; /* 700.0 */
extern f32 lbl_80796EC8; /* 6.0 */
extern f32 lbl_80796ECC; /* 0.5 */
extern f32 lbl_80796ED0; /* 360.0 */
extern f32 lbl_80796ED4; /* 65536.0 */
extern f32 lbl_80796ED8; /* 60.0 */
extern f32 lbl_80796EDC; /* 0.04 */

/* the two 0x40-byte `.rodata` tables the joint-lookup family indexes */
extern "C" u8 lbl_8056F8E0[];
extern "C" u8 lbl_8056F920[];
/* the two `.data` tables `fn_80135644`/`fn_801354F4` read */
extern "C" u8 lbl_805A1DA0[];
extern "C" u8 lbl_805A1F70[];
/* The shell callback table `shell_set_func_ptr` points at; only its +0x3C entry is used by this unit
 * (the entry `fn_801493A8` calls as `(self, &rec, mode, shell_set_func_ptr)`).  Same record as
 * `enemy/fn_8014A1BC.c`'s private `ShellSetFunc` (rule-1 follow-up: the two copies should move to one
 * header). size: 0x40 */
typedef struct EmShellSetFunc {
    /* +0x00 */ u8 unused_0x00[0x3C];
    /* +0x3C */ void (*field_0x3c)(_ENEMY_WORK* self, void* params, u32 mode, void* table);
} EmShellSetFunc;

/* the table pointer itself (`.sbss`, no registered range, so rule 2 leaves the declaration here) */
extern "C" EmShellSetFunc* shell_set_func_ptr;
/* ------------------------------------------------------------------------------------------------ *
 * this unit's own forward declarations
 * ------------------------------------------------------------------------------------------------ */

extern "C" Helper_80147CE0* fn_80147DF0(Helper_80147CE0* self);
extern "C" void fn_80147F00(_ENEMY_WORK* self, u32 arg);

/* ------------------------------------------------------------------------------------------------ *
 * functions, in address order
 * ------------------------------------------------------------------------------------------------ */

/* The action's per-tick entry: attach the vtable helper when the enemy has none, advance the effect
 * timer for the "hit" step, arm the two frame counters and spawn the 0x1C72 effect. */
extern "C" void fn_80147CE0(_ENEMY_WORK* self, s32 arg) {
    VEC3 v;
    Helper_80147CE0* helper;

    VEC3_ctor(&v);
    if ((arg & 0xFF) == 2) {
        self->pos.y += lbl_80796E18;
        fn_80130248(self);
        fn_801305C4(self);
        fn_80128A8C(self, 0, 3);
    }
    if ((arg & 0xFF) != 0) {
        fn_8012FCC4(self, 0, lbl_80796E1C);
        fn_8012FCC4(self, 10, lbl_80796E20);
    }
    if (fn_801391E8(self) == 0) {
        helper = (Helper_80147CE0*)operator new(0xC);
        if (helper != 0) {
            fn_80147DF0(helper);
        }
        fn_801390FC(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, lbl_80796E1C, lbl_80796E24, lbl_80796E28);
        fn_801057A4(self, 27, &v, lbl_80796E2C, 7282);
    }
}

/* The derived helper's constructor: run the base and override the table with its own. */
extern "C" Helper_80147CE0* fn_80147DF0(Helper_80147CE0* self) {
    fn_80147E2C(self);
    self->tbl = lbl_805A4478;
    return self;
}

/* The helper's base constructor (the engine's own joint-table install, then the base table).  The
 * signature is the one `include/unsplit/enemy.h` publishes for the consumer
 * `enemy/fn_80176C58.cpp`; the body's trailing `mr r3,r31` is the target's `return self`. */
extern "C" void* fn_80147E2C(void* self) {
    Helper_80147CE0* helper = (Helper_80147CE0*)self;

    fn_800E3B2C((MHchar*)self);
    helper->tbl = lbl_805A1368;
    return helper;
}

/* One action-cancel/transition step: on a `2` command byte, map the state byte through the three
 * sub-steps (`fn_8012ECF0` reporting the motion as finished) and write the new state. */
extern "C" void fn_80147E68(_ENEMY_WORK* self, u8* in, u8* out) {
    if (in[0] == 2) {
        switch (out[0]) {
          case 0:
            if (fn_8012ECF0() == 1) {
                out[0] = 3;
            }
            break;
          case 7:
            if (fn_8012ECF0() == 1) {
                out[0] = 8;
            }
            break;
          case 9:
            if (fn_8012ECF0() == 1) {
                out[0] = 10;
            }
            break;
        }
    }
}

/* The motion-selection tail: `fn_8012C220`/`fn_8012C4E8` with the area byte and the position. */
extern "C" void fn_80147F00(_ENEMY_WORK* self, u32 arg) {
    u32 id = self->team == 1 ? 2 : 1;

    if ((arg & 0xFF) == 0) {
        fn_8012C220((u8)id, self->area_no);
    } else {
        fn_8012C4E8((u8)id, self->area_no, 100, &self->pos, lbl_80796E30);
    }
}

/* The 14-way dispatch on the action argument: clears the action's flag bytes and runs the state-family
 * steps for the argument values 1/3/4/7/8/9/10/13 (the rest return). */
extern "C" void fn_80147F48(_ENEMY_WORK* self, u32 arg, u32 sub) {
    self->field_0x38B = 0;
    if ((u8)arg != 0) {
        self->field_0x356 = 0;
    }
    if ((u8)arg != 3 || (u8)sub != 11) {
        self->field_0x358 = 0;
    }
    if ((u8)arg > 13) {
        return;
    }
    switch ((u8)arg) {
      case 1:
        switch ((u8)sub) {
          case 3:
            self->field_0x356 = 1;
            break;
          case 5:
            fn_80130F74(self);
            break;
          case 9:
            if (fn_8012ECF0() == 1) {
                fn_80147F00(self, 0);
            }
            fn_80147F00(self, 1);
            break;
          case 10:
            fn_801376B4(self);
            break;
        }
        break;
      case 3:
        switch ((u8)sub) {
          case 9:
            self->field_0x1FB = 1;
            break;
          case 14:
          case 22:
          case 23:
            self->field_0x38B = 1;
            self->field_0x358 = 1;
            break;
        }
        break;
      case 4:
        switch ((u8)sub) {
          case 1:
            self->field_0x356 = 1;
            break;
          case 11:
            if (fn_8012ECF0() == 1) {
                fn_80147F00(self, 0);
            }
            fn_80147F00(self, 1);
            break;
        }
        break;
      case 7:
        switch ((u8)sub) {
          case 5:
          case 20:
          case 31:
          case 33:
          case 45:
            self->field_0x356 = 1;
            break;
        }
        break;
      case 8:
      case 9:
        self->field_0x356 = 1;
        break;
      case 10:
        switch ((u8)sub) {
          case 202:
            fn_80147F00(self, 0);
            fn_80147F00(self, 1);
            fn_80135C5C(self, 0, 0);
            break;
          case 26:
          case 27:
          case 35:
          case 126:
          case 127:
          case 182:
            fn_80147F00(self, 0);
            fn_80147F00(self, 1);
            break;
        }
        break;
      case 13:
        if ((u8)sub == 7) {
            self->field_0x357 = 1;
        }
        break;
    }
}

/* The 50-frame action counter: tick `field_0x354` and wrap it. */
extern "C" void fn_801481D8(_ENEMY_WORK* self) {
    if (++self->field_0x354 >= 50) {
        self->field_0x354 = 0;
    }
}

/* The action's opening step. */
extern "C" void fn_801481FC(_ENEMY_WORK* self) {
    fn_80130248(self);
    fn_801305C4(self);
    fn_80128AAC(self, 3, 11);
    fn_80133BB4(self);
}

/* The action's first step: arm the motion `fn_8012F62C(self, 1, 10, 0)` with a 150-frame timer and
 * close the action when it runs out. */
extern "C" void fn_80148248(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        self->timer_0x020 = 150;
        fn_8012F62C(self, 1, 10, 0);
        break;
      case 1:
        if (--self->timer_0x020 < 0) {
            fn_80127F48(self);
        }
        break;
    }
}

/* Arm the motion `fn_8012F62C(self, 2, 10, 0)` and close the action when `fn_8012F93C` reports it
 * finished. */
extern "C" void fn_801482D0(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 2, 10, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The same two-step shape with the motion `fn_8012F62C(self, 17, 6, 0)`. */
extern "C" void fn_8014834C(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 17, 6, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The same two-step shape with the motion `fn_8012F62C(self, 26, 6, 0)` and the second action-end tail
 * (`fn_80127FE4`). */
extern "C" void fn_801483C8(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F62C(self, 26, 6, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127FE4(self);
        }
        break;
    }
}

/* The same shape with `fn_80154CA4`'s clamp on both steps and `fn_801280AC` as the end tail. */
extern "C" void fn_80148448(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 3);
        fn_80154CA4(self);
        fn_8012F62C(self, 27, 6, 0);
        break;
      case 1:
        fn_80154CA4(self);
        if (fn_8012F93C(self) == 1) {
            fn_801280AC(self);
        }
        break;
    }
}

/* The sub-state dispatcher: sub-states 0/4 share the first step, 1/2/3 the three motion variants and
 * 6 the last one. */
extern "C" void fn_801484D4(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_80148248(self);
        break;
      case 1:
        fn_801482D0(self);
        break;
      case 2:
        fn_8014834C(self);
        break;
      case 3:
        fn_801483C8(self);
        break;
      case 4:
        fn_80148248(self);
        break;
      case 6:
        fn_80148448(self);
        break;
    }
}

/* The action's two-step body: arm `fn_8012F5B8(self, 5, 10, 0)` and the 19-frame sub-timer, then run
 * the three `em_frame_check` windows (the 110/0 pair spawning the effect, the 112/214 pair with the
 * 8-frame modulo on `timer_0x020`) before the action-end test. */
extern "C" void fn_80148528(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 5, 10, 0);
        fn_80129668(self, 0, 19);
        self->timer_0x020 = 0;
        break;
      case 1:
        if (em_frame_check(self, 0, lbl_80796E34, lbl_80796E1C) == 1) {
            fn_8012933C(self, 1, 8, 5);
            fn_80056A54((u32)self, 27, 10);
        }
        setVector3(&v, lbl_80796E1C, lbl_80796E38, lbl_80796E3C);
        if (em_frame_check(self, 0, lbl_80796E34, lbl_80796E1C) == 1) {
            fn_80304508(self, 0, 26, &v, lbl_80796E20);
        }
        if (em_frame_check(self, 3, lbl_80796E40, lbl_80796E44) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                fn_80304508(self, 1, 26, &v, lbl_80796E20);
            }
            self->timer_0x020++;
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The same two-step shape with `fn_8012F5B8(self, 18, 10, 0)`. */
extern "C" void fn_801486A4(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 18, 10, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The same two-step shape with `fn_8012F5B8(self, 124, 2, 0)`. */
extern "C" void fn_80148720(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 124, 2, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The same shape with the five-argument motion setter `fn_8012F504(self, 2, 50, 0, 1)`. */
extern "C" void fn_8014879C(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F504(self, 2, 50, 0, 1);
        break;
      case 1:
        if (em_frame_check(self, 1, lbl_80796E48, lbl_80796E1C) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The action's long body: `fn_8012CF20` on the opening call, a 300-frame timer armed with
 * `fn_8012F5B8(self, 19, 6, 0)`, then (on `arg == 1`) six `em_frame_check` windows whose hits spawn the
 * two effects through `fn_80304508`/`fn_801049D0`; when the timer runs out the action is released
 * through `fn_80128A14(self, 1, 8)` (or 5 on the other argument). */
extern "C" void fn_80148828(_ENEMY_WORK* self, u32 arg) {
    VEC3 v;

    VEC3_ctor(&v);
    if ((arg & 0xFF) == 0) {
        fn_8012CF20(self);
    }
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 19, 6, 0);
        self->timer_0x020 = 300;
        break;
      case 1:
        if ((arg & 0xFF) == 1) {
            if (em_frame_check(self, 0, lbl_80796E4C, lbl_80796E1C) == 1 ||
                em_frame_check(self, 0, lbl_80796E50, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                fn_80304508(self, 51, 26, &v, lbl_80796E20);
            }
            if (em_frame_check(self, 0, lbl_80796E5C, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E1C, lbl_80796E60);
                fn_801049D0(self, 26, 47, 0, &v, lbl_80796E64);
            }
            if (em_frame_check(self, 3, lbl_80796E68, lbl_80796E6C) == 1 ||
                em_frame_check(self, 3, lbl_80796E70, lbl_80796E74) == 1) {
                if ((system_w.field_0x0c & 3) == 0) {
                    setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                    fn_80304508(self, 52, 26, &v, lbl_80796E20);
                }
            }
            if (em_frame_check(self, 0, lbl_80796E78, lbl_80796E1C) == 1 ||
                em_frame_check(self, 0, lbl_80796E7C, lbl_80796E1C) == 1 ||
                em_frame_check(self, 0, lbl_80796E80, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                fn_80304508(self, 51, 26, &v, lbl_80796E84);
            }
            if (em_frame_check(self, 0, lbl_80796E50, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E1C, lbl_80796E60);
                fn_801049D0(self, 26, 47, 0, &v, lbl_80796E64);
            }
            if (em_frame_check(self, 3, lbl_80796E88, lbl_80796E8C) == 1 ||
                em_frame_check(self, 3, lbl_80796E90, lbl_80796E94) == 1) {
                if ((system_w.field_0x0c & 3) == 0) {
                    setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                    fn_80304508(self, 52, 26, &v, lbl_80796E98);
                }
            }
            if (em_frame_check(self, 0, lbl_80796E9C, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                fn_80304508(self, 51, 26, &v, lbl_80796E2C);
            }
        }
        if (--self->timer_0x020 <= 0) {
            if ((arg & 0xFF) == 1) {
                fn_80128A14(self, 1, 8);
            } else {
                fn_80128A14(self, 1, 5);
            }
        }
        break;
    }
}

/* The second action's long body: `fn_8012F5B8(self, 20, 6, 0)` on the opening step and six
 * `em_frame_check` windows whose hits spawn the effect - the 12/16 and 50/66 and 72/80 pairs gated by
 * the low two bits of `system_w`'s +0x0C word, the odd ones ungated. */
extern "C" void fn_80148BD0(_ENEMY_WORK* self, u32 arg) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 20, 6, 0);
        break;
      case 1:
        if ((arg & 0xFF) == 1) {
            if (em_frame_check(self, 0, lbl_80796EA0, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                fn_80304508(self, 51, 26, &v, lbl_80796E98);
            }
            if (em_frame_check(self, 3, lbl_80796EA4, lbl_80796EA8) == 1 &&
                (system_w.field_0x0c & 3) == 0) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                fn_80304508(self, 52, 26, &v, lbl_80796E20);
            }
            if (em_frame_check(self, 0, lbl_80796EAC, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                fn_80304508(self, 51, 26, &v, lbl_80796EB0);
            }
            if (em_frame_check(self, 3, lbl_80796E60, lbl_80796EB4) == 1 &&
                (system_w.field_0x0c & 3) == 0) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                fn_80304508(self, 52, 26, &v, lbl_80796E20);
            }
            if (em_frame_check(self, 0, lbl_80796EB8, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                fn_80304508(self, 51, 26, &v, lbl_80796E20);
            }
            if (em_frame_check(self, 3, lbl_80796EBC, lbl_80796E40) == 1 &&
                (system_w.field_0x0c & 3) == 0) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                fn_80304508(self, 52, 26, &v, lbl_80796E20);
            }
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The four-step body of the second action: motion 23, then 24 with a 1800-frame timer
 * (`fn_80132224`), then 25 (`fn_80132264`) once the timer runs out, then the action-end test. */
extern "C" void fn_80148E6C(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 23, 6, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 24, 0, 0);
            self->timer_0x020 = 1800;
            fn_80132224(self);
        }
        break;
      case 2:
        fn_8013221C(self, lbl_80796E84, 1, 10);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            fn_8012F5B8(self, 25, 4, 0);
            fn_80132264(self);
        }
        break;
      case 3:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The same two-step shape with `fn_8012F5B8(self, 17, 6, 0)`. */
extern "C" void fn_80148F88(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 17, 6, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The second action's sub-state dispatcher (sub-states 0..10; 4/7 share the long body with 0/1 and
 * 5/8 the second one). */
extern "C" void fn_80149004(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_80148528(self);
        break;
      case 1:
        fn_801486A4(self);
        break;
      case 2:
        fn_80148720(self);
        break;
      case 3:
        fn_8014879C(self);
        break;
      case 4:
        fn_80148828(self, 0);
        break;
      case 5:
        fn_80148BD0(self, 0);
        break;
      case 6:
        fn_80148E6C(self);
        break;
      case 7:
        fn_80148828(self, 1);
        break;
      case 8:
        fn_80148BD0(self, 1);
        break;
      case 9:
        fn_80148528(self);
        break;
      case 10:
        fn_80148F88(self);
        break;
    }
}

/* The third action's body: `fn_8012CF20` on the opening call, `fn_80134004`'s rotation seed picked by
 * the argument (the `-500` default, `0` for 1, and `0` clamped to 700 for 2), the motion
 * `fn_8012F5B8(self, 6, 6, 0)`, then the 64-frame release test. */
extern "C" void fn_80149068(_ENEMY_WORK* self, s32 arg1, u32 arg2) {
    if ((arg2 & 0xFF) == 1) {
        fn_8012CF20(self);
    }
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        switch (arg1 & 0xFF) {
          default:
            fn_80134004(self, 0, lbl_80796EC0);
            break;
          case 1:
            fn_80134004(self, 0, lbl_80796E1C);
            break;
          case 2:
            fn_80134004(self, 0, lbl_80796E1C);
            if (self->value_0x378 > lbl_80796EC4) {
                self->value_0x378 = lbl_80796EC4;
            }
            break;
        }
        fn_8012F5B8(self, 6, 6, 0);
        break;
      case 1:
        if (fn_80134114(self, 0, 64) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The fourth action's body: the joint-table pair `lbl_8056F8E0` (`fn_80134964` to arm, `fn_80134B0C`
 * to test). */
extern "C" void fn_80149170(_ENEMY_WORK* self, u32 arg) {
    if ((arg & 0xFF) == 1) {
        fn_8012CF20(self);
    }
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_80134964(self, lbl_8056F8E0, 0, 0, 0);
        break;
      case 1:
        if (fn_80134B0C(self, lbl_8056F8E0) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The same shape with the second joint-table pair `lbl_8056F920` (armed with the 1 that picks its
 * second entry). */
extern "C" void fn_80149214(_ENEMY_WORK* self, u32 arg) {
    if ((arg & 0xFF) == 1) {
        fn_8012CF20(self);
    }
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_80134964(self, lbl_8056F920, 0, 1, 0);
        break;
      case 1:
        if (fn_80134B0C(self, lbl_8056F920) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The spawn record's position reset: zero the VEC3 at +0x04 and hand the record back. */
extern "C" EmSpawnRec* fn_80149788(EmSpawnRec* rec) {
    VEC3_ctor(&rec->pos);
    return rec;
}

/* The spawn helper the record is built for: fill it and hand it to `fn_801493A8`. */
extern "C" void fn_801497BC(_ENEMY_WORK* self, u32 arg, u32 flag) {
    if ((flag & 0xFF) != 0) {
        fn_8012CF20(self);
    }
    fn_801493A8(self, (u8)arg, 0, 0);
}

/* The third action's second half: the same `fn_80134004` seed pick as `fn_80149068` with the motion
 * `fn_8012F5B8(self, 7, 10, 0)` and the same 64-frame release test. */
extern "C" void fn_801492B8(_ENEMY_WORK* self, s32 arg) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        switch (arg & 0xFF) {
          default:
            fn_80134004(self, 0, lbl_80796EC0);
            break;
          case 1:
            fn_80134004(self, 0, lbl_80796E1C);
            break;
          case 2:
            fn_80134004(self, 0, lbl_80796E1C);
            if (self->value_0x378 > lbl_80796EC4) {
                self->value_0x378 = lbl_80796EC4;
            }
            break;
        }
        fn_8012F5B8(self, 7, 10, 0);
        break;
      case 1:
        if (fn_80134114(self, 0, 64) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The fourth action's four-step body: motion 41, then 67 with `fn_801353E4`'s joint setup, then the
 * `lbl_805A1DA0` scale through `fn_80135644`/`fn_80135584`, then motion 26 and `fn_80133C50`'s
 * 0x100-keyed release. */
extern "C" void fn_801498C8(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 41, 6, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130248(self);
            fn_801305C4(self);
            fn_8012F5B8(self, 67, 0, 0);
            fn_801353E4(self);
        }
        break;
      case 2:
        self->field_0x314 = fn_80135644(self, lbl_805A1DA0);
        fn_80130248(self);
        fn_80135584(self, &self->field_0x1BC);
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 26, 6, 0);
        }
        break;
      case 3:
        fn_80133C50(self, 256);
        if (fn_8012F93C(self) == 1) {
            fn_80128A70(self, 3, 2);
        }
        break;
    }
}

/* The same shape as `fn_801498C8` without its fourth step: the release tail is `fn_80127FE4`. */
extern "C" void fn_80149A08(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 41, 6, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130248(self);
            fn_801305C4(self);
            fn_8012F5B8(self, 67, 0, 0);
            fn_801353E4(self);
        }
        break;
      case 2:
        self->field_0x314 = fn_80135644(self, lbl_805A1DA0);
        fn_80130248(self);
        fn_80135584(self, &self->field_0x1BC);
        if (fn_8012F93C(self) == 1) {
            fn_80127FE4(self);
        }
        break;
    }
}

/* The fifth action's empty step (retail compiles it to a bare `blr`). */
extern "C" void fn_80149C54(_ENEMY_WORK* self) {
}

/* The sixth action's sub-state dispatcher (sub-states 0..13; 4/5/6 all spawn through `fn_801497BC`
 * with a different flag pair, and 7/9/11/8/10/12/13 share the other handlers' argument picks). */
extern "C" void fn_80149814(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_80149068(self, 0, 0);
        break;
      case 1:
        fn_80149170(self, 0);
        break;
      case 2:
        fn_80149214(self, 0);
        break;
      case 3:
        fn_801492B8(self, 0);
        break;
      case 4:
        fn_801497BC(self, 0, 0);
        break;
      case 5:
        fn_801497BC(self, 1, 0);
        break;
      case 6:
        fn_801497BC(self, 0, 1);
        break;
      case 7:
        fn_80149068(self, 1, 0);
        break;
      case 8:
        fn_801492B8(self, 1);
        break;
      case 9:
        fn_80149068(self, 2, 0);
        break;
      case 10:
        fn_801492B8(self, 2);
        break;
      case 11:
        fn_80149068(self, 0, 1);
        break;
      case 12:
        fn_80149170(self, 1);
        break;
      case 13:
        fn_80149214(self, 1);
        break;
    }
}

/* The sixth action's five-step body: motions 26 (`fn_80134E8C`), 29, 46 and 65, the `fn_80130008`
 * gate on the second step and the `fn_80134F18` joint refresh on it. */
extern "C" void fn_80149AFC(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 26, 6, 0);
        fn_80134E8C(self);
        break;
      case 1:
        fn_80134F18(self);
        fn_80130248(self);
        if (fn_80130008(self) == 1) {
            self->state++;
            fn_80130478(self, 3);
            fn_8012F5B8(self, 29, 6, 0);
        }
        break;
      case 2:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 46, 6, 0);
        }
        break;
      case 3:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 0);
            fn_8012F5B8(self, 65, 0, 0);
        }
        break;
      case 4:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The seventh action's body: motion 31, then `fn_80128BF8`'s release with the target-angle rate
 * (`calcVecAng2` against `vec_0x36C` minus the object's own angle, normalised to turns per frame at
 * 60 fps) fed to `fn_8012F860`, and the frame count from `fn_8012F8EC` handed to `fn_8012F7D4`
 * with the motion parameter at +0x464. */
extern "C" void fn_80149C58(_ENEMY_WORK* self) {
    u16 angle;
    u16 diff;
    f32 rate;

    switch (self->state) {
      case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F810(self);
        fn_8012F5B8(self, 31, 0, 0);
        break;
      case 1:
        fn_80128BF8(self, 0);
        angle = (u16)calcVecAng2(&self->pos, &self->vec_0x36C);
        diff = (u16)(angle - self->field_0x1C0);
        if (diff != 0) {
            rate = (f32)(s16)diff * lbl_80796ED0 / lbl_80796ED4 / lbl_80796ED8;
        } else {
            rate = lbl_80796E1C;
        }
        fn_8012F860(self, rate, lbl_80796EDC);
        fn_8012F7D4(self, 35, 36, (s32)fn_8012F8EC(self), self->field_0x464);
        break;
    }
}

/* The sixth action's six-state spawn body: motion 214, the `fn_80133C50` gate on arguments 2/4, then
 * motion 215 with the `lbl_805A1F70` scale, `fn_801303EC`'s angle and the `fn_803B9BA0` shell call,
 * then the two `fn_801545B8`/shell-callback spawn paths keyed on the argument pair, and finally
 * motion 27/216/217 with `fn_801280AC`'s release. */
extern "C" void fn_801493A8(_ENEMY_WORK* self, u32 arg1, s32 arg2, u32 arg3) {
    EmSpawnRec rec;
    u32 v;

    fn_80149788(&rec);
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 214, 4, 0);
        break;
      case 1:
        if ((arg2 & 0xFF) == 2 || (arg2 & 0xFF) == 4) {
            fn_80133C50(self, 1024);
        }
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 3);
            fn_801303EC(self, lbl_80796E60);
            fn_8012F5B8(self, 215, 0, 0);
            fn_801353F8(self);
            self->field_0x318 = fn_80135644(self, lbl_805A1F70);
            fn_801354F4(self, &self->field_0x1BC);
            if ((arg3 & 0xFF) == 1) {
                fn_803B9BA0(self, &self->pos, 100);
                fn_802B9574(0);
            }
        }
        break;
      case 2:
        v = (u8)arg2 - 1;
        if (v <= 3 && em_frame_check(self, 0, lbl_80796EC8, lbl_80796E1C) == 1) {
            fn_801545B8(&rec, 1, 7646, 0);
            if (v <= 1) {
                rec.field_0x10 |= 64;
                if ((arg3 & 0xFF) == 1) {
                    rec.field_0x10 |= 128;
                }
                shell_set_func_ptr->field_0x3c(self, &rec, 0, shell_set_func_ptr);
                fn_801039B0(self, 23, rec.field_0x12, rec.field_0x14);
                fn_801039B0(self, 2, rec.field_0x12, rec.field_0x14);
            } else if ((u8)arg2 - 3 <= 1) {
                shell_set_func_ptr->field_0x3c(self, &rec, 1, shell_set_func_ptr);
                fn_801039B0(self, 4, rec.field_0x12, rec.field_0x14);
            }
        }
        if (fn_8012F93C(self) == 1) {
            self->state++;
            self->state_0x006 = 0;
            self->field_0x324 = lbl_80796ECC;
            fn_801355C8(self, &self->field_0x1BC);
            fn_8012F5B8(self, 27, 6, 0);
        } else {
            self->field_0x318 = fn_80135644(self, lbl_805A1F70);
            fn_801354F4(self, &self->field_0x1BC);
        }
        break;
      case 3:
        fn_801355C8(self, &self->field_0x1BC);
        if (self->field_0x318 > lbl_80796E1C) {
            self->field_0x318 = lbl_80796E1C;
        }
        if (fn_8012F93C(self) == 1) {
            switch ((u8)v) {
              case 0:
                switch (self->state_0x006) {
                  case 0:
                    self->state_0x006++;
                    fn_8012F5B8(self, 27, 0, 0);
                    break;
                  case 1:
                    self->state++;
                    fn_8012F5B8(self, 216, 6, 0);
                    break;
                }
                break;
              case 1:
                fn_801280AC(self);
                break;
            }
        }
        break;
      case 4:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 0);
            fn_8012F5B8(self, 217, 0, 0);
        }
        break;
      case 5:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}
