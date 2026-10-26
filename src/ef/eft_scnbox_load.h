/* Leaf header (docs/plan.md 6.5 rule 2): `eft_scnbox_load`, which `ef/effect` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_EF_EFT_SCNBOX_LOAD_H
#define MHTRI_EF_EFT_SCNBOX_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F9C20 - loads the scene box data of map `mapno` area `areano` into `block`. */
void eft_scnbox_load(u8* block, u8 mapno, u8 areano);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_SCNBOX_LOAD_H */
