/* Leaf header (docs/plan.md 6.5 rule 2): the save-band unit's (0x8004CAD8..) gallery opener `enemy/em_pop.cpp` calls,
 * for a consumer that cannot include the owner's full header.  C++ scope: the map row is `gallery_open__FUc`. */
#ifndef MHTRI_GALLERY_OPEN_H
#define MHTRI_GALLERY_OPEN_H

#include "types.h"

/* 0x8004E824 - opens gallery entry `id`. */
void gallery_open(u8 id);

#endif /* MHTRI_GALLERY_OPEN_H */
