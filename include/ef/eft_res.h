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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_RES_H */
