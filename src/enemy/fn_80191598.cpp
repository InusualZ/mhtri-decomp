/* enemy/fn_80191598.cpp - the enemy aim/action group of the `ResUserDataAc` class set.
 *
 * `.text` 0x80191598..0x801926EC (26 functions, 4436 B), extab 0x8000ED64..0x8000EDF4,
 * extabindex 0x8002A300..0x8002A3D8.  Registered from `proposal/80191598_fn_80191598.cpp`.
 *
 * Module `enemy`.  The range's callees are the enemy work API
 * (`em_act_ck__FP11_ENEMY_WORKUcUc`, `em_parts_damage_level_get__FP11_ENEMY_WORKUc`,
 * `get_em_scale__FP11_ENEMY_WORK`, `get_joint_wmat_em__FP11_ENEMY_WORKUlPQ34nw4r4math5MTX34`,
 * `get_move_work_adrs__FUc`), both bracketing registered units are `enemy/*`, and the three `.data`
 * class tables that hold this range's entry points (0x805AA960, 0x805AA9CC, 0x805AAA38, slots 4..13)
 * sit beside the `em017_prog_tbl`/`em021_prog_tbl` labels, i.e. the range is part of the per-enemy
 * class definition block.  Language C++: every call out of the range is a mangled symbol.
 *
 * Seam.  The edges are the proposal's own: below is `proposal/8018B3B8` (0x8018B3B8..0x80191598,
 * unclaimed) and above `proposal/801926EC`.  Neither is a byte-budget artefact - the extabindex run
 * this unit claims (0x8002A300..0x8002A3D8) holds exactly the 18 records of this range's functions,
 * with the neighbouring functions' records (`fn_801913FC` below, `fn_801926EC` above) on either side,
 * and the class tables straddle the seam (`fn_80191038`/`fn_801913FC` are their first two slots).
 *
 * Name.  The map has only `fn_XXXXXXXX` for this range and the runtime dump answers only `zz_`
 * placeholders (`dumpmap.py lookup 0x80191598` -> `zz_0191598_`), so the file keeps the map's stem.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py
 * lookup on the range's inventory: all 26 names are bare `.text` entries in
 * config/RMHE08/symbols.txt and the runtime dump has only `zz_XXXXXXXX_` placeholders for them)
 *
 * Types.  This range reads `_ENEMY_WORK` bytes that `include/enemy.h` views differently: it steps
 * +0x328/+0x32C/+0x330/+0x338/+0x33C as 4-byte fixed-point angle words where that header has a
 * `VEC3 v_0x320` and 16-bit timers, and it reads +0x344/+0x348/+0x34C as floats where the header has
 * padding.  The unit therefore carries its own view (`EmActWork`), exactly as `enemy/fn_8013ACC4.cpp`
 * (`EmWork`), `enemy/fn_80138074.c` (`_ENEMY_WORK`) and `enemy/fn_8013F764.cpp` (`EmcWork`) do.  The
 * view's size (0xB18) is measured from this range itself: `fn_80192448` walks the
 * `get_move_work_adrs(3)` records with `addi r29,r29,0xB18`.  The outbox asks for these fields to be
 * folded into the shared header.
 *
 * The record at +0x328 (0x3C bytes) is the work's aim/animation target: a fixed-point angle word,
 * two 3-word angle groups (`fn_80191EF8` steps them, `fn_80191CE4` copies them into the `_CP_VECTOR`
 * `cpSetRotMatrix` takes), a float vector at +0x1C, two floats and a byte of bits (0x01 `fn_80192618`
 * arms, 0x02 `fn_801923E0` and 0x08 `fn_80192410` read).  `fn_80192630` initialises it; the flag
 * accessors `fn_80192348`/`58`/`70`/`CC` use the work's own displacement (0x35D/0x35B), which is what
 * places the record at the work base.
 *
 * Measured result.  All 26 bodies are written - nothing is stubbed - and 22 of them are byte-identical
 * (official `report generate` fuzzy_match_percent, this worktree, against the split target object):
 *   fn_80191990 100.00, fn_80191A6C 100.00, fn_80191AD8 100.00, fn_80191AE8 100.00,
 *   fn_80191CDC 100.00, fn_80191CE4 100.00, fn_80191EF8 100.00, fn_80192080 100.00,
 *   fn_80192108 100.00, fn_801921A8 100.00, fn_80192204 100.00, fn_80192348 100.00,
 *   fn_80192358 100.00, fn_80192370 100.00, fn_8019238C 100.00, fn_801923CC 100.00,
 *   fn_801923E0 100.00, fn_80192410 100.00, fn_80192440 100.00, fn_8019255C 100.00,
 *   fn_80192618 100.00, fn_80192630 100.00;
 *   fn_80191B4C 99.80 (400 B), fn_80192448 99.13 (276 B), fn_80191598 98.82 (1016 B),
 *   fn_80191E30 93.10 (200 B).  The unit's sections match the target's exactly: `.text` 0x1144,
 *   extab 0x90, extabindex 0xD8 (18 records, one per function with a frame).
 *
 * Source shapes worth keeping (each measured, each worth its score):
 *   * `#pragma peephole off` (one scoped pragma, docs/plan.md 8.2): the build's -O3 peephole fuses
 *     the `extsh` + `cmpwi` pair a 16-bit value test needs into the record form `extsh.`.  With the
 *     peephole on: `fn_80191EF8` 94.64, `fn_80192108` 97.38, `fn_801921A8` 95.43; with it off all
 *     three are 100.00 and every already-identical function is unchanged.
 *   * A one-case `switch` is the idiom several of these guards are written in: `switch (x) { case 3: }`
 *     makes MWCC emit the *signed* compare (`cmpwi r0,3`) where `if (x == 3)` emits `cmplwi`
 *     (`fn_80191990`'s map-key gate and state byte, `fn_80191AE8`'s kind gate, `fn_80191B4C`'s
 *     kind gate, `fn_80191CE4`/`fn_80191E30`'s item-type gate - the last two were 97/92 % as `if`s).
 *   * `u8 >= 2` (`fn_8019238C`) is not a compare at all: MWCC emits the borrow trick
 *     `((v | ~2) - ((v - 2) >> 1)) >> 31` (`li r3,2; orc; addi; srwi; subf; srwi 31`).  The source is
 *     the comparison, not the arithmetic - writing the arithmetic by hand gives the same bytes but
 *     the *idiom* is what a later reader needs.
 *   * `fn_80191CE4`'s frame is sized for a 0x24-byte 3x3 matrix (`EmMtx33`) at +0x14, not for a
 *     second 0x30-byte matrix: with `MTX34` there the frame is 0x90 where retail keeps 0x80, and the
 *     score drops from 100.00 to 98.96.
 *   * Local *declaration order* drives MWCC's callee-saved assignment (`fn_80192448`: `max`, `i`,
 *     `work` gives retail's r31/r30/r29; `max`, `work`, `i` gives r31/r29/r30 and 98.04 vs 99.13).
 *
 * Residuals (every remaining objdiff row of the four functions that are not byte-identical):
 *   * `fn_80191E30` 93.10 - the whole body matches except (a) the two callee-saved registers of the
 *     item argument and the work pointer are swapped (retail: item in r30, work in r31; this build:
 *     the reverse, i.e. MWCC ordered by first use here) and (b) two extra `b` instructions (12 B):
 *     retail's three inner cases each branch to the shared continuation at +0x70, ours fall into the
 *     next case's `li`.  Tried: `if (item->type_0x04 == 0xFF)` instead of the one-case `switch` (drops
 *     to 91.9), the `work` pointer declared before/after the other locals, and `default: return;` vs a
 *     guarded `if` (the latter grows the function further).
 *   * `fn_80191598` 98.82 - one instruction (4 B) long.  `fn_80191B4C` 99.80 and `fn_80192448` 99.13 -
 *     a single row each; both are the residual register colouring of a value MWCC keeps in a different
 *     callee-saved register than retail (the `hit`/`probe` and `work`/`i` pairs).
 *
 * Type and declaration follow-ups (the outbox carries each as a `shared-file`/`config_requests` entry):
 *   * `_ENEMY_WORK`'s +0x328..+0x3A4 aim record, the +0x590 cluster array's live byte and position,
 *     +0x9F6 and +0x1E0/0x1E1/0x1E2 belong in `include/enemy.h` (this range's view of 0x328..0x33E
 *     contradicts that header's `VEC3 v_0x320`/16-bit timers).
 *   * `ResUserDataAc` (this range's `EmUserData` view) belongs in `include/enemy/fn_80138074.h`, its
 *     owner's header - `include/enemy/fn_80138074.h` declares `fn_8013A654` with `_ENEMY_WORK*` where
 *     the callee's own body reads +0x04/+0x08 (settled from the callee, rule 6 of the playbook).
 *   * the plain prototypes at the top of this file (`fn_80130478`, `fn_80126324`, `fn_80129xxx`,
 *     `fn_8013918C`, `fn_80139A64`, `fn_80139A7C`, `fn_8013A654`, `fn_8008E8D0`, `fn_8008EE68`,
 *     `fn_800FBB90`, `fn_800FC0D4`, `fn_805012E8`, `fn_80101428`, `fn_8010140C`, `MTX34_ctor`,
 *     `fn_800516F0`, `setVec3`, `copyVec3`, `VEC3_ctor`, `fn_80051490`, `fn_802B0668`,
 *     `fn_80182D5C`) belong in their owners' headers / `include/unsplit/enemy.h`; `setVec3`'s
 *     `void` return in `include/mh3_pad.h` is wrong for this range's call sites, which read its r3.
 *   * the unit registers a `.ctors` word (0x8056F33C..0x8056F340, added by the split itself): the
 *     original translation unit has a static constructor, most plausibly the one that fills the five
 *     static vectors `fn_80192204` builds (`lbl_806A7A00`..`lbl_806A7A60`).
 */
