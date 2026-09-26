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

/* 0x800CF384 - r3 unused; the ef unit's live-effect count, read as a byte by
 * `enemy/fn_801A9540.cpp`'s entry point.  Added with that unit's registration (rule 2: this
 * range owns the address). */
u32 fn_800CF384(void);
u8  fn_800CF208(void);
u32 fn_800CF280(void);

s32 fn_800CED10(char* path, u32 dma, u32 size);
/* Builds the nw4r::ef::EffectSystem the effect manager keeps at `eft_control` +0x04. */
s32 fn_800D3C0C(void);
s32 fn_800CEE2C(const char* path, void* info);
/* The task-table entry `ef/fn_80059550.cpp` reads (its own return view is `_GXTexObj*`). */
u8* fn_800D0568(s32 index);

/* 0x800D0708 - the owner's own byte read (`src/ef/fn_800CDB2C.cpp:484`, `system_w`'s +0x7D3, whose
 * consumer compares the result against 1).  The map spells the symbol `fn_800D0708`, a placeholder
 * stem rather than a mangling, so it is declared at C scope (playbook 42/48) - the consumers
 * (`Pl/fn_80224AC4.cpp`, `lobby/fn_802076D4.cpp`) declare it `extern "C"` too, and a declaration
 * in the C++-scope block below clashes with those (`(10505) illegal overloading`).  Added with
 * `Pl/fn_80224AC4.cpp` (rule 2). */
u32 fn_800D0708(void);

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
#endif

#endif /* MHTRI_EF_FN_800CDB2C_H */
