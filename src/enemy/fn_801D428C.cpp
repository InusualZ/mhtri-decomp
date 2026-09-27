/* enemy/fn_801D428C.cpp - the `em0xx` enemy's action band, `.text` 0x801D428C..0x801D80EC (42
 * functions).
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup`: every address of the range answers `zz_XXXXXXXX_` in the
 * shared runtime dump and carries a bare `fn_XXXXXXXX = .text:0x...` entry in
 * config/RMHE08/symbols.txt; no `__FILE__` string is reachable from the range - the only `enemy`
 * source name in the image is `enemy_control.cpp`, and it belongs to the registered
 * `enemy/enemy_control.cpp` 10 KB below).
 *
 * What it is.  One enemy's action/state band, the same family as the neighbouring registered units
 * `enemy/fn_80191598.cpp` (below) and `enemy/fn_801550FC.cpp` (above):
 *
 *   * the two dispatchers `fn_801D4C3C` (on `_ENEMY_WORK::state_sub`, +0x1E6) and `fn_801D4C90` (on
 *     `action`, +0x1E5) tail-branch into the action functions of the *unclaimed* band below
 *     (0x801CB308..0x801D3CF8) and into this range's own `fn_801D428C`/`fn_801D4B84`; the 13-way
 *     `fn_801D8078` dispatches `state_sub` over the range's own step functions;
 *   * the per-state step functions (`fn_801D771C`, `fn_801D7798`, `fn_801D7814`, `fn_801D7890`,
 *     `fn_801D797C`, `fn_801D79F8`, `fn_801D7F04`, `fn_801D7F80`, `fn_801D7FFC`, `fn_801D7A74`,
 *     `fn_801D7BF8`, `fn_801D7D14`, `fn_801D7E44`) - each opens on `switch (self->state)` (+0x05),
 *     state 0 arming the action (`em_move_mode_set` + a motion setter) and state 1 waiting for the motion
 *     (`em_mot_end_ck`) before closing it (`em_action_finish`/`fn_801280F4`); `fn_801D791C` is the
 *     intermediate `state_sub` dispatcher over them;
 *   * the aim/effect body `fn_801D4CE0`, the effect-spawn body `fn_801D4DD8` (through
 *     `eft009_set_pos`/`fn_801048B4`/`fn_8010D2B0`), the frame gate `fn_801D6694` and the target
 *     selectors `fn_801D6758`/`fn_801D67D8`/`fn_801D68A8`/`fn_801D6DA4`/`fn_801D75D0`;
 *   * the class plumbing `fn_801D71C4` (attaches the 12-byte helper `fn_801D752C` builds through
 *     `fn_801390FC`), its base constructor `fn_801D752C` and the action callback `fn_801D756C`.
 *
 * Module and name (brief section 2, in evidence order).
 *   1. No `__FILE__` string covers the range (see above).
 *   2. `dumpmap.py lookup` answers `zz_01d428c_` (a placeholder is not evidence).
 *   3. The code is enemy-band: every function takes the shared `_ENEMY_WORK`, the range's own data
 *      tables (`jumptable_805B64F8`, `lbl_805B75B8`, `lbl_805B6950`) sit in the enemy `.data` run, and
 *      the band's naming scheme is the map's own `fn_XXXXXXXX` stem.
 * The file therefore keeps the map stem (brief option 4); no name was invented.
 *
 * Seam.  Unproven, as the brief says: the left edge at 0x801D428C is the discovery cut and the band
 * below it (0x801CB308..0x801D3CF8, whose functions this range's dispatchers tail-call) is unclaimed,
 * so the two very probably belong to one TU.  The right edge at 0x801D80EC is evidence: `fn_801D8078`
 * ends exactly there and the next discovered run starts at 0x801D80EC.
 *
 * Language.  C++: the range's callees include C++ manglings (`setVector3__FPQ34nw4r4math4VEC3fff`,
 * `em_frame_check__FP11_ENEMY_WORKUsff`, `getTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor`,
 * `__nw__FUl`), the `MHchar` at `_ENEMY_WORK::char_0x024` is called as a class and the range installs
 * a vtable-like class descriptor (`lbl_805B75B8`).  Rule 9: those are declared at C++ scope with the
 * signature their mangling encodes and called through it; every `fn_*` definition stays `extern "C"`.
 *
 * Types.  `_ENEMY_WORK` is the shared record `include/enemy/ENEMY_WORK.h` owns (rule 1).  This unit
 * added the bytes it measures to that header: +0x1B0 (the aim vector `copyVec3` copies into),
 * +0x20C, +0x380/+0x381, +0x452, +0x76C, +0x836, +0x9F6, and the +0x328 block - two vectors plus the
 * two flags `fn_801D71C4` clears, reached through the union member `action_0x328` because
 * `enemy/fn_80170600.cpp`'s `timers_0x328` view names the same 44 bytes.
 *
 * Declarations.  This file declares its foreign callees itself, the way the neighbouring
 * `enemy/fn_801550FC.cpp`/`enemy/fn_80177890.cpp` do.  The enemy band's headers do not fit here:
 * `include/unsplit/enemy.h` carries the old 0-argument spellings of `fn_8012EC60`/`fn_8012EC3C`
 * (`u32 fn_8012EC60(void)`) although both bodies read the work record (+0x8AA/+0x89F - settled from
 * the callees), and `include/enemy/fn_801251D0.h` publishes `fn_80126278(u16, VEC3*)` although the
 * callee's body takes `self` in r3, the id in r4 and the output in r5.  MWCC rejects the two
 * C-linkage spellings in one TU (`10197 illegal function overloading`), so this unit's declarations
 * are the call sites' and the owner headers' corrections are recorded as a `shared-file` request in
 * the outbox.
 *
 * Residuals.  `fn_801D428C` (0x8F8, the 16-case action body) and `fn_801D4F78` (0x152C, the 12-case
 * engine) are NOT written: both are jump-table state machines over the same fields and are the next
 * pass's work.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "sound/mhchar.h"
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* ----------------------------------------------------------------------------------------------------
 * the pool the range reads (owned elsewhere; declared, never defined - playbook 29)
 * -------------------------------------------------------------------------------------------------- */

extern f32 lbl_80799220;
extern f32 lbl_80799224;
extern f32 lbl_80799230;
extern f32 lbl_80799238;
extern f32 lbl_8079923C;
extern f32 lbl_80799240;
extern f32 lbl_80799244;
extern f32 lbl_80799248;
extern f32 lbl_8079924C;
extern f32 lbl_80799254;
extern f32 lbl_8079925C;
extern f32 lbl_80799260;
extern f32 lbl_80799264;
extern f32 lbl_80799270;
extern f32 lbl_80799274;
extern f32 lbl_80799280;
extern f32 lbl_80799288;
extern f32 lbl_8079928C;
extern f32 lbl_80799294;
extern f32 lbl_807992B0;
extern f32 lbl_807992BC;
extern f32 lbl_807992C0;
extern f32 lbl_807992E8;
extern f32 lbl_807992F4;
extern f32 lbl_80799308;
extern f32 lbl_8079930C;
extern f32 lbl_80799310;
extern f32 lbl_80799318;
extern f32 lbl_8079931C;
extern f32 lbl_80799324;
extern f32 lbl_80799348;
extern f32 lbl_8079934C;
extern f32 lbl_80799354;
extern f32 lbl_8079935C;
extern f32 lbl_80799368;
extern f32 lbl_8079936C;
extern f32 lbl_80799378;
extern f32 lbl_80799380;
extern f32 lbl_80799388;
extern f32 lbl_80799390;
extern f32 lbl_8079939C;
extern f32 lbl_807993A0;
extern f32 lbl_807993A4;
extern f32 lbl_807993AC;
extern f32 lbl_807993E8;
extern f32 lbl_807993EC;
extern f32 lbl_807993F0;
extern f32 lbl_807993F4;
extern f32 lbl_807993F8;
extern f32 lbl_807993FC;
extern f32 lbl_80799400;
extern f32 lbl_80799404;
extern f32 lbl_80799408;
extern f32 lbl_8079940C;
extern f32 lbl_80799410;
extern f32 lbl_80799414;
extern f32 lbl_80799418;
extern f32 lbl_8079941C;
extern f32 lbl_80799420;
extern f32 lbl_80799424;
extern f32 lbl_80799428;
extern f32 lbl_8079942C;
extern f32 lbl_80799430;
extern f32 lbl_80799434;
extern f32 lbl_80799438;
extern f32 lbl_8079943C;
extern f32 lbl_80799440;
extern f32 lbl_80799444;
extern f32 lbl_80799448;
extern f32 lbl_8079944C;
extern f32 lbl_80799450;
extern f32 lbl_80799454;
extern f32 lbl_80799458;
extern f32 lbl_8079945C;
extern f32 lbl_80799460;
extern f32 lbl_80799464;
extern f32 lbl_80799468;
extern f32 lbl_8079946C;
extern f32 lbl_80799470;
extern f32 lbl_80799474;
extern f32 lbl_80799478;
extern f32 lbl_8079947C;
extern f32 lbl_80799480;
extern f32 lbl_80799484;
extern f32 lbl_80799488;
extern f32 lbl_8079948C;
extern f32 lbl_80799490;
extern f32 lbl_80799494;
extern f32 lbl_80799498;
extern f32 lbl_8079949C;
extern f32 lbl_807994A0;
extern f32 lbl_807994A4;
extern f32 lbl_807994A8;
extern f32 lbl_807994AC;
extern f32 lbl_807994B0;
extern f32 lbl_807994B4;
extern f32 lbl_807994B8;
extern f32 lbl_807994BC;
extern f32 lbl_807994C0;
extern f32 lbl_807994C4;
extern f32 lbl_807994C8;
extern f32 lbl_807994CC;
extern f32 lbl_807994D0;
extern f32 lbl_807994D4;
extern f32 lbl_807994D8;
extern f32 lbl_807994DC;
extern f32 lbl_807994E0;
extern f32 lbl_807994E4;
extern f32 lbl_807994E8;
extern f32 lbl_807994EC;
extern f32 lbl_807994F0;
extern f32 lbl_807994F8;
extern f32 lbl_807994FC;
extern f32 lbl_80799500;
extern f32 lbl_80799504;
extern f32 lbl_80799508;
extern f32 lbl_8079950C;
extern f32 lbl_80799510;
extern f32 lbl_80799514;
extern f32 lbl_80799518;
extern f32 lbl_8079951C;

