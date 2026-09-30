/*
 * Declarations owned by `sound/fn_800D7F54.cpp` (docs/plan.md 6.5 rule 2).  A consumer includes this
 * header instead of declaring the symbol itself.  Keep it minimal.
 */
#ifndef MHTRI_SOUND_FN_800D7F54_H
#define MHTRI_SOUND_FN_800D7F54_H

#include "types.h"

struct SeSlot;
struct _se_w;

#ifdef __cplusplus
extern "C" {
#endif

/* The sound-module frame entry point `main.cpp` calls. */
void sound_frame_entry(void);


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
/* Reconcile 2026-09-25: the owner (`sound/fn_800D7F54.cpp:134`) declares this
 * `SeSlot* fn_800DA72C(s32, s32, nw4r::math::VEC3*)`, so the `void*` here was a boundary artefact.
 * `sound/se.h` carries the owner's spelling - the two now agree and a consumer that includes both
 * no longer trips `(10505) illegal overloading` (no consumer reads the result). */
struct SeSlot* fn_800DA72C(s32 kind, s32 id, Vec3* pos);
void fn_800DCF0C(s32 handle, Vec3* pos);
void fn_800DA864(Vec3* pos);
void fn_800DA8F4(Vec3* pos);
void fn_800DA93C(Vec3* pos);
void fn_800DA95C(Vec3* pos);
void fn_800DCB18(s32 id, Vec3* pos);
void fn_800DB608(u8 flag, Vec3* pos, u8 arg);
/* Merged 2026-09-25 (ef/fn_80105314 batch): the two per-position sound requests the enemy-effect
 * dispatch (`fn_80105564`) tail-calls. */
void fn_800DC60C(nw4r::math::VEC3* pos, u32 mode);
void fn_800DB964(nw4r::math::VEC3* pos);
/* Merged 2026-09-25 (ef/fn_80105314 batch): the per-position sound requests the enemy-effect
 * state-0 handlers call. */
void fn_800DC6D8(nw4r::math::VEC3* pos, u32 mode);
void fn_800DB974(void* src, nw4r::math::VEC3* pos);

/* Merged 2026-09-24: the per-material impact calls `ef/eft019.cpp`'s creation tail dispatches into. */
void fn_800DA8AC(Vec3* pos);
void fn_800DA9A4(Vec3* pos);
void fn_800DA9B4(Vec3* pos);
void fn_800DA9C4(Vec3* pos);
void fn_800DA9D4(Vec3* pos);
void fn_800DA9E4(Vec3* pos);
void fn_800DA9F4(Vec3* pos);
void fn_800DC46C(Vec3* pos);
void fn_800DC4B4(Vec3* pos);

/* 0x800DA428 - the player's frame-set request; the owner defines it `extern "C"`.  Added with
 * `Pl/fn_80229ECC.cpp`, which calls it once per motion. */
void fn_800DA428(struct _se_w* work, s32 a, u32 param, s32 d, s32 e);

/* 0x800DCC24 - the SE request layer's entry the AI band arms with `(work, kind, flag)`; the owner
 * defines it `extern "C" void fn_800DCC24(_se_w*, s32, u8)` (`fn_800D7F54.cpp:1798`).  Added with
 * `ai/fn_802C474C.cpp` (docs/plan.md 6.5 rule 2). */
void fn_800DCC24(struct _se_w* work, s32 kind, u8 c);
/* 0x800DBDD4 - the fixed-SE request the quest-result band's swap timer arms (`li r3,0x18` then
 * `fn_800F04FC`/`fn_800DBB78(0x18,0)`), so it takes no argument and is declared at C linkage like
 * its `fn_` siblings here (the map name is the plain `fn_800DBDD4`).  Added with
 * `menu/menu_result.cpp` (rule 2: this TU owns the address). */
void fn_800DBDD4(void);

/* 0x800DBC84 - stops sound effect `id`. */
void sysSE_stop(u32 id);

/* 0x800DACA8 - plays the "item could not be used" sound for the local hunter (the `fn_800F0C14` voice of its
 * move work's +0x13C record).  GUESS name from its two callers, which raise the "cannot use" message first. */
void snd_item_fail_play(void);

#ifdef __cplusplus
}

