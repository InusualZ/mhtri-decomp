/* Leaf header (docs/plan.md 6.5 rule 2): `ef/fn_8030681C.cpp`'s `eft042_set`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_EF_EFT042_SET_H
#define MHTRI_EF_EFT042_SET_H

#include "types.h"

struct _PLW;

extern "C" {

/* 0x80306D14 - spawns effect 42 from a model (registry name). */
void eft042_set(struct MHchar* model, u8 type);

}

#endif /* MHTRI_EF_EFT042_SET_H */