/* the range's own `.data` tables (no `.data` range is registered for this unit yet, so they stay the
 * shared pool's bytes; only the ones the code loads explicitly are declared). */
extern u8 lbl_805B52D8[];
extern u8 lbl_805B5310[];
extern u8 lbl_805B531C[];
extern u8 lbl_805B61F8[];
extern u8 lbl_805B63A8[];
extern u8 lbl_805B63E8[];
extern u8 lbl_805B6950[];
extern u8 lbl_805B75B8[];
extern u8 lbl_806A7B30[];
extern u8 lbl_806A7B48[];
extern u8 lbl_806A7B60[];
extern u8 lbl_806A7B78[];

/* ----------------------------------------------------------------------------------------------------
 * the C++-mangled callees (rule 9: declared with the signature the mangling encodes, called through it)
 * -------------------------------------------------------------------------------------------------- */

u16 calcVecAngX(nw4r::math::VEC3* v);                       /* calcVecAngX__FPQ34nw4r4math4VEC3 */
f32 calcDistanceSqXZ(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
                                                            /* calcDistanceSqXZ__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3 */
void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);  /* setVector3__FPQ34nw4r4math4VEC3fff */
void rotVecY(nw4r::math::VEC3* v, u32 angle);               /* rotVecY__FPQ34nw4r4math4VEC3Ul */
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                            /* em_frame_check__FP11_ENEMY_WORKUsff */
s32 em_die_ck(struct _ENEMY_WORK* self);                    /* em_die_ck__FP11_ENEMY_WORK */
u16 em_get_mot_no(struct _ENEMY_WORK* self);                /* em_get_mot_no__FP11_ENEMY_WORK */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
                                                            /* em_parts_damage_level_get__FP11_ENEMY_WORKUc */
f32 get_em_chg_scale(struct _ENEMY_WORK* self);             /* get_em_chg_scale__FP11_ENEMY_WORK */
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
                                                            /* get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3 */
void* get_move_work_adrs(u8 index);                         /* get_move_work_adrs__FUc */
u16 get_move_work_max(u8 index);                            /* get_move_work_max__FUc */
void eft009_set_pos(u8 kind, nw4r::math::VEC3* pos, void* rot, f32 scale, u32 area);
                                                            /* eft009_set_pos__FUcPQ34nw4r4math4VEC3P10_CP_VECTORfUl */

/* the runtime's allocator pair; spelling the manglings `__nw__FUl`/`__dl__FPv` as identifiers would be
 * rule 9's violation, so the C++ definitions the compiler mangles to them are declared and called. */
void* operator new(unsigned long size);
void operator delete(void* ptr) throw();

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------------------------------
 * the enemy-band callees
 *
 * The signatures are the call sites' (the argument counts are the ones the callers set in r4..r6 and
 * f1..f3); where a callee's body disagrees with its owner header, the body won and the correction is
 * recorded in the outbox.
 * -------------------------------------------------------------------------------------------------- */

