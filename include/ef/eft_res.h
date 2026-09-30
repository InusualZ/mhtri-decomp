/*
 * Declarations for the symbols `src/ef/eft_res.cpp` owns (docs/plan.md 6.5, rule 2).  The signatures are
 * the owner's own `extern "C"` definitions, so a consumer that includes this header cannot disagree
 * with the owner (an owned symbol is declared once, in the owner's header).  Only the symbols a consumer
 * needs today are declared; the rest of the unit's entry points follow with the extern sweep.
 */
#ifndef MHTRI_EF_EFT_RES_H
#define MHTRI_EF_EFT_RES_H

#include "types.h"
#include "ef.h"

/* The effect resource slot `fn_800F8788` pools; its layout stays the owner's business. */
struct EftResSlot;

#ifdef __cplusplus
extern "C" {
#endif

/* Retires one pooled effect's runtime record. */
void fn_800F886C(void* self);
/* Places `count` pooled models in the `mode` layout and files the result through `arg`. */
void fn_800F93D8(_EFT* self, void** models, s32 mode, s32 count, void* arg);
/* Pools the slots `size` bytes' worth of effect records need and returns the first record. */
EftResSlot* fn_800F8788(u32 size);
/* Releases `count` handles from `list` back to the heap. */
void fn_800F8A44(void* list, s32 count);
/* 0x800F92F4 - the per-mode alive check the effect state bodies make (`mode` 0 is the "may the
 * record keep stepping" test).  Added with `ef/fn_8030681C.cpp`, whose kind-1 bodies branch on it.
 * The second parameter is `u32`: `ef/fn_801173AC.cpp` includes this header and carries its own
 * declaration of the same address with `u32` (its line 323), so any other width here is a second
 * overload in that TU - `s32` cost the whole tree its build with (10197) illegal function
 * overloading.  `_EFT*` for `self` is this header's spelling; the owner's own `void*` definition
 * would be the third overload there, so it stays a rule-2 residual for the consolidation pass. */
s32 fn_800F92F4(_EFT* self, u32 mode);
/* Takes one pooled model record out of the effect-model pool and returns it (the `EftModel`
 * `res_eft_*_model_create` then binds a character to).  Added with `ef/eft035.cpp`, which seeds each
 * of its work block's model slots with it. */
u8* fn_800F8914(void);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* The two model-create entry points are C++ free functions (the map spells them
 * `res_eft_model_create__FP6MHcharUsUl` / `res_eft_UV_model_create__FP6MHcharUsUllPP9_g3d_worklUc`),
 * so their declarations sit at C++ scope where the front-end reproduces the mangling (rule 9).
 * Added with `ef/eft035.cpp` (rule 2: this TU owns both addresses; `res_eft_model_create` is the
 * owner's own `src/ef/eft_res.cpp:801`, `res_eft_UV_model_create` is still un-reconstructed). */
struct MHchar;
struct _g3d_work;
/* 0x800F91B4 - creates the pooled effect `id` of resource group `group`; `stage/shell.cpp`'s
 * `res_eft_create_shell` forwards to it with flags 0. */
namespace nw4r { namespace ef { struct Effect; } }
nw4r::ef::Effect* res_eft_create(u16 group, u16 id, u32 flags);
void* res_eft_model_create(MHchar* model, u16 id, u32 arg);
void* res_eft_UV_model_create(MHchar* model, u16 id, u32 arg, long mode, struct _g3d_work** list,
                              long count, u8 flag);
#endif

#endif /* MHTRI_EF_EFT_RES_H */
