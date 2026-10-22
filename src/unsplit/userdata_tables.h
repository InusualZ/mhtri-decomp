/*
 * The save-block tables `fn_8004CAD8.cpp` reads that no registered unit owns (docs/plan.md 6.5 rule 2): the run
 * `.data` 0x80581180..0x805813E8 sits between `fn_80047398.cpp`'s `.data` (..0x80581180) and `userdata_item.cpp`'s
 * (0x805813E8..), and `fn_8004CAD8.cpp` is its only referrer - but its own `.data` starts at 0x80581584, after
 * `userdata_item.cpp`'s and `fn_8004C9A0.cpp`'s runs, so it cannot claim the run without owning theirs too.
 */
#ifndef MHTRI_UNSPLIT_USERDATA_TABLES_H
#define MHTRI_UNSPLIT_USERDATA_TABLES_H

#include "types.h"

/* One gallery unlock: the event id (0 ends the table) and its bit in the unlock words.  size: 0x2 */
typedef struct GalleryUnlock {
    /* +0x0 */ u8 id;
    /* +0x1 */ u8 bit;
} GalleryUnlock;

#ifdef __cplusplus
extern "C" {
#endif

/* .data 0x80581180 (0x148): the event ids whose flags the save block keeps, in bit order, ended by 0xFFFF. */
extern const u16 event_flag_id_table[];
/* .data 0x805812C8 (0xD0): the hunter points ranks 1..50 start at. */
extern const u32 hunter_rank_points_table[];
/* .data 0x80581398 (0x50): the gallery unlocks, ended by an id of 0. */
extern const GalleryUnlock gallery_unlock_table[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_USERDATA_TABLES_H */