/* enemy/enemy_control.cpp (0x801411B8..0x80147CE0) */
s16 fn_80145FE4();
u32 fn_80146008(u32 id);
void fn_80146058(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void fn_8014610C(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void fn_8014616C(struct _ENEMY_WORK* self, u32 a);
void fn_801461A8(struct _ENEMY_WORK* self, s16 a, void* b, u32 c);
void fn_801462A4(struct _ENEMY_WORK* self, s16 a, void* b, void* c, u32 d, u32 e);
void fn_8014619C(struct _ENEMY_WORK* self);

/* enemy/fn_801251D0.cpp (0x801251D0..0x8012BA00).  `fn_80126278` takes the work record in r3 (the
 * callee's body does `mr r30,r4` / `mr r31,r5` before `fn_801261D8`), which the owner header
 * (`void fn_80126278(u16 id, VEC3* out)`) does not carry - corrected here. */
void fn_80126278(struct _ENEMY_WORK* self, u16 id, nw4r::math::VEC3* out);
void em_action_finish(struct _ENEMY_WORK* self);
void fn_801280F4(struct _ENEMY_WORK* self);
void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80128A8C(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_8012933C(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void fn_80129668(struct _ENEMY_WORK* self, u32 a, u32 b);
u32 fn_80129A70(struct _ENEMY_WORK* self, u16 a);
u8 fn_80129DB8(struct _ENEMY_WORK* self);
u32 fn_8012A014(struct _ENEMY_WORK* self, u32 a, u32 b, u16 c, void* d, void* e);
s32 fn_8012A204(struct _ENEMY_WORK* self);

/* enemy/fn_8012BDF4.cpp (0x8012BDF4..0x8012E968) */
void em_busy_set(struct _ENEMY_WORK* self);
u32 fn_8012E5A8(struct _ENEMY_WORK* self);

/* enemy/fn_8012E968.cpp (0x8012E968..0x8012EC74) - both bodies read the work record (settled from the
 * callees: `fn_8012EC3C` loads +0x89F, `fn_8012EC60` loads +0x8AA). */
u32 fn_8012EC3C(struct _ENEMY_WORK* self);
u32 fn_8012EC60(struct _ENEMY_WORK* self);

/* enemy/fn_8012EC74.cpp (0x8012EC74..0x80137604) */
void fn_8012F504(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d);
void em_mot_set(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_mot_set_ck(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
u32 em_mot_end_ck(struct _ENEMY_WORK* self);
s32 fn_8012F948(struct _ENEMY_WORK* self);
void fn_80129724(struct _ENEMY_WORK* self, u32 a);
void fn_80130248(struct _ENEMY_WORK* self);
void em_mot_speed_set(struct _ENEMY_WORK* self, f32 a);
u32 fn_8013023C(struct _ENEMY_WORK* self);
f32 fn_8013032C(struct _ENEMY_WORK* self);
void fn_801303EC(struct _ENEMY_WORK* self, f32 a);
void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
void fn_801305C4(struct _ENEMY_WORK* self);
void fn_80133C3C(struct _ENEMY_WORK* self);
void fn_801354F4(struct _ENEMY_WORK* self, void* p);
f32 fn_80135644(struct _ENEMY_WORK* self, void* tbl);
void fn_801369A0(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c);
void fn_80130CDC(struct _ENEMY_WORK* self, u32 a);
void fn_80130F74(struct _ENEMY_WORK* self);
struct _ENEMY_WORK* fn_80131034(struct _ENEMY_WORK* self, u8 kind, u8 distance_check);
void fn_8013221C(struct _ENEMY_WORK* self, f32 a, u32 b, u32 c);
void fn_80132224(struct _ENEMY_WORK* self);
void fn_80132264(struct _ENEMY_WORK* self);
u8* fn_801377D0(u8 index);
void fn_80135C5C(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80136B50(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_801376B4(struct _ENEMY_WORK* self);

/* enemy/fn_80138074.c (0x80138074..0x8013ACC4) */
void fn_801390FC(struct _ENEMY_WORK* self, void* helper);
void fn_8013918C(void* helper, s16 flag);
s32 fn_801391E8(struct _ENEMY_WORK* self);

/* enemy/fn_80147CE0.cpp (0x80147CE0..0x80149D6C) */
void* fn_80147E2C(void* self);

/* this range's own not-yet-written bodies (`fn_801D4C3C` dispatches into `fn_801D428C`);
 * declared so the dispatchers compile, written in the next pass (see the residual note above). */

/* ef module */
void fn_801048B4(struct _ENEMY_WORK* self, u32 a, s32 b, f32 c);
void fn_801049D0(struct _ENEMY_WORK* self, u32 id, u32 type, s32 joint, nw4r::math::VEC3* pos,
                 f32 scale);
void fn_8010562C(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c);
void fn_801057FC(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c, s32 d);
void fn_80106694(struct _ENEMY_WORK* self, void* pos, u8 a, f32 b);
void fn_8010D2B0(void* pos, u8 a, u8 b, s32 c, f32 d);
void fn_800FA378(void* out);
void fn_800FA3B8(void* out);
void fn_80051490(void* out, const void* in);
void fn_80051378(void* out, const void* in, const void* v);
void fn_8004FFC8(void* a, void* b, void* c, f32 d);
void fn_80056A54(struct _ENEMY_WORK* self, u32 a, u32 b);

/* the unclaimed enemy action band below this range (0x801CB308..0x801D3CF8) - the dispatchers'
 * tail-call targets.  Their band header is `include/unsplit/enemy.h` and the move is recorded as a
 * `shared-file` request in the outbox; until it lands this file carries its own copy. */
void fn_801CB308(struct _ENEMY_WORK* self);
void fn_801CB9DC(struct _ENEMY_WORK* self);
void fn_801CCCE8(struct _ENEMY_WORK* self);
void fn_801CE898(struct _ENEMY_WORK* self);
void fn_801CF648(struct _ENEMY_WORK* self);
void fn_801D290C(struct _ENEMY_WORK* self);
void fn_801D2B10(struct _ENEMY_WORK* self);
void fn_801D2E0C(struct _ENEMY_WORK* self);
void fn_801D2EBC(struct _ENEMY_WORK* self);
void fn_801D2ED0(struct _ENEMY_WORK* self);
void fn_801D320C(struct _ENEMY_WORK* self);
void fn_801D3564(struct _ENEMY_WORK* self);
void fn_801D38A0(struct _ENEMY_WORK* self);
void fn_801D3C38(struct _ENEMY_WORK* self);
void fn_801D3CF8(struct _ENEMY_WORK* self, u32 a);

/* the unclaimed band above this range (0x801D80EC..0x801EC9E0), bracketed by `enemy` below and
 * `lobby` above - rule 2's documented gap, so the declaration stays here. */
void fn_801E2D04(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c, u32 d);

/* the same gap in the other modules: 0x802Bxxxx sits between `Pl/pl_act.cpp` and
 * `stage/fn_802B2978.c`, 0x8030xxxx between `ai/fn_802D0DCC.c` and `ef/fn_803066F0.c`. */
s32 fn_8028F558(void* a, void* b);
s32 fn_802907BC(s32 a, void* b);
void fn_802B43A8(void* pos, u8 a, u16 b);
void fn_80304508(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c);

/* ----------------------------------------------------------------------------------------------------
 * the records this unit needs locally
 * -------------------------------------------------------------------------------------------------- */

/* The 12-byte helper `fn_801D71C4` allocates and hands `fn_801390FC`; `fn_801D752C` is its
 * constructor (it chains `fn_80147E2C` and stores the class descriptor `lbl_805B75B8` at +0x00).
 * size: 0x0C (traced from the `operator new(0xC)` in `fn_801D71C4`). */
struct EmHelper801D428C {
    /* +0x00 */ void* vtbl;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
};

/* The two stack scratch records `fn_801D66BC` builds: `fn_800FA3B8`/`fn_800FA378` fill them,
 * `fn_8004FFC8` reads the first and `fn_8028F558` compares them.  No function reads their contents, so
 * only their sizes (the target's own frame layout) are stated.
 * size: 0x20 */
struct EmScratchA {
    /* +0x00 */ u8 unused_0x00[0x20];
};

/* size: 0x40 */
struct EmScratchB {
    /* +0x00 */ u8 unused_0x00[0x40];
};

/* The record `fn_801377D0` hands back for an area entry, as `fn_801D6758` reads it (the `_ENEMY_WORK`
 * shape at +0x00/+0x16/+0x5A6).
 * size: 0x5A8 (only the three bytes the caller reads are named) */
struct EmAreaEntry801D428C {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 unused_0x001[0x016 - 0x001];
    /* +0x016 */ u8 area_no;
    /* +0x017 */ u8 unused_0x017[0x5A6 - 0x017];
    /* +0x5A6 */ u8 flags_0x5A6;
    /* +0x5A7 */ u8 unused_0x5A7[0x5A8 - 0x5A7];
};

/* One 0x1C-byte entry of the `lbl_805B6950` table `fn_801D66BC` indexes (`arg0 * 0x1C`): the two
 * 0xC-byte records `fn_8004FFC8` interpolates between, and the scale it takes at +0x18.
 * size: 0x1C */
struct EmGrowTable801D428C {
    /* +0x00 */ u8 rec_a[0xC];
    /* +0x0C */ u8 rec_b[0xC];
    /* +0x18 */ f32 scale;
};

/* The 0x18-byte spawn record `fn_801D6EDC` fills (`id` word, a vector, then the three scalars).
 * size: 0x18 */
struct EmSpawnRec801D428C {
    /* +0x00 */ u32 id;
    /* +0x04 */ nw4r::math::VEC3 pos;
    /* +0x10 */ u8 field_0x10;
    /* +0x12 */ u16 field_0x12;
    /* +0x14 */ u16 field_0x14;
};

/* ====================================================================================================
 * bodies
 * ================================================================================================== */

/* this range's own functions that a later body calls before its definition */
void* fn_801D752C(EmHelper801D428C* self);
void fn_801D4CE0(struct _ENEMY_WORK* self);
void fn_801D4DD8(struct _ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 arg5);

void fn_801D4F78(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 spot;
    u32 area_flag;
    u8 area;
    u8 flags;

    VEC3_ctor(&spot);
    fn_801D4CE0(self);
    switch (em_get_mot_no(self)) {
    case 0x8:
        if ((em_after_frame_check(self, 0, lbl_807993A4, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_80799444, lbl_80799220) == 1)) {
            fn_801D4DD8(self, 0, 0, 0x25, 0, lbl_8079925C);
            fn_801D4DD8(self, 0, 0, 0x2D, 0, lbl_8079925C);
        }
        if ((em_after_frame_check(self, 0, lbl_807992F4, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_8079934C, lbl_80799220) == 1)) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
            fn_80136B50(self, -1, 0);
        }
        if ((em_after_frame_check(self, 0, lbl_8079944C, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_80799450, lbl_80799220) == 1)) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
        }
        break;
    case 0x9:
        if (em_after_frame_check(self, 0, lbl_80799390, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799454);
            fn_80136B50(self, 8, 7);
        }
        if (em_after_frame_check(self, 0, lbl_80799274, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799458);
            fn_80136B50(self, 0x11, 7);
        }
        break;
    case 0xC:
        if (em_after_frame_check(self, 0, lbl_80799348, lbl_80799220) == 1) {
            fn_8012933C(self, 0, 0x14, 8);
            fn_8012933C(self, 1, 0x15, 0x10);
        }
        if (em_after_frame_check(self, 0, lbl_8079930C, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 0, 0x11, 0, lbl_8079925C);
        }
        break;
    case 0xD:
        if (em_after_frame_check(self, 0, lbl_80799348, lbl_80799220) == 1) {
            fn_8012933C(self, 0, 0x16, 8);
            fn_8012933C(self, 1, 0x17, 0x10);
        }
        if (em_after_frame_check(self, 0, lbl_8079930C, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 0, 8, 0, lbl_8079925C);
        }
        break;
    case 0x1B:
        if (em_after_frame_check(self, 0, lbl_807992C0, lbl_80799220) == 1) {
            fn_801D4DD8(self, 2, 0, 3, 0, lbl_80799230);
        }
        break;
    case 0x1E:
        if (em_after_frame_check(self, 0, lbl_8079945C, lbl_80799220) == 1) {
            fn_801D4DD8(self, 2, 0, 3, 0, lbl_80799230);
        }
        break;
    case 0x25:
    case 0x26:
        if (em_after_frame_check(self, 0, lbl_807992B0, lbl_80799220) == 1) {
            fn_801D4DD8(self, 2, 0, 3, 0, lbl_80799230);
        }
        break;
    case 0x29:
        if (em_after_frame_check(self, 0, lbl_807992F4, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 5, 3, 0, lbl_80799460);
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079928C);
            fn_801369A0(self, 1, 0, &spot, lbl_80799460);
        }
        break;
    case 0x2A:
        if (em_after_frame_check(self, 0, lbl_80799378, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 5, 3, 0, lbl_80799460);
        }
        break;
    case 0x41:
        if (em_after_frame_check(self, 0, lbl_80799318, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 3, 0, lbl_80799288);
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079928C);
            fn_801369A0(self, 2, 0, &spot, lbl_80799368);
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0x44:
        if (em_after_frame_check(self, 0, lbl_807992C0, lbl_80799220) == 1) {
            fn_801D4DD8(self, 2, 0, 3, 0, lbl_80799230);
        }
        break;
    case 0x64:
        if (em_after_frame_check(self, 0, lbl_807993AC, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
        }
        if (em_after_frame_check(self, 0, lbl_80799310, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
        }
        break;
    case 0x65:
        if (em_after_frame_check(self, 0, lbl_80799464, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
        }
        if (em_after_frame_check(self, 0, lbl_80799308, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
        }
        break;
    case 0x66:
        if (em_after_frame_check(self, 0, lbl_80799404, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 0, 4, 0x2AAB, lbl_80799260);
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0x67:
        if (em_after_frame_check(self, 0, lbl_80799404, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 0, 4, 0xD556, lbl_80799260);
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0x71:
        if (em_after_frame_check(self, 0, lbl_80799348, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 5, 4, 0, lbl_80799240);
            fn_80136B50(self, -1, 1);
        }
        break;
    case 0x72:
        if (em_after_frame_check(self, 0, lbl_80799468, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 1, 0x19, 0, lbl_8079924C);
        }
        break;
    case 0x74:
        if (em_after_frame_check(self, 0, lbl_8079946C, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 1, 0x19, 0, lbl_8079924C);
        }
        break;
    case 0x78:
        if (em_after_frame_check(self, 0, lbl_80799274, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 0, 0x25, 0, lbl_8079925C);
        }
        if (em_after_frame_check(self, 0, lbl_80799468, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 0, 0x2D, 0, lbl_8079925C);
        }
        if (em_after_frame_check(self, 0, lbl_80799308, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 0x18, 0, lbl_80799288);
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0x7D:
        if (em_after_frame_check(self, 0, lbl_80799310, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_80799288);
        }
        if (em_after_frame_check(self, 0, lbl_80799470, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 1, 4, 0, lbl_80799240);
        }
        break;
    case 0x7F:
        if (em_after_frame_check(self, 0, lbl_80799388, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 2, 4, 0, lbl_8079925C);
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0x80:
        if (em_after_frame_check(self, 0, lbl_80799474, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x1C, 0, lbl_80799454);
        }
        break;
    case 0x82:
        if (em_after_frame_check(self, 0, lbl_80799478, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 1, 0x1A, 0, lbl_807993A0);
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0x83:
        if (em_after_frame_check(self, 0, lbl_80799468, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
            fn_80136B50(self, -1, 7);
        }
        if ((em_after_frame_check(self, 0, lbl_80799354, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_8079944C, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_8079939C, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_80799308, lbl_80799220) == 1)) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
        }
        break;
    case 0x84:
        if (em_after_frame_check(self, 0, lbl_8079947C, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
            fn_80136B50(self, -1, 7);
        }
        if (em_after_frame_check(self, 0, lbl_80799468, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
        }
        break;
    case 0xC9:
    case 0xCA:
        if (em_after_frame_check(self, 0, lbl_807993AC, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 6, 4, 0, lbl_80799348);
        }
        break;
    case 0xCB:
    case 0xCC:
        if ((em_after_frame_check(self, 0, lbl_807992B0, lbl_80799220) == 1) || (em_after_frame_check(self, 0, lbl_80799308, lbl_80799220) == 1)) {
            fn_801D4DD8(self, 2, 0, 4, 0, lbl_80799230);
        }
        break;
    case 0xCD:
    case 0xCF:
        if (em_after_frame_check(self, 0, lbl_80799294, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 5, 4, 0, lbl_80799460);
        }
        break;
    case 0xCE:
    case 0xD0:
        if (em_after_frame_check(self, 0, lbl_807993A4, lbl_80799220) == 1) {
            fn_801D4DD8(self, 2, 0, 4, 0, lbl_80799230);
        }
        break;
    case 0xD1:
        if (em_after_frame_check(self, 0, lbl_80799378, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_807993A0);
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0xD2:
        if (em_after_frame_check(self, 0, lbl_8079936C, lbl_80799220) == 1) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079923C);
            fn_801049D0(self, 8, 2, 0, &spot, lbl_80799448);
        }
        if (em_after_frame_check(self, 0, lbl_80799380, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
        }
        break;
    case 0xD3:
        if (em_after_frame_check(self, 0, lbl_8079936C, lbl_80799220) == 1) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079923C);
            fn_801049D0(self, 0x11, 2, 0, &spot, lbl_80799448);
        }
        if (em_after_frame_check(self, 0, lbl_80799380, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
        }
        break;
    case 0xD4:
        if ((u8) self->action == 0xA) {
            if (em_after_frame_check(self, 0, lbl_80799318, lbl_80799220) == 1) {
                fn_8012933C(self, 0, 3, 8);
                fn_8012933C(self, 1, 4, 0x18);
            }
            if (em_frame_check(self, 0, lbl_8079935C, lbl_80799220) == 1) {
                fn_80129724(self, 1);
                fn_8012933C(self, 1, 0x10, 0x10);
            }
        }
        if (em_after_frame_check(self, 0, lbl_80799270, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
            fn_80136B50(self, 8, 7);
        }
        if (em_after_frame_check(self, 0, lbl_80799378, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
            fn_80136B50(self, 0x11, 7);
        }
        break;
    case 0xD5:
        if (em_after_frame_check(self, 0, lbl_80799380, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x2D, 0, lbl_80799348);
        }
        if (em_after_frame_check(self, 0, lbl_80799324, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799348);
        }
        if (em_after_frame_check(self, 0, lbl_80799468, lbl_80799220) == 1) {
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0xD7:
        if (em_after_frame_check(self, 0, lbl_80799318, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_80799460);
            fn_80136B50(self, -1, 1);
        }
        break;
    case 0xD9:
        if (em_after_frame_check(self, 0, lbl_80799380, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x25, 0, lbl_80799348);
        }
        if (em_after_frame_check(self, 0, lbl_80799324, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799348);
        }
        if (em_after_frame_check(self, 0, lbl_80799468, lbl_80799220) == 1) {
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0xDB:
        if (em_after_frame_check(self, 0, lbl_80799318, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_80799460);
            fn_80136B50(self, -1, 1);
        }
        break;
    case 0xDC:
        if (((s32) (self->flags_0x836 & 1) == 0) && (em_after_frame_check(self, 0, lbl_80799480, lbl_80799220) == 1)) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_80799220);
            fn_801E2D04(self, 1, 0x1A, &spot, 0xC, lbl_80799240);
            fn_801E2D04(self, 1, 0x1C, &spot, 0xC, lbl_80799240);
        }
        break;
    case 0xDD:
        if (((s32) (self->flags_0x836 & 1) == 0) && (em_after_frame_check(self, 0, lbl_80799480, lbl_80799220) == 1)) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_80799220);
            fn_801E2D04(self, 1, 0x19, &spot, 0xC, lbl_80799240);
            fn_801E2D04(self, 1, 0x1B, &spot, 0xC, lbl_80799240);
        }
        break;
    case 0xDE:
    case 0xE1:
        if (em_after_frame_check(self, 0, lbl_80799484, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_80799460);
        }
        break;
    case 0xDF:
        if (em_after_frame_check(self, 0, lbl_80799488, lbl_80799220) == 1) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_80799220);
            fn_80304508(self, 0x7C, 0x14, &spot, lbl_80799240);
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0xE2:
        if (em_after_frame_check(self, 0, lbl_80799488, lbl_80799220) == 1) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_80799220);
            fn_80304508(self, 0x7D, 0xB, &spot, lbl_80799240);
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0xE4:
        if (em_after_frame_check(self, 0, lbl_80799388, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 0x11, 0, lbl_80799448);
        }
        if (em_after_frame_check(self, 0, lbl_807993AC, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 2, 8, 0, lbl_80799448);
        }
        break;
    case 0xE9:
        if (em_after_frame_check(self, 0, lbl_80799388, lbl_80799220) == 1) {
            fn_801D4DD8(self, 2, 0, 3, 0, lbl_80799230);
        }
        break;
    case 0xEC:
        if (em_after_frame_check(self, 0, lbl_80799274, lbl_80799220) == 1) {
            fn_801D4DD8(self, 0, 5, 3, 0, lbl_80799460);
        }
        break;
    case 0xED:
        if (em_after_frame_check(self, 0, lbl_80799348, lbl_80799220) == 1) {
            fn_801369A0(self, 1, 0, NULL, lbl_80799240);
        }
        break;
    case 0xEF:
        if (em_after_frame_check(self, 0, lbl_80799274, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_807993A0);
            fn_80136B50(self, -1, 1);
        }
        break;
    case 0xF0:
    case 0xF1:
        if (em_after_frame_check(self, 0, lbl_80799388, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_807993A0);
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0xF2:
    case 0xF3:
        if (em_after_frame_check(self, 0, lbl_80799388, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_807993A0);
            fn_80136B50(self, -1, 7);
        }
        break;
    case 0xF6:
        if (em_after_frame_check(self, 0, lbl_80799348, lbl_80799220) == 1) {
            fn_801D4DD8(self, 1, 4, 4, 0, lbl_807993A0);
            fn_80136B50(self, -1, 1);
        }
        break;
    }
    area_flag = 0;
    if ((u8)fn_802B0668(self->field_0x1E0) == 4 && (self->area_no == 4 || self->area_no == 6)) {
        area_flag = 1;
    }
    if (fn_8012EC60(self) == 1 || area_flag == 1) {
        self->field_0x761 = (u8)(self->field_0x761 | 1);
    } else {
        flags = self->field_0x761;
        if ((flags & 1) != 0) {
            self->field_0x761 = (u8)(flags & 0xFE);
        }
    }
    if (area_flag == 1 && self->field_0x762 == 0) {
        self->field_0x761 = (u8)(self->field_0x761 | 2);
        return;
    }
    flags = self->field_0x761;
    if ((flags & 2) != 0) {
        self->field_0x761 = (u8)(flags & 0xFD);
    }
}

void fn_801D428C(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 spot;

    VEC3_ctor(&spot);
    switch (self->state) {
    case 0:
        self->state++;
        self->timer_0x020 = 0;
        fn_80130248(self);
        fn_801305C4(self);
        em_mot_set(self, 1, 0, 0);
        fn_8014616C(self, 0);
        break;
    case 1:
        if (fn_80146008(0x10E) == 1) {
            self->state++;
            fn_8014619C(self);
            em_mot_set(self, 1, 0, 0);
            fn_8014610C(self, lbl_807993E8, lbl_807993EC, lbl_807993F0);
            fn_80146058(self, lbl_807993F4, lbl_807993F8, lbl_807993FC);
        }
        break;
    case 2:
        fn_8014610C(self, lbl_807993E8, lbl_807993EC, lbl_807993F0);
        fn_80133C3C(self);
        if (fn_80146008(0x1B4) == 1) {
            self->state++;
            em_mot_set(self, 2, 0xA, 0);
        }
        break;
    case 3:
        fn_8014610C(self, lbl_807993E8, lbl_807993EC, lbl_807993F0);
        fn_80133C3C(self);
        if (em_frame_check(self, 1, lbl_80799254, lbl_80799220) == 1) {
            self->state++;
            em_mot_set(self, 8, 8, 0);
            em_mot_speed_set(self, lbl_80799368);
        }
        break;
    case 4:
        fn_8014610C(self, lbl_807993E8, lbl_807993EC, lbl_807993F0);
        fn_80133C3C(self);
        if (em_frame_check(self, 0, lbl_80799274, lbl_80799220) == 1) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079923C);
            fn_801049D0(self, 1, 5, 0, &spot, lbl_807993A0);
        }
        if (fn_80146008(0x294) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xDA, 0, 0x56);
            em_mot_speed_set(self, lbl_80799280);
            fn_8014610C(self, lbl_80799220, lbl_80799400, lbl_80799220);
            fn_801462A4(self, fn_80145FE4(), lbl_805B61F8, lbl_805B63A8, 7, 2);
            fn_801303EC(self, self->pos.y - self->field_0x20C);
        }
        break;
    case 5:
        fn_801462A4(self, fn_80145FE4(), lbl_805B61F8, lbl_805B63A8, 7, 2);
        fn_801303EC(self, self->pos.y - self->field_0x20C);
        if (fn_80146008(0x2AE) == 1) {
            self->state++;
            em_mot_set(self, 0xDB, 0, 0);
        }
        break;
    case 6:
        fn_801462A4(self, fn_80145FE4(), lbl_805B61F8, lbl_805B63A8, 7, 2);
        fn_801303EC(self, self->pos.y - self->field_0x20C);
        if (em_frame_check(self, 0, lbl_80799404, lbl_80799220) == 1) {
            get_joint_wpos_em(self, 3, &spot);
            spot.y = lbl_807992E8 + self->pos.y;
            fn_8010D2B0(&spot, self->area_no, 5, 0, lbl_80799408);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 2, 4, 0);
            em_mot_speed_set(self, lbl_80799288);
        }
        break;
    case 7:
        fn_801462A4(self, fn_80145FE4(), lbl_805B61F8, lbl_805B63A8, 7, 2);
        fn_801303EC(self, self->pos.y - self->field_0x20C);
        if (em_frame_check(self, 1, lbl_80799354, lbl_80799220) == 1) {
            self->state++;
            em_mot_speed_set(self, lbl_80799240);
        }
        break;
    case 8:
        fn_801462A4(self, fn_80145FE4(), lbl_805B61F8, lbl_805B63A8, 7, 2);
        fn_801303EC(self, self->pos.y - self->field_0x20C);
        if (fn_80146008(0x3FC) == 1) {
            self->state++;
            em_mot_set(self, 2, 0, 0x110);
            fn_8014610C(self, lbl_80799220, lbl_8079940C, lbl_80799220);
            fn_80146058(self, lbl_80799410, lbl_80799220, lbl_80799414);
            fn_801303EC(self, lbl_80799220);
        }
        break;
    case 9:
        if (em_frame_check(self, 1, lbl_80799418, lbl_80799220) == 1) {
            self->state++;
            em_mot_set(self, 0xDF, 0xA, 0);
        }
        break;
    case 10:
        if (fn_80146008(0x42A) == 1) {
            fn_8014610C(self, lbl_80799220, lbl_8079941C, lbl_80799220);
            fn_801461A8(self, fn_80145FE4(), lbl_805B63E8, 0);
            fn_801303EC(self, self->pos.y - self->field_0x20C);
        } else if (fn_8012F948(self) == 0) {
            self->field_0x318 = fn_80135644(self, lbl_805B52D8);
            fn_801354F4(self, &self->field_0x1BC);
        }
        if (fn_80146008(0x454) == 1) {
            self->state++;
            em_mot_set(self, 0xDF, 0, 0);
            em_mot_speed_set(self, lbl_80799420);
        }
        break;
    case 11:
        fn_8014610C(self, lbl_80799220, lbl_8079941C, lbl_80799220);
        fn_801461A8(self, fn_80145FE4(), lbl_805B63E8, 0);
        fn_801303EC(self, self->pos.y - self->field_0x20C);
        if (em_frame_check(self, 1, lbl_80799424, lbl_80799220) == 1) {
            self->state++;
            em_mot_speed_set(self, lbl_80799240);
        }
        break;
    case 12:
        fn_8014610C(self, lbl_80799220, lbl_8079941C, lbl_80799220);
        fn_801461A8(self, fn_80145FE4(), lbl_805B63E8, 0);
        fn_801303EC(self, self->pos.y - self->field_0x20C);
        if (fn_80146008(0x49C) == 1) {
            self->state++;
            fn_8014610C(self, lbl_80799220, lbl_80799428, lbl_80799220);
            fn_80146058(self, lbl_8079942C, lbl_80799220, lbl_80799430);
        }
        break;
    case 13:
        if (fn_80146008(0x5AA) == 1) {
            self->state++;
            em_mot_set(self, 0x12, 0, 0);
            em_mot_speed_set(self, lbl_80799434);
        }
        break;
    case 14:
        if (fn_80146008(0x6E4) == 1) {
            self->state++;
            em_mot_set(self, 5, 0, 0);
            fn_8014610C(self, lbl_80799220, lbl_80799428, lbl_80799220);
            fn_80146058(self, lbl_80799438, lbl_80799220, lbl_8079943C);
            self->timer_0x020 = 0;
        }
        break;
    case 15:
        if (em_frame_check(self, 0, lbl_80799238, lbl_80799220) == 1) {
            fn_80056A54(self, 0x1D, 0xA);
        }
        if (em_frame_check(self, 0, lbl_80799238, lbl_80799220) == 1) {
            setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079923C);
            fn_80304508(self, 0, 0x1B, &spot, lbl_80799240);
        }
        if (em_frame_check(self, 3, lbl_80799244, lbl_80799248) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                setVector3(&spot, lbl_80799220, lbl_80799220, lbl_8079923C);
                fn_80304508(self, 1, 0x1B, &spot, lbl_80799240);
            }
            self->timer_0x020++;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 2, 4, 0);
            em_mot_speed_set(self, lbl_80799440);
        }
        break;
    }
}

void fn_801D4B84(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0, 0);
        fn_8014610C(self, lbl_80799220, lbl_80799428, lbl_80799220);
        fn_80146058(self, lbl_80799438, lbl_80799220, lbl_8079943C);
        fn_801303EC(self, lbl_80799220);
        fn_8014619C(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D4C3C(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801D2ED0(self);
        break;
    case 1:
        fn_801D320C(self);
        break;
    case 2:
        fn_801D3564(self);
        break;
    case 3:
        fn_801D38A0(self);
        break;
    case 4:
        fn_801D3C38(self);
        break;
    case 5:
        fn_801D3CF8(self, 0);
        break;
    case 6:
        fn_801D3CF8(self, 1);
        break;
    case 7:
        fn_801D428C(self);
        break;
    case 8:
        fn_801D4B84(self);
        break;
    }
}

void fn_801D4C90(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_801CB308(self);
        break;
    case 1:
        fn_801CB9DC(self);
        break;
    case 2:
        fn_801CCCE8(self);
        break;
    case 3:
        fn_801CE898(self);
        break;
    case 4:
        fn_801CF648(self);
        break;
    case 5:
        fn_801D290C(self);
        break;
    case 6:
        fn_801D2B10(self);
        break;
    case 7:
        fn_801D2E0C(self);
        break;
    case 8:
        fn_801D2EBC(self);
        break;
    case 9:
        fn_801D4C3C(self);
        break;
    }
}

void fn_801D4CE0(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    u16 ang;

    VEC3_ctor(&pos);
    if (fn_8012EC60(self) != 0) {
        ang = calcVecAngX(&self->vec_0x76C);
        if ((u16)(ang + 0x8000) > 0x671B) {
            u16 mot = em_get_mot_no(self);
            if (mot != 0xD8 && mot != 0xE4) {
                setVector3(&pos, lbl_80799220, lbl_807992BC, lbl_80799380);
                if ((system_w.field_0x0c & 0x1F) == 0) {
                    if ((u16)(ang + 0x8000) > 0x6E38) {
                        fn_8010562C(self, 0x16, 0x1C, &pos, lbl_80799240);
                    } else {
                        fn_8010562C(self, 0x15, 0x1C, &pos, lbl_80799240);
                    }
                }
            }
        }
    }
}

void fn_801D4DD8(struct _ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 arg5) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (arg1) {
    case 0:
        if ((u32)(arg2 - 0xC) > 0xD) {
            if (arg2 != 0x26) {
                if (arg3 == 0xFF) {
                    pos.x = self->pos.x;
                    pos.y = lbl_807992E8 + self->field_0x20C;
                    pos.z = self->pos.z;
                    eft009_set_pos(arg2, &pos, &self->field_0x1BC, arg5, self->area_no);
                } else {
                    fn_801048B4(self, arg3, arg4, arg5);
                }
            }
        }
        break;
    case 1:
        if (arg3 == 0xFF) {
            copyVec3(&pos, &self->pos);
        } else {
            get_joint_wpos_em(self, arg3, &pos);
        }
        pos.y = self->field_0x20C;
        fn_8010D2B0(&pos, self->area_no, arg2, self->field_0x1C0,
                    arg5 * get_em_chg_scale(self));
        break;
    case 2:
        if (arg3 == 0xFF) {
            copyVec3(&pos, &self->pos);
        } else {
            get_joint_wpos_em(self, arg3, &pos);
        }
        pos.y = self->field_0x20C;
        fn_80106694(self, &pos, arg2, arg5 * get_em_chg_scale(self));
        break;
    }
}

void fn_801D64A4(struct _ENEMY_WORK* self) {
    _GXColor color;

    ((MHchar*)self->char_0x024)->getTevKColor(5, GX_KCOLOR3, &color);
    if (fn_8012EC60(self) == 1) {
        if (color.a < 251) {
            color.a = color.a + 4;
        } else {
            color.a = 255;
        }
    } else {
        if (color.a > 4) {
            color.a = color.a - 4;
        } else {
            color.a = 0;
        }
    }
    ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR3, color);
}

s32 fn_801D6548(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 c;
    nw4r::math::VEC3 d;
    struct _ENEMY_WORK* other;
    f32 dist;
    f32 scale;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    other = fn_80131034(self, 0x1C, 0);
    if (other != NULL && fn_8012E5A8(other) == 1) {
        if (arg1 == 0) {
            return 1;
        }
        copyVec3(&b, setVec3(&d, lbl_80799220, lbl_80799220,
                                    lbl_80799264 * get_em_chg_scale(self)));
        rotVecY(&b, self->field_0x1C0);
        fn_80051378(&c, &self->pos, &b);
        copyVec3(&a, &c);
        dist = calcDistanceSqXZ(&a, &other->pos);
        scale = lbl_8079948C * get_em_chg_scale(self);
        if (dist < lbl_8079948C * get_em_chg_scale(self) * scale) {
            return 1;
        }
    }
    return 0;
}

s32 fn_801D6694(struct _ENEMY_WORK* self) {
    if (self->action == 13 && self->state_sub <= 4) {
        return 1;
    }
    return 0;
}

s32 fn_801D66BC(u8 arg0, void* target) {
    EmScratchA scratchA;
    EmScratchB scratchB;
    EmGrowTable801D428C* entry;

    fn_800FA3B8(&scratchA);
    fn_800FA378(&scratchB);
    if (arg0 >= 3) {
        return 0;
    }
    entry = (EmGrowTable801D428C*)lbl_805B6950 + arg0;
    fn_8004FFC8(entry->rec_a, entry->rec_b, &scratchA, entry->scale);
    fn_8028F558(&scratchA, &scratchB);
    return fn_802907BC((s32)target, &scratchB) - 1 == 0;
}

s32 fn_801D6758(struct _ENEMY_WORK* self) {
    EmAreaEntry801D428C* entry;

    if (self->field_0x382 != 0xFF && self->field_0x380 == 1) {
        entry = (EmAreaEntry801D428C*)fn_801377D0(self->state_0x381);
        if (entry->active != 0 && entry->area_no == self->area_no &&
            (entry->flags_0x5A6 & 0x7F) == 1) {
            return 1;
        }
    }
    return 0;
}

u8 fn_801D67D8(struct _ENEMY_WORK* self, u8 arg1, u8 arg2, nw4r::math::VEC3* target) {
    nw4r::math::VEC3 pos;
    f32 dist;
    u8 best;
    u8 id;
    s32 i;

    VEC3_ctor(&pos);
    i = 0;
    id = arg1;
    best = arg2;
    for (i = 0; i < (s32)arg2; i++) {
        fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | id), &pos);
        if (i == 0) {
            dist = calcDistanceSqXZ(target, &pos);
            best = (u8)i;
        } else {
            f32 d = calcDistanceSqXZ(target, &pos);
            if (dist > d) {
                dist = d;
                best = (u8)i;
            }
        }
        id++;
    }
    return best;
}

u32 fn_801D68A8(struct _ENEMY_WORK* self, u32 arg1) {
    nw4r::math::VEC3 pos;
    u32 t;
    u8 sel;

    VEC3_ctor(&pos);
    sel = (u8)arg1;
    switch (sel) {
    case 0:
        if ((u8)fn_802B0668(self->field_0x1E0) == 4) {
            switch (self->area_no) {
            case 2:
                setVector3(&pos, lbl_80799490, lbl_8079930C, lbl_80799494);
                if (calcDistanceSqXZ(&self->pos, &pos) <= lbl_80799498) {
                    return 0;
                }
                break;
            case 3:
                setVector3(&pos, lbl_8079949C, lbl_8079931C, lbl_807994A0);
                if (calcDistanceSqXZ(&self->pos, &pos) <= lbl_80799498) {
                    return 0;
                }
                break;
            case 7:
                setVector3(&pos, lbl_807994A4, lbl_80799220, lbl_807994A8);
                if (calcDistanceSqXZ(&self->pos, &pos) <= lbl_80799498) {
                    return 0;
                }
                setVector3(&pos, lbl_807994AC, lbl_807994B0, lbl_807994B4);
                if (calcDistanceSqXZ(&self->pos, &pos) <= lbl_80799498) {
                    return 1;
                }
                break;
            }
        }
        return 0xFF;
    case 1:
        return fn_80131034(self, 0x1C, 1) != NULL;
    case 2:
        return self->action_0x328.armed_0x328.field_0x330;
    case 3:
        if (em_parts_damage_level_get(self, 2) >= 1) {
            return 1;
        }
        break;
    case 4:
        if (em_parts_damage_level_get(self, 3) >= 1) {
            return 1;
        }
        break;
    case 5:
        if (self->field_0x010 == 2) {
            sel = fn_801D67D8(self, 0x11, 3, &self->pos);
        } else {
            sel = fn_801D67D8(self, 0xC, 3, &self->pos);
        }
        switch (sel) {
        case 0:
            if (fn_801D66BC(0, &self->vec_0x36C) == 1 ||
                fn_801D66BC(1, &self->vec_0x36C) == 1 ||
                fn_801D66BC(2, &self->vec_0x36C) == 1) {
                return 0;
            }
            break;
        case 1:
            if (fn_801D66BC(0, &self->vec_0x36C) == 1 ||
                fn_801D66BC(1, &self->vec_0x36C) == 1 ||
                fn_801D66BC(2, &self->vec_0x36C) == 1) {
                return 0;
            }
            break;
        case 2:
            if (fn_801D66BC(0, &self->vec_0x36C) == 1 ||
                fn_801D66BC(1, &self->vec_0x36C) == 1 ||
                fn_801D66BC(2, &self->vec_0x36C) == 1) {
                return 0;
            }
            break;
        }
        if (fn_801D6758(self) == 1) {
            return 0;
        }
        if (self->field_0x010 == 2) {
            return (u8)(fn_801D67D8(self, 9, 3, &self->vec_0x36C) + 1);
        }
        return (u8)(fn_801D67D8(self, 6, 3, &self->vec_0x36C) + 1);
    case 6:
        t = fn_801D6758(self);
        return (u32)((1 - t) | (t - 1)) >> 31;
    }
    return 0;
}

void fn_801D6C50(struct _ENEMY_WORK* self, u8* out1, u8* out2) {
    *out1 = 12;
    *out2 = 0;
    self->pos.y = self->pos.y + lbl_80799224;
}

s32 fn_801D6C74(struct _ENEMY_WORK* self, u8 arg1) {
    if (arg1 == 0 && fn_8012EC60(self) == 1 && self->field_0x1E2 == 0) {
        return 1;
    }
    return 0;
}

s32 fn_801D6CCC(struct _ENEMY_WORK* self, u8 arg1) {
    if (arg1 == 0 && fn_8012EC3C(self) == 1 && self->field_0x1E2 == 0) {
        return 1;
    }
    return 0;
}

void fn_801D6D24(struct _ENEMY_WORK* self) {
    if (self->action_0x328.armed_0x328.field_0x330 == 0xFF) {
        if ((u8)fn_802B0668(self->field_0x1E0) == 4) {
            if (self->area_no == 5 && self->field_0x9F6 == 7) {
                self->action_0x328.armed_0x328.field_0x330 = 1;
            } else {
                self->action_0x328.armed_0x328.field_0x330 = 0;
            }
        } else {
            self->action_0x328.armed_0x328.field_0x330 = 0xFF;
        }
    }
}

s32 fn_801D6DA4(struct _ENEMY_WORK* self, u16 arg1) {
    u8 kind = (u8)fn_802B0668(self->field_0x1E0);
    s32 found;
    u8 sel;
    u8 sub;

    if (kind != 4) {
        return 0;
    }
    found = 0;
    sel = 0xFF;
    if (kind == 4) {
        sel = 7;
    }
    if (sel != 0xFF) {
        sub = fn_80129DB8(self);
        if (sub == 1) {
            found = 1;
        } else if (sub == 2) {
            return 1;
        }
    }
    if (found == 0 && self->value_0x452 >= 0x384) {
        u8 arg = 0xFF;
        if (kind == 4) {
            arg = 5;
        }
        if (fn_8012A014(self, 0x1C, arg, arg1, lbl_805B5310, lbl_805B531C) == 1) {
            return 1;
        }
    }
    if (fn_80129A70(self, arg1) == 1) {
        return 1;
    }
    return fn_8012A204(self) - 1 == 0;
}

void fn_801D6EDC(EmSpawnRec801D428C* rec, u8 a1, u16 a2, u16 a3) {
    nw4r::math::VEC3 pos;

    setVec3(&pos, lbl_80799220, lbl_807992BC, lbl_80799380);
    rec->id = 29;
    copyVec3(&rec->pos, &pos);
    rec->field_0x10 = a1;
    rec->field_0x12 = a2;
    rec->field_0x14 = a3;
}

void* fn_801D6F5C(void* self, s16 arg1) {
    if (self != NULL) {
        fn_8013918C(self, 0);
        if (arg1 > 0) {
            operator delete(self);
        }
    }
    return self;
}

void fn_801D6FB8(void) {
    nw4r::math::VEC3 v0;
    nw4r::math::VEC3 v1;
    nw4r::math::VEC3 v2;
    nw4r::math::VEC3 v3;
    nw4r::math::VEC3 v4;
    nw4r::math::VEC3 v5;
    nw4r::math::VEC3 v6;
    nw4r::math::VEC3 v7;
    nw4r::math::VEC3 v8;
    nw4r::math::VEC3 v9;
    nw4r::math::VEC3 v10;
    nw4r::math::VEC3 v11;
    nw4r::math::VEC3 v12;
    nw4r::math::VEC3 v13;

    fn_80051490(lbl_806A7B30, setVec3(&v0, lbl_80799220, lbl_807992B0, lbl_80799220));
    fn_80051490(lbl_806A7B30 + 0xC, setVec3(&v1, lbl_80799220, lbl_807994B8, lbl_80799220));
    fn_80051490(lbl_806A7B48, setVec3(&v2, lbl_80799220, lbl_807992B0, lbl_80799220));
    fn_80051490(lbl_806A7B48 + 0xC, setVec3(&v3, lbl_80799220, lbl_807994B8, lbl_80799220));
    fn_80051490(lbl_806A7B60, setVec3(&v4, lbl_80799220, lbl_807992F4, lbl_80799220));
    fn_80051490(lbl_806A7B60 + 0xC, setVec3(&v5, lbl_80799220, lbl_807994BC, lbl_80799220));
    fn_80051490(lbl_806A7B78, setVec3(&v6, lbl_80799220, lbl_807992B0, lbl_80799220));
    fn_80051490(lbl_806A7B78 + 0xC, setVec3(&v7, lbl_80799220, lbl_807994B8, lbl_80799220));
    fn_80051490(lbl_805B6950, setVec3(&v8, lbl_807994C0, lbl_80799220, lbl_807994C4));
    fn_80051490(lbl_805B6950 + 0xC, setVec3(&v9, lbl_807994C8, lbl_807994CC, lbl_807994D0));
    fn_80051490(lbl_805B6950 + 0x1C, setVec3(&v10, lbl_807994D4, lbl_80799220, lbl_807994D8));
    fn_80051490(lbl_805B6950 + 0x28, setVec3(&v11, lbl_807994DC, lbl_80799220, lbl_807994E0));
    fn_80051490(lbl_805B6950 + 0x38, setVec3(&v12, lbl_807994E4, lbl_80799220, lbl_807994E8));
    fn_80051490(lbl_805B6950 + 0x44, setVec3(&v13, lbl_807994EC, lbl_80799220, lbl_807994F0));
}

void fn_801D71C4(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 pos;
    EmHelper801D428C* helper;

    VEC3_ctor(&pos);
    setVector3(&self->action_0x328.vec_0x328, lbl_807994F8, lbl_807994F8, lbl_807994F8);
    setVector3(&self->action_0x328.vec_0x334, lbl_807994F8, lbl_807994F8, lbl_807994F8);
    self->action_0x328.field_0x340 = lbl_807994FC;
    self->action_0x328.field_0x344 = 0;
    self->action_0x328.field_0x345 = 0;
    if (fn_801391E8(self) == 0) {
        helper = (EmHelper801D428C*)operator new(0xC);
        if (helper != NULL) {
            fn_801D752C(helper);
        }
        fn_801390FC(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&pos, lbl_807994FC, lbl_807994FC, lbl_80799500);
        fn_801057FC(self, 0x14, 0x18, &pos, lbl_807994F8, 0);
    }
    if (arg1 == 2) {
        switch ((u8)fn_802B0668(self->field_0x1E0)) {
        case 1:
            switch (self->area_no) {
            case 4:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 7), &self->pos);
                break;
            case 5:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 5), &self->pos);
                break;
            }
            break;
        case 2:
            switch (self->area_no) {
            case 4:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 6), &self->pos);
                break;
            case 6:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 7), &self->pos);
                break;
            }
            break;
        case 3:
            switch (self->area_no) {
            case 1:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 1), &self->pos);
                break;
            case 3:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 4), &self->pos);
                break;
            }
            break;
        case 4:
            switch (self->area_no) {
            case 3:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 7), &self->pos);
                break;
            case 6:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 5), &self->pos);
                break;
            }
            break;
        case 5:
            switch (self->area_no) {
            case 4:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 1), &self->pos);
                break;
            case 6:
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8 | 4), &self->pos);
                break;
            }
            break;
        case 8:
            if (self->area_no == 1) {
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
            }
            break;
        case 9:
            if (self->area_no == 0) {
                fn_80126278(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
            }
            break;
        }
        em_move_mode_set(self, 4);
        fn_80128A8C(self, 6, 5);
    }
}

