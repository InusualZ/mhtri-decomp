/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft052.cpp`'s `eft052_box_open`.  C linkage (the map row is a plain
 * name). */
#ifndef MHTRI_EF_EFT052_BOX_OPEN_H
#define MHTRI_EF_EFT052_BOX_OPEN_H

#include "types.h"
#include "ef/eft052_box.h"   /* Eft052BoxCell, Eft052BoxGrid */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80359CA8 - opens the item box over `cells` (with the sort buffer `sorted`) and the window data `grid`. */
void eft052_box_open(Eft052BoxCell* cells, Eft052BoxCell* sorted, Eft052BoxGrid* grid);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT052_BOX_OPEN_H */
