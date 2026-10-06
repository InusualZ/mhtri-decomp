/*
 * Declarations for the symbols `src/fn_8004CAD8.cpp` owns (docs/plan.md 6.5, rule 2).  The parameters
 * are the owner's `long`, which is the mangling's (`wii_sysmsg_gen__FlPcl`) - `int` would ask for a
 * different symbol.
 */
#ifndef MHTRI_FN_8004CAD8_H
#define MHTRI_FN_8004CAD8_H

#include "types.h"
#include "nw4r/math.h"
#include "fn_8004CAD8/mtx.h" /* MTX34_ctor / set_slot_none - the two this unit's consumers share */
#include "fn_8004CAD8/get_qResult_work.h" /* get_qResult_work (leaf header) */
#include "vec3_scale.h" /* vec3_scale (leaf header) */

#ifdef __cplusplus
void wii_sysmsg_gen(long id, char* buf, long a);

/* 0x8004D140 - the quest-result record accessor this range owns (`get_qResult_work__Fv`, a
 * 12-byte `lis`/`addi`/`blr` over the 0x438 B `qResult` buffer `src/fn_8004CAD8.cpp` defines).
 * Added with `menu/menu_result.cpp`, whose whole band reads the record (rule 2: the declaration
 * belongs with the owner, which had not declared it yet - `src/lobby/fn_801F9CD4.cpp` carried a
 * local copy of this spelling). */
/* (C++ scope: the owner defines it without `extern "C"`, so the map row is the mangling and the
 * declaration reproduces it - `quest/quest_entry.cpp`'s relocations name `get_qResult_work__Fv`.  It is
 * declared in the leaf header `fn_8004CAD8/get_qResult_work.h`, included below the guard.) */

/* The 0x100 B VS user block `get_vsUser_work` indexes (the two slots at 0x8066A620/0x8066A720).
 * The block's own name is the mangling's (`ck_mydata_vs__FP13_vs_user_dataP13_vs_user_data`), so a
 * caller that reaches a mangled consumer spells it `_vs_user_data`.  Only the fields this project's
 * functions touch are named; every other byte is filler.  size: 0x100 */
struct _vs_user_data {
    /* +0x00 */ u8 cfg_0x00[0x18];  /* the per-index option bytes `get_arena_cfg` reads (the
                                     * `0x803BE30C` band's arena twin of `option_w`) */
    /* +0x18 */ s32 point_0x18;      /* the credit `score_add_clamped` accumulates for the player */
    /* +0x1C */ u8 pad_0x1C[0x2C - 0x1C];
    /* +0x2C */ u32 item_0x2C[0x10]; /* the 16 four-byte item slots `item_count_find`/`item_take` walk */
    /* +0x6C */ u32 slot_a_0x6C[10]; /* the first of the two per-slot flag runs `multi_box_phase_ck` counts (ten words: `arena_result_next` walks them all) */
    /* +0x94 */ u32 slot_b_0x94[10]; /* its sibling; the two are read as a pair per slot */
    /* +0xBC */ u16 ready_mask_0xBC; /* the per-player ready bits the phase gate tests against 0x380 */
    /* +0xBE */ u8 pad_0xBE[0xCF - 0xBE];
    /* +0xCF */ u8 player_0xCF;      /* the player's index into `system_w`'s per-player flag runs */
    /* +0xD0 */ u8 pad_0xD0[0x100 - 0xD0];
};

/* 0x8004D14C - one of the two 0x100 B VS user slots, or null when the index is out of range (the
 * owner defines it with the same `long` parameter, which is what its mangling `__Fl` asks for).
 * Added with `menu/multi_result.cpp`, its first consumer (rule 2). */
_vs_user_data* get_vsUser_work(long index);
#endif

/* 0x80050A90 - `calcVecAng2`, the two-vector angle helper: it loads the two vectors' x/z floats and
 * tail-calls 0x80050A40, so the angle comes back in r3.  Added when `enemy/fn_80147CE0.cpp`
 * registered as a consumer (rule 2); the return is the 16-bit angle its three consumers mask
 * (`clrlwi r3,r3,16`) and sign-extend (`extsh`), spelled `s32` here as `enemy/fn_801550FC.cpp` does.
 * C++ linkage (outside the `extern "C"` block below): the map name is the mangling, not a plain
 * name, and `enemy/fn_801550FC.cpp` declares it that way. */