void* fn_801D752C(EmHelper801D428C* self) {
    fn_80147E2C(self);
    self->vtbl = lbl_805B75B8;
    return self;
}

void fn_801D7568(void) {
}

void fn_801D756C(struct _ENEMY_WORK* self, u8 arg1, u8 arg2) {
    switch (arg1) {
    case 1:
        switch (arg2) {
        case 0:
            fn_801376B4(self);
            break;
        case 5:
        case 9:
        case 12:
            fn_80130F74(self);
            break;
        }
        break;
    case 10:
        if (arg2 == 201) {
            fn_80135C5C(self, 0, 0);
        }
        break;
    }
}

void fn_801D75D0(struct _ENEMY_WORK* self) {
    u16 max;
    struct _ENEMY_WORK* other;
    u16 i;
    s32 found;

    max = get_move_work_max(3);
    other = (struct _ENEMY_WORK*)get_move_work_adrs(3);
    found = 0;
    if (em_die_ck(self) == 0) {
        for (i = 0; i < max; i++) {
            if (other->active != 0 && (other->field_0x1C8 & 1) != 0 && other != self &&
                self->area_no == other->area_no &&
                fn_8013023C(other) == fn_8013023C(self) && other->action == 0xB) {
                copyVec3(&self->aim, &other->pos);
                self->action_0x328.field_0x345 = 1;
                found = 1;
                break;
            }
            other++;
        }
    }
    if (found == 0) {
        self->action_0x328.field_0x345 = 0;
    }
    if (fn_8012EC60(self) == 1) {
        if ((self->flags_0x836 & 0x8000) != 0) {
            self->flags_0x836 &= 0x7FFF;
        }
    } else {
        if ((self->flags_0x836 & 0x8000) == 0) {
            self->flags_0x836 |= 0x8000;
        }
    }
}

