/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft022_fx.cpp`'s `eft021_pl_create`. */
#ifndef MHTRI_EF_EFT021_PL_CREATE_H
#define MHTRI_EF_EFT021_PL_CREATE_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801153D0: creates the actor's eft021 effect record (registry name) */
struct _EFT* eft021_pl_create(struct _PLW* source, s32 timer, u8 type);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT021_PL_CREATE_H */