#include "types.h"

#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* Retail keeps the `extsh` + `cmpwi` pair a value test needs: this build's -O3 peephole fuses
 * them into the record form `extsh.`.  Measured on this unit: with the peephole on `fn_80191EF8`
 * 94.64 / `fn_80192108` 97.38 / `fn_801921A8` 95.43, with it off 100.00 / 100.00 / 100.00 and every
 * already-identical function unchanged (docs/plan.md 8.2: a per-unit deviation, numbers in the
 * outbox's `flags_probed`). */
#pragma peephole off

/* The map's mangled callees take a `_ENEMY_WORK*`/`_CP_VECTOR*`; the front-end has to spell those
 * names to reproduce their symbols (rule 9), so both tags are forward-declared and the calls below
 * cast this range's view onto them (a pointer cast, never arithmetic). */
struct _ENEMY_WORK;

/* `_CP_VECTOR` is `include/ef.h`'s type (three words).  That header is not includable here: its
 * `setVec3` declaration returns void, where this range's call sites read the callee's r3 (see
 * `fn_80192108`/`fn_80192204`), so the outbox carries the header fix instead. */
struct _CP_VECTOR;

/* ------------------------------------------------------------------------------------------------ */
/* this range's view of the records it reads                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* Three 32-bit fixed-point angles (`_CP_VECTOR`'s layout, under this unit's own name).
 * size: 0x0C */
struct EmRotVec {
    /* +0x00 */ u32 x;
    /* +0x04 */ u32 y;
    /* +0x08 */ u32 z;
};

/* The 3x3 matrix `fn_805012E8` fills (the SDK's `Mtx33` layout - nine floats; the callee's own body
 * stores only at +0x20, the 3x3's last element, and the frame this unit's `fn_80191CE4` measures is
 * sized for this object, not for a second 0x30-byte matrix).
 * size: 0x24 */
struct EmMtx33 {
    /* +0x00 */ f32 m[3][3];
};

/* The aim/animation target record at `EmActWork::aim_0x328`.
 * size: 0x3C */
struct EmAimRec {
    /* +0x00 */ u32 angle_0x00;    /* the lead angle: +-0x222 per frame */
    /* +0x04 */ EmRotVec rot_0x04; /* step +-0x1C7; z is armed to -1 by the initialiser */
    /* +0x10 */ EmRotVec rot_0x10; /* step +-0x1C7 */
    /* +0x1C */ VEC3 vec_0x1C;     /* the vector `fn_8008E8D0` is handed */
    /* +0x28 */ f32 value_0x28;    /* armed with `value_0x2C` before bit 0 of `flags_0x35` */
    /* +0x2C */ f32 value_0x2C;
    /* +0x30 */ u8 flag_0x30;
    /* +0x31 */ u8 flag_0x31;
    /* +0x32 */ u8 flag_0x32;
    /* +0x33 */ u8 flag_0x33;      /* nonzero: `fn_801923CC` reports */
    /* +0x34 */ u8 flag_0x34;
    /* +0x35 */ u8 flags_0x35;     /* bit 0x01 `fn_80192618`, 0x02 `fn_801923E0`, 0x08 `fn_80192410` */
    /* +0x36 */ s16 value_0x36;
    /* +0x38 */ s16 value_0x38;
    /* +0x3A */ s16 value_0x3A;
};

/* One 0x90-byte per-area cluster at `EmActWork::clusters_0x590` (three of them, ending at +0x740;
 * `enemy/fn_80138074.c` carries the same array as an unnamed byte blob, and `fn_80139954` reaches it
 * with the same `i * 0x90 + 0x590` addressing).  The two fields this range reads are the liveness
 * byte and the position `fn_800FBB90` copies.
 * size: 0x90 */
struct EmCluster {
    /* +0x00 */ u8 unused_0x00[0x03];
    /* +0x03 */ u8 live_0x03;     /* `fn_80191E30` needs >= 1 */
    /* +0x04 */ u8 unused_0x04[0x20];
    /* +0x24 */ VEC3 pos_0x24;    /* the work's +0x5B4 */
    /* +0x30 */ u8 unused_0x30[0x60];
};

/* The enemy work record, as this range's accesses measure it.
 * size: 0xB18 */
struct EmActWork {
    /* +0x000 */ u8 active;       /* `fn_80192448` skips an empty record */
    /* +0x001 */ u8 unused_0x001[0x003 - 0x001];
    /* +0x003 */ u8 team;         /* the dispatch key of `fn_80191598` (16/17/21) and `fn_80192448`
                                   * (0x12) */
    /* +0x004 */ u8 unused_0x004[0x00A - 0x004];
    /* +0x00A */ u8 field_0x00A;  /* 1 selects the second entry block of the dispatch */
    /* +0x00B */ u8 unused_0x00B[0x188 - 0x00B];
    /* +0x188 */ VEC3 pos_0x188;
    /* +0x194 */ u8 unused_0x194[0x1E0 - 0x194];
    /* +0x1E0 */ u8 field_0x1E0;  /* the key `fn_802B0668` (map lookup) is called with */
    /* +0x1E1 */ u8 act_id;       /* the action id every dispatch in this range switches on */
    /* +0x1E2 */ u8 state_0x1E2;
    /* +0x1E3 */ u8 unused_0x1E3[0x328 - 0x1E3];
    /* +0x328 */ EmAimRec aim_0x328;
    /* +0x364 */ u8 unused_0x364[0x590 - 0x364];
    /* +0x590 */ EmCluster clusters_0x590[3];
    /* +0x740 */ u8 unused_0x740[0x9F6 - 0x740];
    /* +0x9F6 */ u8 state_0x9F6;
    /* +0x9F7 */ u8 unused_0x9F7[0xB18 - 0x9F7];
};

/* The `ResUserDataAc` object this range's virtuals run on: `src/enemy/fn_80138074.c` owns the
 * canonical definition (vtable at +0, work at +4, a flag word at +8) and this is its view of the two
 * fields the range reads.
 * size: 0x0C */
struct EmUserData {
    /* +0x00 */ void* vtable_0x00;
    /* +0x04 */ EmActWork* work_0x04;
    /* +0x08 */ u32 flags_0x08;
};

/* The user-data item the six-argument class methods take (`src/enemy/fn_80138074.c` owns the full
 * 0x18-byte record `UserDataItem`; this range reads the +0x04 type byte).
 * size: 0x18 */
struct EmUserItem {
    /* +0x00 */ u32 key_0x00;
    /* +0x04 */ u8 type_0x04;   /* 0xFF admits the call */
    /* +0x05 */ u8 unused_0x05[0x13];
};

/* A holder of one matrix pointer (`fn_80139A64`/`fn_80139A7C`'s argument; the owner spells it
 * `MtxHolder`).
 * size: 0x04 */
struct EmMtxHolder {
    /* +0x00 */ MTX34* mtx_0x00;
};

/* The static vector pair `fn_80192204` builds (`VEC3[2]` per 0x18-byte object).
 * size: 0x18 */
struct EmVecPair {
    /* +0x00 */ VEC3 vec_0x00;
    /* +0x0C */ VEC3 vec_0x0C;
};

/* The request record `fn_80192108` fills in.
 * size: 0x18 */
struct EmEffRequest {
    /* +0x00 */ u32 id_0x00;      /* always 0x17 */
    /* +0x04 */ VEC3 pos_0x04;
    /* +0x10 */ u8 field_0x10;
    /* +0x11 */ u8 unused_0x11;
    /* +0x12 */ s16 field_0x12;
    /* +0x14 */ s16 field_0x14;
    /* +0x16 */ u8 unused_0x16[0x02];
};

/* ------------------------------------------------------------------------------------------------ */
/* the shared pool this range reads                                                                  */
/* ------------------------------------------------------------------------------------------------ */

extern const f32 lbl_80797E88;
extern const f32 lbl_80797E9C;
extern const f32 lbl_80797EB4;
extern const f32 lbl_80797F24;
extern const f32 lbl_80797F80;
extern const f32 lbl_8079800C;
extern const f32 lbl_807981C0;
extern const f32 lbl_8079822C;
extern const f32 lbl_80798230;
extern const f32 lbl_80798234;
extern const f32 lbl_80798238;
extern const f32 lbl_8079823C;
extern const f32 lbl_80798240;
extern const f32 lbl_80798244;
extern const f32 lbl_80798248;
extern const f32 lbl_8079824C;
extern const f32 lbl_80798250;
extern const f32 lbl_80798254;

/* The five static vectors `fn_80192204` builds (the last one is the default `fn_80192108` copies
 * from). */
extern EmVecPair lbl_806A7A00;
extern EmVecPair lbl_806A7A18;
extern EmVecPair lbl_806A7A30;
extern EmVecPair lbl_806A7A48;
extern VEC3 lbl_806A7A60;

/* The one-shot latch `fn_80192108` sets. */
extern s8 lbl_80794AA0;

/* The `.data` word `fn_80191B4C` hands to `fn_8012A014` (the map's own label; it sits in the class
 * table block this range's entry points live in). */
extern u8 lbl_805AAA74[];

/* ------------------------------------------------------------------------------------------------ */
/* callees                                                                                           */
/* ------------------------------------------------------------------------------------------------ */

/* The mangled callees, outside `extern "C"` so the front-end mangles them the way the map spells them
 * (rule 9).  Their parameter widths are the ones the map's manglings encode. */
u32 em_act_ck(struct _ENEMY_WORK* work, u8 a, u8 b);
u8 em_parts_damage_level_get(struct _ENEMY_WORK* work, u8 part);
u16 get_move_work_max(u8 kind);
void* get_move_work_adrs(u8 kind);
f32 get_em_scale(struct _ENEMY_WORK* work);
void get_joint_wmat_em(struct _ENEMY_WORK* work, u32 joint, MTX34* mtx);
void senko_set(VEC3* pos, f32 value, u8 a, s16 b);
void cpSetRotMatrix(struct _CP_VECTOR* rot, MTX34* mtx);
void operator delete(void* p) throw ();

#ifdef __cplusplus
extern "C" {
#endif

/* The plain callees.  Their signatures are this range's own call sites (the arity and the return
 * width the target's registers show); they belong in their owner's header or in
 * `include/unsplit/enemy.h`, and the outbox carries that list - the same interim spelling
 * `enemy/fn_8013ACC4.cpp` uses for its neighbours. */
void fn_80130478(EmActWork* self, u32 mode);
void fn_80126324(EmActWork* self, u8 a, u8 b, f32 value);
void fn_80182D5C(void);
u8 fn_802B0668(u8 key);
u32 fn_8012EC60(EmActWork* self);
u32 fn_8012EC3C(EmActWork* self);
u32 fn_801321DC(EmActWork* self);
u32 fn_80129D3C(EmActWork* self);
u8 fn_80129DB8(EmActWork* self);
u32 fn_80129A70(EmActWork* self, u16 value);
u32 fn_8012A014(EmActWork* self, u32 a, u32 b, u16 c, u32 d, u8* table);
s32 fn_8012A204(EmActWork* self);
void fn_8012F5B8(EmActWork* self, s32 a, s32 b, s32 c);
void fn_803B9BA0(EmActWork* self, VEC3* pos, s32 value);
void fn_8013918C(void* p, s16 flag);
void fn_8012933C(EmActWork* self, u8 a, u32 b, u32 c);
void fn_80139A64(EmMtxHolder* holder, void* src);
void fn_80139A7C(EmMtxHolder* holder, void* mtx);
void fn_8013A654(EmUserData* self, u32 flags);
void fn_8008E8D0(void* holder, void* vec);
void fn_8008EE68(void* holder, void* mtx);
void fn_800FBB90(MTX34* mtx, VEC3* vec);
void fn_800FC0D4(void* dst, void* src);
void fn_805012E8(EmMtx33* dst, const MTX34* src);
void fn_80101428(MTX34* mtx, VEC3* vec);
void fn_8010140C(MTX34* mtx, VEC3* out);
void fn_800516F0(void* mtx);
void fn_80051490(void* dst, const void* src);

/* This range's own entry points (rule 2: declared where they are defined, i.e. here). */
void fn_80191598(EmActWork* self, u8* out_class, u8* out_state);
void fn_80191990(EmActWork* self);
u32 fn_80191A6C(EmActWork* self);
u32 fn_80191AD8(EmActWork* self);
void fn_80191AE8(EmActWork* self, u32 kind);
s32 fn_80191B4C(EmActWork* self, u32 arg);
void fn_80191CDC(EmUserData* self);
void fn_80191CE4(EmUserData* self, EmMtxHolder* holder, u32 a2, u32 a3, u32 kind, EmUserItem* item);
void fn_80191E30(EmUserData* self, EmMtxHolder* holder, u32 a2, u32 a3, u32 kind, EmUserItem* item);
void fn_80191EF8(EmActWork* self);
void fn_80192080(EmActWork* self);
void fn_80192108(EmEffRequest* out, u8 a, s16 b, s16 c);
void* fn_801921A8(void* p, u32 flag);
void fn_80192204(void);
void fn_80192348(EmActWork* self, u8 mask);
void fn_80192358(EmActWork* self, u8 mask);
u32 fn_80192370(EmActWork* self, u32 mask);
u32 fn_8019238C(EmActWork* self);
u32 fn_801923CC(EmActWork* self);
u32 fn_801923E0(EmActWork* self);
u32 fn_80192410(EmActWork* self);
f32 fn_80192440(EmActWork* self);
s32 fn_80192448(EmActWork* self);
void fn_8019255C(EmActWork* self);
void fn_80192618(EmActWork* self);
void fn_80192630(EmActWork* self);

/* ------------------------------------------------------------------------------------------------ */
/* bodies, in address order                                                                          */
/* ------------------------------------------------------------------------------------------------ */

/* Picks the caller's class/state pair from the work's team, map key and action id. */
void fn_80191598(EmActWork* self, u8* out_class, u8* out_state) {
    switch (self->team) {
    case 16:
        fn_80130478(self, 0);
        *out_class = 12;
        *out_state = 2;
        switch (fn_802B0668(self->field_0x1E0)) {
        case 1:
            switch (self->act_id) {
            case 6:
                fn_80130478(self, 0);
                *out_class = 12;
                *out_state = 8;
                fn_80126324(self, 14, 15, lbl_80797E88);
                break;
            case 7:
                fn_80130478(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324(self, 13, 14, lbl_80797E88);
                break;
            }
            break;
        case 3:
            switch (self->act_id) {
            case 2:
                fn_80130478(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324(self, 6, 4, lbl_80797E88);
                break;
            case 3:
                fn_80130478(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324(self, 15, 16, lbl_80797E88);
                break;
            }
            break;
        case 9:
        case 11:
            if (self->act_id == 1) {
                fn_80130478(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324(self, 0, 1, lbl_80797E88);
            }
            break;
        }
        break;
    case 17:
        fn_80182D5C();
        switch (fn_802B0668(self->field_0x1E0)) {
        case 1:
            switch (self->act_id) {
            case 5:
            case 6:
            case 8:
                fn_80130478(self, 0);
                *out_class = 12;
                *out_state = 5;
                break;
            case 7:
            case 12:
                fn_80130478(self, 2);
                *out_class = 12;
                *out_state = 0;
                break;
            default:
                *out_class = 12;
                if (self->field_0x00A == 1) {
                    fn_80130478(self, 2);
                    *out_state = 3;
                } else {
                    fn_80130478(self, 0);
                    *out_state = 2;
                }
                break;
            }
            break;
        case 3:
            switch (self->act_id) {
            case 1:
            case 5:
                fn_80130478(self, 0);
                *out_class = 12;
                *out_state = 5;
                break;
            case 7:
                fn_80130478(self, 0);
                *out_class = 12;
                *out_state = 6;
                break;
            case 2:
            case 3:
            case 4:
            case 6:
            case 8:
                fn_80130478(self, 2);
                *out_class = 12;
                *out_state = 0;
                break;
            case 10:
                fn_80130478(self, 0);
                *out_class = 12;
                *out_state = 7;
                break;
            default:
                *out_class = 12;
                if (self->field_0x00A == 1) {
                    fn_80130478(self, 2);
                    *out_state = 3;
                } else {
                    fn_80130478(self, 0);
                    *out_state = 2;
                }
                break;
            }
            break;
        default:
            *out_class = 12;
            if (self->field_0x00A == 1) {
                fn_80130478(self, 2);
                *out_state = 3;
            } else {
                fn_80130478(self, 0);
                *out_state = 2;
            }
            break;
        }
        break;
    case 21:
        fn_80130478(self, 4);
        *out_class = 12;
        *out_state = 1;
        break;
    }
}

/* Re-arms the action the work's map key and action id select. */
void fn_80191990(EmActWork* self) {
    u32 state = 0;

    switch (fn_802B0668(self->field_0x1E0)) {
    case 3:
        switch (self->act_id) {
        case 1:
            state = 1;
            break;
        case 2:
            state = 2;
            break;
        case 3:
            switch (self->state_0x9F6) {
            case 2:
                state = 1;
                break;
            }
            break;
        }
        break;
    }
    if (state == 1) {
        fn_80130478(self, 0);
        fn_8012F5B8(self, 1, 0, 0);
    } else if (state == 2) {
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x28, 0, 0);
    }
}

/* Reports whether the work is in one of the two team-specific ready states. */
u32 fn_80191A6C(EmActWork* self) {
    if (self->team == 21) {
        if (self->state_0x1E2 == 4 && fn_801321DC(self) == 1) {
            return 1;
        }
    } else if (self->state_0x1E2 == 2 && fn_8012EC60(self) == 0) {
        return 1;
    }
    return 0;
}

/* Reports whether the work's state byte is clear. */
u32 fn_80191AD8(EmActWork* self) {
    return self->state_0x1E2 == 0;
}

/* Re-seats the work at its own position while the damage part admits the action. */
void fn_80191AE8(EmActWork* self, u32 kind) {
    switch ((u8)kind) {
    case 1:
        if (em_parts_damage_level_get((struct _ENEMY_WORK*)self, 1) == 2 && self->state_0x1E2 != 1) {
            fn_803B9BA0(self, &self->pos_0x188, 100);
        }
        break;
    }
}

/* Reports whether the work may run its current action: the team/state gate, the entry probe and the
 * two restart checks. */
s32 fn_80191B4C(EmActWork* self, u32 arg) {
    u8 kind = fn_802B0668(self->field_0x1E0);
    s32 hit;
    u32 probe;

    if (self->team != 16) {
        return 0;
    }
    switch (kind) {
    case 1:
    case 3:
        break;
    default:
        return 0;
    }
    if (fn_80129D3C(self) == 1) {
        return 1;
    }
    hit = 0;
    switch (kind) {
    case 1:
        probe = 8;
        break;
    case 3:
        probe = 6;
        break;
    default:
        probe = 0xFF;
        break;
    }
    if (probe != 0xFF) {
        switch (fn_80129DB8(self)) {
        case 1:
            hit = 1;
            break;
        case 2:
            return 1;
        }
    }
    if (hit == 0) {
        switch (kind) {
        case 1:
            probe = 12;
            break;
        case 3:
            probe = 4;
            break;
        default:
            probe = 0xFF;
            break;
        }
        if (fn_8012A014(self, 0, probe, (u16)arg, 0, lbl_805AAA74) == 1) {
            return 1;
        }
    }
    if (fn_80129A70(self, (u16)arg) == 1) {
        return 1;
    }
    return (fn_8012A204(self) - 1) == 0;
}

/* Re-enters the user-data accessor's third flag state. */
void fn_80191CDC(EmUserData* self) {
    fn_8013A654(self, 3);
}

/* Applies one user-data item's aim transform to the caller's matrix holder. */
void fn_80191CE4(EmUserData* self, EmMtxHolder* holder, u32 a2, u32 a3, u32 kind, EmUserItem* item) {
    MTX34 mtx;
    EmMtx33 dst;
    EmRotVec rot;
    EmActWork* work;

    work = self->work_0x04;
    MTX34_ctor(&mtx);
    fn_800516F0(&dst);
    switch (item->type_0x04) {
    case 0xFF:
        switch (kind) {
        case 16:
        case 18:
        case 20:
            fn_8008E8D0(holder, &work->aim_0x328.vec_0x1C);
            break;
        case 24:
            rot.x = work->aim_0x328.angle_0x00;
            rot.y = 0;
            rot.z = 0;
            cpSetRotMatrix((struct _CP_VECTOR*)&rot, &mtx);
            fn_805012E8(&dst, &mtx);
            fn_8008EE68(holder, &dst);
            break;
        case 25:
            fn_800FC0D4(&rot, &work->aim_0x328.rot_0x04);
            cpSetRotMatrix((struct _CP_VECTOR*)&rot, &mtx);
            fn_805012E8(&dst, &mtx);
            fn_8008EE68(holder, &dst);
            break;
        case 26:
            fn_800FC0D4(&rot, &work->aim_0x328.rot_0x10);
            cpSetRotMatrix((struct _CP_VECTOR*)&rot, &mtx);
            fn_805012E8(&dst, &mtx);
            fn_8008EE68(holder, &dst);
            break;
        }
        break;
    }
}

/* Places the aimed cluster the item's index selects. */
void fn_80191E30(EmUserData* self, EmMtxHolder* holder, u32 a2, u32 a3, u32 kind, EmUserItem* item) {
    MTX34 mtx;
    u32 index;
    EmActWork* work;

    work = self->work_0x04;
    MTX34_ctor(&mtx);
    switch (item->type_0x04) {
    case 0xFF:
        switch (kind) {
        case 16:
            index = 0;
            break;
        case 18:
            index = 1;
            break;
        case 20:
            index = 2;
            break;
        default:
            return;
        }
        if (work->clusters_0x590[index].live_0x03 >= 1) {
            fn_80139A7C(holder, &mtx);
            fn_800FBB90(&mtx, &work->clusters_0x590[index].pos_0x24);
            fn_80139A64(holder, &mtx);
        }
        break;
    }
}

/* Steps the five aim angles by their per-frame increments and clamps them. */
void fn_80191EF8(EmActWork* self) {
    EmAimRec* rec = &self->aim_0x328;
    u32 v;

    if (fn_8012EC60(self) == 1) {
        v = rec->angle_0x00 + 546;
        rec->angle_0x00 = v;
        if ((s16)v > 0) {
            rec->angle_0x00 = 0;
        }
        v = rec->rot_0x04.x + 455;
        rec->rot_0x04.x = v;
        if ((s16)v > 0) {
            rec->rot_0x04.x = 0;
        }
        v = rec->rot_0x04.y + 455;
        rec->rot_0x04.y = v;
        if ((s16)v > 0) {
            rec->rot_0x04.y = 0;
        }
        v = rec->rot_0x10.x + 455;
        rec->rot_0x10.x = v;
        if ((s16)v > 0) {
            rec->rot_0x10.x = 0;
        }
        v = rec->rot_0x10.y - 455;
        rec->rot_0x10.y = v;
        if ((s16)v < 0) {
            rec->rot_0x10.y = 0;
        }
    } else {
        v = rec->angle_0x00 - 546;
        rec->angle_0x00 = v;
        if ((s16)v < -5460) {
            rec->angle_0x00 = (u16)-5460;
        }
        v = rec->rot_0x04.x - 455;
        rec->rot_0x04.x = v;
        if ((s16)v < -4550) {
            rec->rot_0x04.x = (u16)-4550;
        }
        v = rec->rot_0x04.y - 455;
        rec->rot_0x04.y = v;
        if ((s16)v < -4550) {
            rec->rot_0x04.y = (u16)-4550;
        }
        v = rec->rot_0x10.x - 455;
        rec->rot_0x10.x = v;
        if ((s16)v < -4550) {
            rec->rot_0x10.x = (u16)-4550;
        }
        v = rec->rot_0x10.y + 455;
        rec->rot_0x10.y = v;
        if ((s16)v > 4551) {
            rec->rot_0x10.y = 4551;
        }
    }
}

/* Runs the aim vector's fade in or out and mirrors it into its follower. */
void fn_80192080(EmActWork* self) {
    EmAimRec* rec = &self->aim_0x328;
    f32 v;

    if (fn_8012EC3C(self) == 0) {
        v = rec->vec_0x1C.x - lbl_8079822C;
        rec->vec_0x1C.x = v;
        if (v > lbl_80797E9C) {
            rec->vec_0x1C.x = lbl_80797E9C;
        }
        rec->vec_0x1C.y = rec->vec_0x1C.x;
    } else {
        v = rec->vec_0x1C.x + lbl_8079822C;
        rec->vec_0x1C.x = v;
        if (v < lbl_8079800C) {
            rec->vec_0x1C.x = lbl_8079800C;
        }
        rec->vec_0x1C.y = rec->vec_0x1C.x;
    }
}

/* Fills in one effect request (id 0x17) from the default vector. */
void fn_80192108(EmEffRequest* out, u8 a, s16 b, s16 c) {
    if (lbl_80794AA0 == 0) {
        setVec3(&lbl_806A7A60, lbl_80797E88, lbl_80797E88, lbl_80797EB4);
        lbl_80794AA0 = 1;
    }
    out->id_0x00 = 0x17;
    copyVec3(&out->pos_0x04, &lbl_806A7A60);
    out->field_0x10 = a;
    out->field_0x12 = b;
    out->field_0x14 = c;
}

/* Releases a heap block when the flag asks for it, then hands the pointer back. */
void* fn_801921A8(void* p, u32 flag) {
    if (p != 0) {
        fn_8013918C(p, 0);
        if ((s16)flag > 0) {
            operator delete(p);
        }
    }
    return p;
}

/* Builds the five static vectors the effect requests start from. */
void fn_80192204(void) {
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;
    VEC3 v4;
    VEC3 v5;
    VEC3 v6;
    VEC3 v7;
    VEC3 v8;

    fn_80051490(&lbl_806A7A00.vec_0x00,
                setVec3(&v1, lbl_80797E88, lbl_807981C0, lbl_80797E88));
    fn_80051490(&lbl_806A7A00.vec_0x0C,
                setVec3(&v2, lbl_80797E88, lbl_80797E88, lbl_80797E88));
    fn_80051490(&lbl_806A7A18.vec_0x00,
                setVec3(&v3, lbl_80797E88, lbl_80798230, lbl_80797E88));
    fn_80051490(&lbl_806A7A18.vec_0x0C,
                setVec3(&v4, lbl_80797E88, lbl_80797E88, lbl_80797E88));
    fn_80051490(&lbl_806A7A30.vec_0x00,
                setVec3(&v5, lbl_80797E88, lbl_80797F80, lbl_80797E88));
    fn_80051490(&lbl_806A7A30.vec_0x0C,
                setVec3(&v6, lbl_80797E88, lbl_80798234, lbl_80797E88));
    fn_80051490(&lbl_806A7A48.vec_0x00,
                setVec3(&v7, lbl_80797E88, lbl_80797E88, lbl_80797EB4));
    fn_80051490(&lbl_806A7A48.vec_0x0C,
                setVec3(&v8, lbl_80797E88, lbl_80797F24, lbl_80797E88));
}

/* Sets the aim bits the caller's mask names. */
void fn_80192348(EmActWork* self, u8 mask) {
    self->aim_0x328.flags_0x35 |= mask;
}

/* Clears the aim bits the caller's mask names. */
void fn_80192358(EmActWork* self, u8 mask) {
    self->aim_0x328.flags_0x35 &= ~mask;
}

/* Reports whether every aim bit of the caller's mask is set.  The mask is narrowed in the source:
 * the target masks the incoming register (`clrlwi r0,r4,24`), which is what a `u8` parameter never
 * needs. */
u32 fn_80192370(EmActWork* self, u32 mask) {
    return (self->aim_0x328.flags_0x35 & (u8)mask) != 0;
}

/* Reports whether the damage part's level is in the aim group's admitted range (`u8 >= 2`). */
u32 fn_8019238C(EmActWork* self) {
    u8 v = em_parts_damage_level_get((struct _ENEMY_WORK*)self, 1);

    return v >= 2;
}

/* Reports whether the aim record's third flag byte is set. */
u32 fn_801923CC(EmActWork* self) {
    return self->aim_0x328.flag_0x33 != 0;
}

/* Reports whether the aim record's bit 0x02 is set. */
u32 fn_801923E0(EmActWork* self) {
    return fn_80192370(self, 2) == 1;
}

/* Reports whether the aim record's bit 0x08 is set. */
u32 fn_80192410(EmActWork* self) {
    return fn_80192370(self, 8) == 1;
}

/* Reads the aim vector's z. */
f32 fn_80192440(EmActWork* self) {
    return self->aim_0x328.vec_0x1C.z;
}

/* Reports whether another live work of team 0x12 already runs one of the four aim actions. */
s32 fn_80192448(EmActWork* self) {
    u16 max = get_move_work_max(3);
    u8 i;
    EmActWork* work = (EmActWork*)get_move_work_adrs(3);

    for (i = 0; (u32)i < max; i++, work++) {
        if (work->active == 0) {
            continue;
        }
        if (self == work) {
            continue;
        }
        if (work->team != 0x12) {
            continue;
        }
        if (self->act_id != work->act_id) {
            continue;
        }
        if (em_act_ck((struct _ENEMY_WORK*)work, 5, 0x1F) == 1 ||
            em_act_ck((struct _ENEMY_WORK*)work, 5, 0x20) == 1 ||
            em_act_ck((struct _ENEMY_WORK*)work, 5, 0x21) == 1 ||
            em_act_ck((struct _ENEMY_WORK*)work, 5, 0x22) == 1) {
            return 0;
        }
    }
    return 1;
}

/* Fires the aim effect at the work's 0x1A joint, scaled by the work's own scale. */
void fn_8019255C(EmActWork* self) {
    VEC3 out;
    VEC3 ofs;
    MTX34 mtx;

    VEC3_ctor(&out);
    VEC3_ctor(&ofs);
    MTX34_ctor(&mtx);
    fn_8012933C(self, 0, 0x12, 0x205);
    setVector3(&ofs, lbl_80797E88, lbl_80797E88, lbl_8079800C);
    get_joint_wmat_em((struct _ENEMY_WORK*)self, 0x1A, &mtx);
    mulVecMat(&ofs, &mtx);
    fn_80101428(&mtx, &ofs);
    fn_8010140C(&mtx, &out);
    senko_set(&out, lbl_80798238 * get_em_scale((struct _ENEMY_WORK*)self), self->act_id, 0);
}

/* Arms the two aim values and sets the aim record's bit 0x01. */
void fn_80192618(EmActWork* self) {
    self->aim_0x328.value_0x28 = lbl_80798244;
    self->aim_0x328.value_0x2C = lbl_80798248;
    fn_80192348(self, 1);
}

/* Initialises the aim record. */
void fn_80192630(EmActWork* self) {
    EmAimRec* rec = &self->aim_0x328;
    VEC3 v1;
    VEC3 v2;

    copyVec3(&rec->angle_0x00, setVec3(&v1, lbl_8079824C, lbl_8079824C, lbl_8079824C));
    copyVec3(&rec->rot_0x10, setVec3(&v2, lbl_8079824C, lbl_8079824C, lbl_8079824C));
    rec->vec_0x1C.x = lbl_80798238;
    rec->vec_0x1C.y = lbl_80798250;
    rec->vec_0x1C.z = lbl_80798254;
    rec->value_0x28 = lbl_8079824C;
    rec->value_0x2C = lbl_8079824C;
    rec->flag_0x30 = 0;
    rec->flag_0x31 = 0;
    rec->flag_0x32 = 0;
    rec->flag_0x33 = 0;
    rec->flag_0x34 = 0;
    rec->rot_0x04.z = (u32)-1;
    rec->flags_0x35 = 0;
    rec->value_0x36 = 0;
    rec->value_0x38 = 0;
    rec->value_0x3A = 0;
}

#ifdef __cplusplus
}
#endif