void fn_801D771C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7798(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7814(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xE, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7890(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        fn_8013032C(self);
        fn_801303EC(self, fn_8013032C(self));
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

void fn_801D791C(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801D771C(self);
        break;
    case 1:
        fn_801D7798(self);
        break;
    case 2:
        fn_801D7814(self);
        break;
    case 3:
        fn_801D771C(self);
        break;
    case 4:
        fn_801D771C(self);
        break;
    case 6:
        fn_801D771C(self);
        break;
    case 7:
        fn_801D7890(self);
        break;
    }
}

void fn_801D797C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D79F8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 9, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7A74(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80799504, lbl_807994FC) == 1) {
            fn_8012933C(self, 0, 0xB, 5);
            fn_80056A54(self, 0x17, 0xA);
            fn_802B43A8(&self->pos, self->area_no, self->bits_0x1EC);
        }
        if (em_frame_check(self, 0, lbl_80799504, lbl_807994FC) == 1) {
            setVector3(&pos, lbl_807994FC, lbl_80799508, lbl_8079950C);
            fn_80304508(self, 0, 0x16, &pos, lbl_807994F8);
        }
        if (em_frame_check(self, 3, lbl_80799510, lbl_80799514) == 1 &&
            (system_w.field_0x0c & 7) == 0) {
            setVector3(&pos, lbl_807994FC, lbl_80799508, lbl_8079950C);
            fn_80304508(self, 1, 0x16, &pos, lbl_807994F8);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7BF8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x28, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x6E, 4, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2:
        fn_8013221C(self, lbl_80799518, 1, 5);
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x70, 4, 0);
            fn_80132264(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7D14(struct _ENEMY_WORK* self, u32 arg1) {
    if ((u8)arg1 == 0) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_8079951C, lbl_807994FC) == 1) {
            fn_80129668(self, 0, 0x3C);
        }
        if (em_mot_end_ck(self) == 1) {
            if (arg1 != 1) {
                if (arg1 != 2) {
                    em_state_set(self, 1, 5);
                } else {
                    em_state_set(self, 1, 0xC);
                }
            } else {
                self->state_0x006 = self->state_0x006 + 1;
                if (self->state_0x006 >= 4) {
                    em_state_set(self, 1, 9);
                }
            }
        }
        break;
    }
}

void fn_801D7E44(struct _ENEMY_WORK* self, u32 arg1) {
    if ((u8)arg1 == 0) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        fn_8012F504(self, 0xC9, 6, 0, 1);
        fn_80129668(self, 0, 0x3D);
        if (arg1 == 1) {
            fn_80130CDC(self, 0x3E8);
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7F04(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x36, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7F80(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x22, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D7FFC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x20, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801D8078(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801D797C(self);
        break;
    case 1:
        fn_801D79F8(self);
        break;
    case 2:
        fn_801D7A74(self);
        break;
    case 3:
        fn_801D7BF8(self);
        break;
    case 4:
        fn_801D7D14(self, 0);
        break;
    case 5:
        fn_801D7E44(self, 0);
        break;
    case 6:
        fn_801D7F04(self);
        break;
    case 7:
        fn_801D7F80(self);
        break;
    case 8:
        fn_801D7D14(self, 1);
        break;
    case 9:
        fn_801D7E44(self, 1);
        break;
    case 10:
        fn_801D7FFC(self);
        break;
    case 11:
        fn_801D7D14(self, 2);
        break;
    case 12:
        fn_801D7E44(self, 2);
        break;
    }
}

#ifdef __cplusplus
}
#endif