#ifdef __cplusplus
s32 calcVecAng2(VEC3* a, VEC3* b);
/* 0x80050D18 / 0x80051064 - the two nw4r-math helpers this range owns.  C++ linkage (the map names are the
 * manglings of exactly these signatures: `calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl`, `rotVecY__FPQ34nw4r4math4VEC3Ul`),
 * so they are declared here rather than inside the `extern "C"` block - rule 9.  Consumers used to spell them
 * locally in their own sources. */
void calcVecAngXY(nw4r::math::VEC3* v, u32* x, u32* y);
void rotVecY(nw4r::math::VEC3* v, u32 angle);
#endif

/* C linkage: the target symbol is the unmangled `mtx34_const_ptr` (.text 0x80051570, a 4-byte `blr`).
 * Moved out of `src/g3d/g3d_anmchr.cpp` on landing (docs/plan.md 6.5, rule 2): that range was
 * written before this owner registered, so its local `extern "C"` declaration was a boundary
 * artefact. */
#ifdef __cplusplus
extern "C" {
u32 userdata_progress_flag_ck(s32 id);
/* 0x8004D210 - sets progress flag `id` (the halfword bit run at +0x47DA `userdata_progress_flag_ck` reads).  GUESS. */
void userdata_progress_flag_set(u32 id);
/* 0x8004D70C - the event-flag test that shares the same table `userdata_progress_flag_ck` reads; added with
 * `src/lobby/fn_802FA9A0.cpp` as the second consumer (it was declared locally by four
 * `src/lobby/*.cpp` files, which is the rule-2 backlog this declaration closes). */
s32 userdata_flag_ck(s32 id);
/* 0x8004DE7C - the hunter-rank cap for the lobby data block `block` (its rank byte and the user record). */
u16 get_hunter_rank_max(const u8* block);
/* 0x8004D774 / 0x8004D79C - sets / tests the save's event bit `event - 9000` (the halfword at +0x52E2).  GUESS
 * names, from the bodies; added with `menu/menu_result.cpp`, their first consumer (rule 2). */
void userdata_event_bit_set(u16 event);
u32 userdata_event_bit_ck(u16 event);
/* 0x8004E424 - credits `points` hunter points to the save (tail call into 0x8004E1C4 with the save block) and
 * returns its rank state.  GUESS name. */
s32 userdata_hunter_points_add(s32 points);
/* 0x8004E4D4 - folds 41 measured sizes (`sizes`, a stride-4 run) into the save's records: `which` 0 keeps the
 * smallest, 1 the largest.  GUESS name. */
void userdata_size_records_update(const u16* sizes, u8 which);
/* 0x8004E634 - the crown class of a monster size: 0xFF when the monster has no size table, else 0..3.  GUESS
 * name. */
u8 monster_size_crown_get(u8 monster, u16 size);
/* 0x8004E8BC / 0x8004E8E8 - ORs the two unlock words into `system_w`'s announced set / raises the unlock bit
 * an event id maps to in the root move work's +0x148 words.  GUESS names. */
void unlock_seen_mark(const u32* bits);
void unlock_bit_raise(u8 event);
#endif
/* 0x8004D0E8 - clamp `*value += delta` into [0, 9999999], the score/point accumulator the VS result
 * and skill bands credit.  The owner defines it at C linkage, so the declaration sits inside the
 * `extern "C"` region (rule 2: added with `menu/multi_result.cpp`, its second consumer). */
void score_add_clamped(s32 delta, s32* value);
u32 mtx34_const_ptr(u32);
/* 0x8005220C - an 8-byte `fabs f1,f1; blr` helper (caller: `gx/fn_8009ACE4.c`, rule 2: this range
 * owns the address). */
f32 abs_f32(f32 value);
/* The VEC3 helpers this range owns, added when `src/ef/ef_util.cpp` registered as a consumer (rule 2).
 * Signatures are the owners' own bodies, not the call sites' guesses: 0x80050EDC is the squared length
 * (`ps_mul`/`ps_madd`/`ps_sum0` of the vector with itself), 0x80050F24 the length (`PSVECMag`),
 * 0x80051424 the scale-by-scalar (`out = in * s`), 0x80051820 the cross product (returns `out`),
 * 0x80052214 the dot product, 0x80050BC0 the square root (`x * FrSqrt(x)`). */
f32 vec3_length_sq(const f32* v);
f32 vec3_len(const f32* v);
void vec3_scale_by(f32* out, const f32* in, f32 s);
f32* vec3_cross(f32* out, const f32* a, const f32* b);
f32 vec3_dot(const f32* a, const f32* b);
/* 0x80050BC0 - the square root: `x * FrSqrt(x)` for x > 0, 0 for x == 0, and a `nw4r::db::Warning`
 * for x < 0.  ONE float argument, settled from the callee's own body (it reads only f1 and never
 * touches f2), not from the call sites: the `f2` a retail caller materialises before the call is the
 * hoisted common subexpression `1.0f - t` that its `else` branch reuses (ef_disc 0x800CCA7C/0x800CCA98,
 * ef_cylinder 0x800CBD20/0x800CBD3C).  The `(f32, f32)` spelling this symbol used to carry in
 * `ef.h` was the wrong view and made every TU including both headers fail to compile. */
f32 sqrt_f32(f32 x);
/* 0x8005024C - the `SinFIdx` wrapper `enemy/fn_80181E24.cpp`'s alpha computation calls: it narrows
 * its argument to u16 (`clrlwi r3,r3,16`), scales the `anim_tick_angle` result and returns
 * `nw4r::math::SinFIdx`, so the signature is `(u16) -> f32` (settled from the callee's own body,
 * docs/plan.md 6.5 rule 6).  Added with proposal/80181C88. */
f32 fn_8005024C(u16 idx);
/* 0x80050CF4 - the SDK vector subtract (`ps_sub` on two paired loads): `dst = a - b`. */
void PSVECSubtract(f32* dst, const f32* a, const f32* b);
/* 0x800504D4/0x8005050C - the two GX pipe-setup helpers `g3d/g3d_state.cpp` calls (rule 2, moved
 * out of that unit's local extern block on landing). */
void mtx34_identity(void* pOut);
/* 0x80051574 - `dst = dst * src` (the 3x4 matrix product) and 0x80050AF4 - the packed angle word of a
 * direction vector. */
void mtx34_concat_assign(MTX34* dst, const MTX34* src);
s32 vec3_angle_xy(const f32* v);
/* `MTX34_ctor` (0x8005050C) and `set_slot_none` (0x8004CAD8) live in `fn_8004CAD8/mtx.h`, included
 * below - this header's other declarations still disagree with several consumers, so a band that
 * only needs those two takes the light one. */
/* 0x80052BC0/0x800534B0 - the `ResTex`/`ResPltt` value-type constructors the
 * `g3d/g3d_resanmtexsrt.cpp` `ResFile` accessors use (rule 2: declared in their owner's header). */
void* res_tex_ctor(void* out, u32 v);
void* res_pltt_ctor(void* out, u32 v);
/* 0x80052B84/0x80053A90 - assign one `ResTex`/`ResPltt` handle from another (the source is the word a
 * `ResFile` lookup returned); 0x800528DC - whether the texture has a palette (colour-indexed format);
 * 0x80052844 - stores the texture/palette pair in the draw-shape work's slot `index` (the palette handle
 * only when the texture has one); 0x800529A0 - clears the work's texture slots `first`..`last`.  Added
 * with `quest/arenatask.cpp`'s `arena_resource_load` (rule 2: this range owns the addresses). */
u32* res_tex_assign(u32* dst, const u32* src);
u32* res_pltt_assign(u32* dst, const u32* src);
void draw_shape_tex_slot_set(const u32* tex, const u32* pltt, u16 index, s8 flag);
void draw_shape_tex_slots_clear(u32 first, u32 last);
/* 0x80050508 - the 4-byte `blr` twin of fn_8005050C.  Its body does not touch r3, and the retail
 * call sites use the pointer it hands back as the following call's first argument
 * (`GXLoadTexMtxImm(mtx34_get_ptr(&mtx), id, ...)` in `src/g3d/g3d_gpu.cpp`, `GXLoadPosMtxImm(
 * mtx34_get_ptr(&mtx), 0)` in `src/ef/ef_drawlinestrategy.cpp`), so the pointer-returning shape is the
 * one the target's call sites require (added when `src/g3d/g3d_gpu.cpp` registered as the consumer). */
void* mtx34_get_ptr(void* pOut);
/* 0x80050EF4 - the two-pointer distance helper: r3 and r4 are the two `VEC3*` (its body moves r3
 * into r5 and calls `subVec3(&out, r4, r3)`, then `vec3_len(&out)`), so it takes two
 * pointers and returns the float.  Moved here from `enemy/fn_801550FC.cpp` on landing (rule 2):
 * this unit owns the address, and the three-argument form the consumer used was wrong
 * (`enemy/fn_8015941C` sets only r3/r4). */
f32 fn_80050EF4(void* a, void* b);
/* 0x80050CA0 / 0x80050F80 - the vector difference and the distance between two positions, both owned here.
 * Signatures are the CALLEES' OWN BODIES, not the callers' guesses: `subVec3(out, a, b)` is
 * `VEC3_ctor(out); PSVECSubtract(out, a, b)`, and `calcVecDistXZ(a, b)` calls `subVec3(&local, b, a)`
 * then the length helper `vec3_len(&local)`, i.e. `|a - b|`.  Ten consumer files used to declare these
 * locally (four spellings, one of them a `MTX34*` misnomer); they now include this header, so the home is
 * here.  All parameters are pointers - a declaration cannot change a call site's codegen. */
void subVec3(void* out, const void* a, const void* b);
f32 calcVecDistXZ(const void* a, const void* b);
void subVec3(void* out, const void* a, const void* b);
f32 calcVecDistXZ(const void* a, const void* b);
/* 0x80050EAC - the SQUARED distance between two positions (the callers compare it against a squared
 * radius constant, e.g. `enemy/fn_801B0010.cpp` against `lbl_80798B3C` = 2250000.0f = 1500^2). */
f32 vec3_dist_sq(const void* a, const void* b);
/* 0x80051378 - the three-pointer vector helper this range owns (unmangled `addVec3`, so C
 * linkage).  Its body saves r3/r4/r5, zeroes the first through `VEC3_ctor`, then tail-forwards all
 * three to `vec3_add_ps`, i.e. `void (VEC3*, VEC3*, VEC3*)`; added with its first consumer
 * (rule 2) - the owner header did not declare it yet. */
void addVec3(VEC3* out, VEC3* a, VEC3* b);
/* 0x80050850 - the in-place normaliser this range owns (unmangled `vec3_normalize_into`, so C
 * linkage).  Its body saves r3/r4 in r29/r30, hands r3 to `PSVECNormalize`'s `src` (r3) and
 * r4 to its `dst` (the target body reads 0(r3)/8(r3) and stores 0(r4)/8(r4)), then restores
 * `mr r3,r29`, so the shape is `dst = normalize(src); return dst;`: two pointers, the source
 * read-only, the destination returned.  Declared here because this unit owns the address
 * (rule 2).  `void*`/`const void*` is the spelling every call site reaches without a cast
 * (`ef`, `g3d` and `enemy` callers mix `VEC3*` and `void*` pointers) - the body moves
 * pointers and cannot distinguish them. */
void* vec3_normalize_into(void* dst, const void* src);
/* 0x80052370 - the animation key-frame reader the C++ pair below sits beside (plain `fn_` map name,
 * so it stays at C linkage).  Added with `Pl/fn_80224AC4.cpp`. */
f32 fn_80052370(f32* a, f32* b, f32* c, f32 frame);
/* 0x800513CC / 0x80050028 / 0x80051EE0 / 0x800513F0 - the four vector helpers the `Pl` hit tests and
 * the `ef`/`enemy` effect code call (rule 2: this range owns the addresses).  `vec3_add_ps(out, a, b)`
 * is the paired-single add `out = a + b`, `fn_80050028(out, src)` the field-by-field three-float copy,
 * `vec3_scale(out, in, s)` the scale (`VEC3_ctor` then `vec3_scale_by`; in its leaf header), and `fn_800513F0` the
 * in-place scale.  Added with `Pl/fn_8028F66C.cpp`, the first consumer to need them here.  The
 * parameter spellings are the ones the consumers that already include this header declare
 * (`ef/fn_801173AC.cpp`, `enemy/fn_801B7020.cpp`, `enemy/fn_8035E034.cpp`): `fn_800513F0`'s return
 * value is ignored at every call site in the tree, so it is declared `void` - a `VEC3*` return here
 * would be a second overload and `(10505) illegal overloading`. */
void vec3_add_ps(VEC3* out, VEC3* a, VEC3* b);
void fn_80050028(VEC3* out, const VEC3* src);
void fn_800513F0(VEC3* v, f32 scale);
/* 0x80053960 / 0x80054178 - the two draw-shape helpers the cockpit band (`menu/fn_802E4978.cpp`,
 * 0x802E4978-0x802E7408) calls (rule 2: this range owns the addresses; the signatures are that
 * consumer's call sites, neither body being written yet).  0x80053960 sets a four-word colour run on
 * the draw-shape state, 0x80054178 takes the 2D vertex pair it rewrites. */
void fn_80053960(u32, s32, s32, u32);
void drawshape_set_offset_ivec2(s16* pos);
/* 0x800501B4 / 0x800501E4 - the animation clock's cosine and its angle: `t` is a tick count (16-bit wrapped). */
f32 anim_tick_cos(u16 t);
f32 anim_tick_angle(u16 t);

/* One 2D textured rectangle for `draw_rect_2d_tex_by_id` (the map's `_DRAW_RECT_2D_TEX`): the corner, the
 * opposite corner, the colour word and the two texture coordinate pairs.  size: 0x14 */
typedef struct _DRAW_RECT_2D_TEX {
    /* +0x00 */ s16 x;       /* the top-left corner */
    /* +0x02 */ s16 y;
    /* +0x04 */ s16 end_x;   /* the opposite corner (corner + size) */
    /* +0x06 */ s16 end_y;
    /* +0x08 */ u32 color;
    /* +0x0C */ s16 u0;      /* the first texture coordinate pair */
    /* +0x0E */ s16 v0;
    /* +0x10 */ s16 u1;      /* the second pair */
    /* +0x12 */ s16 v1;
} _DRAW_RECT_2D_TEX;

/* The draw-shape band's setters the cockpit quest HUD drives (0x80053xxx-0x80054xxx); names are guesses from
 * the callers (the owner's bodies are not written yet).  The vec2 arguments are two-float pairs. */
void draw_rect_2d_tex_init(_DRAW_RECT_2D_TEX* rect, const s16* pos, u16 width, u16 height, u32 color, const s16* uv0,
                           const s16* uv1); /* 0x8005265C */
void draw_rect_2d_tex_by_id(const _DRAW_RECT_2D_TEX* rect, u16 tex_no); /* 0x80053770 */
void drawshape_set_vertex_array_f32(const f32* verts);                     /* 0x800538CC */
void drawshape_set_color_array(const u32* colors);                         /* 0x80053994 */
void drawshape_copy_vec2(f32* dst, const f32* src);                        /* 0x80053CF8 */
void drawshape_set_texture_array_f32(u16 tex_no, const f32* uvs);          /* 0x80053D0C */
void drawshape_set_tex_scale_uniform(const f32* offset, u16 tex_no, f32 scale); /* 0x80053FC0 */
void drawshape_set_scale_ivec2(const s16* offset, const f32* scale);       /* 0x80053FF4 */
void drawshape_set_offset_uniform(const f32* offset, f32 scale);           /* 0x800541DC */
void drawshape_set_offset_f32(const f32* scale, const f32* offset);        /* 0x8005420C */
void drawshape_set_tex_offset_uniform(const f32* offset, f32 scale);       /* 0x8005442C */
void drawshape_set_tex_offset_f32(const f32* scale, u16 tex_no, const f32* offset); /* 0x8005445C */
/* Added by `enemy/em_action.cpp` (rule 2: the declaration belongs with the owner TU, which
 * had not declared it yet). */
s32 fn_80050C40(void* a, void* b);
void fn_8004FFC8(void* a, void* b, void* c, f32 d);
void fn_800516F0(void* out);
/* 0x8004EAF4 - the 16-byte struct copy MWCC emits for a four-word assignment (`dst[0..3] = src[0..3]`)
 * as four separate word moves.  Added with `menu/menu_row.cpp`, its consumer (rule 2). */
void fn_8004EAF4(void* dst, const void* src);
/* 0x8004EA58 - the 8-row item table's insert: it fills the first empty row with the caller's 16-byte
 * record, and when every row is full shifts the table down one row and writes the record into the
 * last.  Added with `menu/menu_row.cpp`, its consumer (rule 2). */
void fn_8004EA58(const void* entry);

/* 0x8004FC80 - packs item-box page `page` (a 0x2DC-byte record) out of the lobby world block (GUESS name: its caller
 * is the network box-page check request). */
void exportItemBoxPage(u8* record, s32 page);
/* 0x8004FD6C - packs the equipment set (a 0xF0-byte record) out of the lobby world block (GUESS name). */
void exportEquipRecord(u8* record);

/* 0x800526F8 - copies the 4-byte coordinate pair at the head of `src` into `dst`. */
/* untyped: a byte range, the coordinate pair at the head of either a position or a sprite record */
void uv_pair_copy(void* dst, const void* src);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x80050F48 - the squared xz distance between two vectors, added with the same consumer.
 * C++ linkage at global scope (outside the `extern "C"` block above): the map name is the mangling
 * (`calcDistanceSqXZ__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3`). */
f32 calcDistanceSqXZ(VEC3* a, VEC3* b);

/* 0x800514AC - the `out = mtx * v + trans` helper (map name
 * `mulVecMatAddTrans__FPQ34nw4r4math4VEC3PQ34nw4r4math5MTX34`, so C++ linkage at global scope).
 * Added with `enemy/fn_8019ED34.cpp`, its consumer (rule 2/9). */
void mulVecMatAddTrans(VEC3* v, MTX34* m);

/* 0x800500CC / 0x80050364 - build the rotation about X / Z by the angle word `angle` into `m` (map
 * manglings `rotMatrixX__FUlPQ34nw4r4math5MTX34` / `rotMatrixZ__...`). */
void rotMatrixX(u32 angle, MTX34* m);
void rotMatrixZ(u32 angle, MTX34* m);

/* 0x80050990 - build an MTX34 from the Z-X-Y Euler triple `_CP_VECTOR`.  The address sits between the
 * registered `fn_8004C9A0.cpp` and `draw_shape.cpp` units, so no unit owns it yet; the callers are
 * `ef/eft053.cpp` and `Pl/pl_act.cpp` (which declared its own copy before this header carried it). */
struct _CP_VECTOR;
void cpSetRotMatrixZXY(_CP_VECTOR* rot, MTX34* mtx);

/* 0x80050F80 - the squared distance between two 3-float vectors (`fn_8004CAD8.cpp`'s range).  The
 * same signature `ai/fn_802D0F34.h` carries, so a TU including both sees one declaration.  Added
 * with `ef/eft053.cpp`. */
f32 calcVecDistXZ(const void* a, const void* b);

/* 0x80050E70 - copy the engine's `Vec` into an nw4r `VEC3` (map mangling
 * `vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec`).  Added with `ef/eft053.cpp`, whose state machines
 * convert the family's placement-table entries. */
struct Vec;
void vec_to_mh_vec3(VEC3* dst, struct Vec* src);

/* 0x80052300 / 0x80052408 - the animation key-frame readers (map manglings `getKeyData__FPff` /
 * `getKeyData3__FPffPfPfPf`), so C++ linkage at global scope, outside the `extern "C"` block.
 * Added with `Pl/fn_80224AC4.cpp`. */
f32 getKeyData(f32* keys, f32 frame);
void getKeyData3(f32* keys, f32 frame, f32* out0, f32* out1, f32* out2);
#endif

#ifdef __cplusplus
/* 0x8004D17C/0x8004D190 - clear the Fq result work / the quest result work.  C++ free functions, the map's
 * `clear_FqResult_work__Fv` and `clear_qResult_work__Fv`.  Added with `quest/arenatask.cpp` (rule 2). */
void clear_FqResult_work(void);
void clear_qResult_work(void);
#endif

#ifdef __cplusplus
/* The save block's quest-stat record (16 bytes), copied whole by `quest_stat_copy`; `quest/quest_entry.h`
 * names its fields. */
struct Q_QuestStat;
extern "C" {
/* 0x8004E434 / 0x8004E484 - add 41 halfword counts into the save block's record set a (+0x39C0) / b
 * (+0x3AC0), each clamped to 9999.  Names are GUESSes from the destination sets. */
void userdata_record_a_count_add(const u16* counts);
void userdata_record_b_count_add(const u16* counts);
/* 0x8004E700 - stores a quest stat into the save block's own record (+0x3F28). */
void userdata_quest_stat_set(const Q_QuestStat* src);
/* 0x8004E710 - copies one 16-byte quest stat. */
void quest_stat_copy(Q_QuestStat* dst, const Q_QuestStat* src);
/* 0x8004FFA4 - a bare `b strstr`: the first occurrence of `needle` in `haystack`, or NULL (GUESS name; the
 * network message layout tests it against its tag names). */
char* findSubstring(const char* haystack, const char* needle);
}
#endif

#endif /* MHTRI_FN_8004CAD8_H */
