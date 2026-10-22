#ifndef MHTRI_ENEMY_EMAREAENTRY_H
#define MHTRI_ENEMY_EMAREAENTRY_H

#include "types.h"
#include "enemy/ENEMY_MINI_WORK.h"

/* One entry of the area table `em_area_entry_tbl` (0x806A54E0..0x806A76E0, `enemy/enemy_control.cpp`'s `.bss`): the
 * record `em_get_unique_work`'s mangling calls `_ENEMY_MINI_WORK` (its layout is `enemy/ENEMY_MINI_WORK.h`).
 * size: 0x44 */
typedef struct _ENEMY_MINI_WORK EmAreaEntry;

#endif