/* 0x800DA154 / the sound-code generator: the two entry points `Pl/fn_80229ECC.cpp` drives.  The map
 * spells them `se_req_frame_set__FP5_se_wllll` / `SE_Code_Make__Flsls`, so they are C++ free
 * functions and their declarations sit at C++ scope where the front-end reproduces the mangling
 * (docs/plan.md 6.5 rule 9).  The owner defines them without `extern "C"`. */
struct _se_w;
void se_req_frame_set(_se_w* work, s32 a, s32 param, s32 d, s32 e);
/* The per-frame-set helper `Pl/fn_80241558.cpp` calls: an unmangled `fn_` name, so C linkage. */

/* The SE request layer's own workhorse and code builder, defined by `sound/fn_800D7F54.cpp`.  The map
 * spells them mangled (`se_req_frame_set__FP5_se_wllll`, `SE_Code_Make__Flsls`), so a consumer calls the
 * real C++ declaration and the front-end mangles it back (docs/plan.md 6.5 rule 9).  Merged 2026-09-25
 * for `Pl/fn_80241558.cpp`, whose whole body is 139 `se_req_frame_set` arming calls. */
void se_req_frame_set(struct _se_w* work, s32 a, s32 param, s32 d, s32 e);
s32 SE_Code_Make(s32 low_code, s16 low, s32 mid_code, s16 span);
/* 0x800DB208 - the position-seated shell-SE request (`shell_se_req__FP5_se_wPQ34nw4r4math4VEC3UcUl`),
 * defined without `extern "C"` in `src/sound/fn_800D7F54.cpp`.  Added for
 * `enemy/fn_801BD6C0.cpp` (rule 2). */
void shell_se_req(_se_w* work, nw4r::math::VEC3* pos, u8 id, u32 arg);
/* 0x800DC8C0 - the electric-effect SE request (`em015_denki_eft_se_req__FP5_se_wPQ34nw4r4math4VEC3Uc`),
 * defined without `extern "C"` in `src/sound/fn_800D7F54.cpp:1766`.  Added with `ef/eft035.cpp`
 * (rule 2: this TU owns the address). */
void em015_denki_eft_se_req(_se_w* work, nw4r::math::VEC3* pos, u8 kind);
/* 0x800D9EA8 - the position-seated SE request (`se_req_pos_ps__FP5_se_wllPQ34nw4r4math4VEC3`),
 * defined without `extern "C"` in the same file.  Added with `ef/eft035.cpp`, whose effect setters
 * request the family's SE through it (rule 2: this TU owns the address). */
SeSlot* se_req_pos_ps(_se_w* work, long id, long param, nw4r::math::VEC3* pos);
#endif


/* 0x800DB2DC - the no-argument SE mix update `Pl/fn_80262940.cpp` drives after the action settles; the
 * owner defines it `extern "C" void fn_800DB2DC(void)` (fn_800D7F54.cpp:1328). */
void fn_800DB2DC(void);

/* 0x800DBB78 - the non-positional SE request (`sysSE_req__Fl`); the owner defines it at C++ scope
 * (fn_800D7F54.cpp:144), so it is declared here at C++ scope, outside the `extern "C"` block
 * above.  Added with `menu/menu_item.cpp` (rule 2: this TU owns the address). */
void sysSE_req(s32 id);

/* Added by `enemy/em_action.cpp` (rule 2: the declaration belongs with the owner TU, which
 * had not declared it yet). */
void fn_800DC9A4(struct _se_w* work, VEC3* pos);

#endif /* MHTRI_SOUND_FN_800D7F54_H */
