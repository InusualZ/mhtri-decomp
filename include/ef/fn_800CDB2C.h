#ifndef MHTRI_EF_FN_800CDB2C_H
#define MHTRI_EF_FN_800CDB2C_H

#include "types.h"

/* Declarations for the symbols `src/ef/fn_800CDB2C.cpp` owns (docs/plan.md 6.5 rule 2).  `fn_800CF208`
 * is the one the owner defines (`u8 fn_800CF208(void)`); the other two are called by
 * `sound/fn_800EF7D8.cpp` before the owner's body exists, so they carry that call site's view.  The
 * caller had declared all three itself, which only became a rule-2 violation when the owner registered
 * in the same landing wave.
 */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x800CF384 - `system_w` +0x27, the local player's index (a zero-extended byte load), read as such by
 * every consumer: `Pl/pl_master.cpp` compares it with a player's own index byte, `sound/fn_800D7F54.cpp`
 * and `hud/move_work_update.cpp` index the per-player move-work array with it, and `main.cpp` indexes
 * `Screen_w.fa72` with `(s8)` of it.  Added with that unit's registration (rule 2: this range owns the
 * address). */
u32 my_player_no(void);
/* 0x800CF394 - the setter paired with it.  The declaration is `s8`, not the owner's own `u8`: every
 * consumer (`light/light.cpp`'s fn_802BEDE8/fn_802BEE3C) narrows its argument with `extsb` before the
 * call, which is what a signed parameter type emits, and `u8` does not (`light/light.cpp` measured
 * 87.14 % on fn_802BEDE8 with `u8` and 100.00 % with `s8`).  Added with that registration (rule 2:
 * the range owns the address).  The map's name carries no mangling, so the consumer-side declaration
 * is C linkage; the owner's own definition (`src/ef/fn_800CDB2C.cpp:247`) is still C++ linkage. */
void fn_800CF394(s8 value);
u8  fn_800CF208(void);
u32 move_work_state_ck(void);
/* 0x800CF2C4 - the play-mode/move-state dispatcher: it reads `fn_800CF208()`, switches on
 * `PlayMode_ck()` and hands the caller's mode byte on (`Pl/pl_act_step.cpp`'s act-175 arm is a
 * consumer, so the declaration belongs here - rule 2). */
void ef_move_state_dispatch(u8 mode);

s32 fn_800CED10(char* path, u32 dma, u32 size);
/* Builds the nw4r::ef::EffectSystem the effect manager keeps at `eft_control` +0x04. */
s32 fn_800D3C0C(void);
s32 fn_800CEE2C(const char* path, void* info);
/* The task-table entry `ef/fn_80059550.cpp` reads (its own return view is `_GXTexObj*`). */
u8* fn_800D0568(s32 index);

/* 0x800D0708 - the owner's own byte read (`src/ef/fn_800CDB2C.cpp:484`, `system_w`'s +0x7D3, whose
 * consumer compares the result against 1).  The name is derived from the call surface, not the dump's:
 * the runtime dump's map repeats a non-matching `SaveLoad::DidGameIDChange(void)` across this band.
 * It carries no mangling, so it is declared at C scope (playbook 42/48) - the consumers
 * (`Pl/fn_80224AC4.cpp`, `lobby/fn_802076D4.cpp`) declare it `extern "C"` too, and a declaration
 * in the C++-scope block below clashes with those (`(10505) illegal overloading`).  Added with
 * `Pl/fn_80224AC4.cpp` (rule 2). */
u32 game_ready_ck(void);

/* 0x800CF3C4 - the owner's own narrowed read, called by `menu/fn_802A6624.cpp`'s
 * `fn_802A6624`/`fn_802A674C` as a signed byte (the caller keeps an `extsb` on the widened
 * form and compares it with a signed `cmpwi`+`ble`).  Added with `menu/fn_802A6624.cpp`
 * (rule 2: this range owns the address). */
s8 fn_800CF3C4(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800CEEBC - the random ring, a C++ free function: the map name `ran_suu__Fl` is a mangling, so the
 * declaration sits at C++ scope and the front-end reproduces it (docs/plan.md 6.5 rule 9).  r3 is the
 * ring index (its body scales it by 2 to reach `field_0x18[index]`), it loads the ring's base from
 * inside, and it returns the narrowed u16 it has just stored (`clrlwi r3,r0,16`) - which is also how
 * the owner defines it (`src/ef/fn_800CDB2C.cpp:559`, `u16 ran_suu(long index)`).  Added with
 * `enemy/fn_801B7020.cpp`, which calls it as `ran_suu(0)`. */
u16 ran_suu(s32 index);

/* 0x800CF218 - the play-mode byte `PlayMode_ck__Fv`; the consumers use `u8` (values < 7).  Added
 * with `Pl/fn_80229ECC.cpp`, which gates on `(u8)PlayMode_ck() == 3`. */
u8 PlayMode_ck(void);

/* 0x800D3058 - the g3d work-handle release `push_g3d_wk__FP9_g3d_work`: the map name carries an
 * argument list, so it is a C++ free function and the declaration sits at C++ scope (rule 9).  The
 * owner uses the same spelling internally (`src/ef/fn_800CDB2C.cpp`).  Added with
 * `ef/eft035.cpp`, whose release paths hand it the pooled `_g3d_work*` handles. */
struct _g3d_work;
void push_g3d_wk(struct _g3d_work* work);
#endif

#endif /* MHTRI_EF_FN_800CDB2C_H */
