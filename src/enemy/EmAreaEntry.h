#ifndef MHTRI_ENEMY_EMAREAENTRY_H
#define MHTRI_ENEMY_EMAREAENTRY_H

#include "types.h"

/* One entry of the area/group table `lbl_806A54E0` (0x806A54E0, .bss, unsplit): four of them per
 * group, 32 groups (the `.bss` run 0x806A54E0..0x806A76E0 the retired object's data run gives).
 * Only the four bytes `fn_8012E968` tests are named.
 * size: 0x44 */
typedef struct EmAreaEntry {
    /* +0x00 */ u8 active;           /* nonzero while the entry is in use */
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 group;            /* matched against `_ENEMY_WORK::group` */
    /* +0x03 */ u8 unused_0x03[0x0A - 0x03];
    /* +0x0A */ u8 area_no;          /* matched against `_ENEMY_WORK::field_0x00A` */
    /* +0x0B */ u8 unused_0x0B;
    /* +0x0C */ u8 field_0x0C;       /* must be 1 for the entry to count */
    /* +0x0D */ u8 unused_0x0D[0x44 - 0x0D];
} EmAreaEntry; /* size: 0x44 */

#endif
