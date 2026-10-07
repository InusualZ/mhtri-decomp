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

/* 0x800DC4C4 / 0x800DC4D4 - the position-seated SE requests of ids 72 and 73 (the eft013 charge start and end). */
void se_req_pos_id72(nw4r::math::VEC3* pos);
void se_req_pos_id73(nw4r::math::VEC3* pos);
/* 0x800DC404 / 0x800DC4F4 - the position-seated SE requests of ids 53 and 74 (each takes the `+100` variant
 * when the position test holds; the eft013 model effects' start sound). */
void se_req_pos_id53(nw4r::math::VEC3* pos);
void se_req_pos_id74(nw4r::math::VEC3* pos);
/* 0x800DB4CC / 0x800DB4DC / 0x800DC3F4 - the position-seated SE requests of ids 228, 250 and 63;
 * 0x800DAA94 / 0x800DB91C / 0x800DCA50 / 0x800DC7F0 - those of ids 54, 84, 83 and 49, each with its `+100` variant
 * when the position test holds (the eft013 effects' start sounds). */
void se_req_pos_id228(nw4r::math::VEC3* pos);
void se_req_pos_id250(nw4r::math::VEC3* pos);
void se_req_pos_id63(nw4r::math::VEC3* pos);
void se_req_pos_id54(nw4r::math::VEC3* pos);
void se_req_pos_id84(nw4r::math::VEC3* pos);
void se_req_pos_id83(nw4r::math::VEC3* pos);
void se_req_pos_id49(nw4r::math::VEC3* pos);
/* 0x800DA428 - the player's frame-set request; the owner defines it `extern "C"`.  Added with
 * `Pl/fn_80229ECC.cpp`, which calls it once per motion. */
void fn_800DA428(struct _se_w* work, s32 a, u32 param, s32 d, s32 e);

/* 0x800DCC24 - the SE request layer's entry the AI band arms with `(work, kind, flag)`; the owner
 * defines it `extern "C" void fn_800DCC24(_se_w*, s32, u8)` (`fn_800D7F54.cpp:1798`).  Added with
 * `ai/fn_802C474C.cpp` (docs/plan.md 6.5 rule 2). */
void fn_800DCC24(struct _se_w* work, s32 kind, u8 c);
/* 0x800DBDD4 - the fixed-SE request the quest-result band's swap timer arms (`li r3,0x18` then
 * `fn_800F04FC`/`fn_800DBB78(0x18,0)`), so it takes no argument and is declared at C linkage like
 * its `fn_` siblings here (the map name is the plain `sysSE_bank24_req`).  Added with
 * `menu/menu_result.cpp` (rule 2: this TU owns the address). */
void sysSE_bank24_req(void);
/* 0x800DBD90 - the bank-0x14 twin: plays `id` of SE bank 0x14 once the bank is loaded.  GUESS name. */
void sysSE_bank20_req(s32 id);

/* 0x800DBC84 - stops sound effect `id`. */
void sysSE_stop(u32 id);
/* 0x800DBACC - points the SE pool's talk voice at `pos` (mode 3/2, kinds 21/16).  GUESS name. */
void se_talk_point_set(nw4r::math::VEC3* pos);

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

/* 0x800DBE50 - the title-bank twin of `sysSE_req` (`titleSE_req__Fl`, C++ scope in the owner, fn_800D7F54.cpp:419).
 * Added with `quest/arenatask.cpp` (rule 2: this TU owns the address). */
void titleSE_req(s32 id);

/* Added by `enemy/em_action.cpp` (rule 2: the declaration belongs with the owner TU, which
 * had not declared it yet). */
void fn_800DC9A4(struct _se_w* work, VEC3* pos);

/* 0x800D9804 - takes a free SE entry for `kind` on `enemy` with `callback` as its sound source and returns it (GUESS
 * name). */
struct SeEntry;
extern "C" struct SeEntry* se_entry_request(s32 kind, struct _ENEMY_WORK* enemy,
                                           void (*callback)(struct _ENEMY_WORK*, s32));

#ifdef __cplusplus
extern "C" {
/* 0x800D7F54 / 0x800D80B8 - set the SE work up and run its frame (GUESS names). */
void se_work_init(void);
void se_frame_step(void);
/* 0x800D8E44 - clears the byte `handle` points at, when there is one (GUESS name). */
void se_handle_clear(u8* handle);
}
#endif

#ifdef __cplusplus
extern "C" {
/* 0x800DBCD8 / 0x800DBE0C - request sound `id` on channel 32 / channel 2 when it is free (GUESS names). */
void sysSE_bank32_req(s32 id);
void se_ch2_req(s32 id);
}
#endif

#endif /* MHTRI_SOUND_FN_800D7F54_H */
