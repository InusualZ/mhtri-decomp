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

u8  fn_800CF208(void);
u32 fn_800CF280(void);

s32 fn_800CED10(char* path, u32 dma, u32 size);
/* Builds the nw4r::ef::EffectSystem the effect manager keeps at `eft_control` +0x04. */
s32 fn_800D3C0C(void);
s32 fn_800CEE2C(const char* path, void* info);
/* The task-table entry `ef/fn_80059550.cpp` reads (its own return view is `_GXTexObj*`). */
u8* fn_800D0568(s32 index);

#ifdef __cplusplus
}
#endif

/* The per-kind work-block accessors and the PRNG, added by `enemy/fn_801B0010.cpp` (rule 2: this unit
 * owns the addresses).  The target objects reference their MANGLINGS
 * (`get_move_work_max__FUc`/`get_move_work_adrs__FUc`/`ran_suu__Fl`), so they are declared at C++
 * scope, outside the `extern "C"` block above (rule 9) - the same spelling `ef/eft_res.cpp` uses.
 * Signatures are the owner's own definitions: `get_move_work_max` answers a 16-bit count,
 * `get_move_work_adrs` the block base, `ran_suu` takes the `long` index its map name spells and
 * answers the 16-bit value its callers mask. */
#ifdef __cplusplus
u16 get_move_work_max(u8 index);
u16 ran_suu(long index);
#endif
/* `get_move_work_adrs` is deliberately NOT declared here: three consumers spell its return type
 * three ways (`void*` in `ef/eft_res.cpp` and this owner, `u32` in `sound/fn_800EF7D8.cpp`), and
 * MWCC rejects the pair as an illegal overload, so the shared header cannot carry it until one
 * spelling is chosen.  `enemy/fn_801B0010.cpp` declares it locally and asks for the unification in
 * its outbox (`shared-file`). */
#ifdef __cplusplus
/* 0x800CEEBC - the random ring, a C++ free function: the map name `ran_suu__Fl` is a mangling, so the
 * declaration sits at C++ scope and the front-end reproduces it (docs/plan.md 6.5 rule 9).  r3 is the
 * ring index (its body scales it by 2 to reach `field_0x18[index]`), it loads the ring's base from
 * inside, and it returns the narrowed u16 it has just stored (`clrlwi r3,r0,16`) - which is also how
 * the owner defines it (`src/ef/fn_800CDB2C.cpp:559`, `u16 ran_suu(long index)`).  Added with
 * `enemy/fn_801B7020.cpp`, which calls it as `ran_suu(0)`. */
u16 ran_suu(s32 index);
#endif

#endif /* MHTRI_EF_FN_800CDB2C_H */
