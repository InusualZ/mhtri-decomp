/* unsplit/lobby_area_view.h - band header (docs/plan.md 6.5 rule 2): the lobby `.data` table no registered unit
 *   claims (between `lobby/lb_menu_scratch.cpp`'s `.data` end 0x805BA540 and the next claim), read by
 *   `lobby/fn_80219260.cpp`'s `lb_area_view_set` and `menu/menu_placeinfo.cpp`'s `placeinfo_model_create`. */
#ifndef MHTRI_UNSPLIT_LOBBY_AREA_VIEW_H
#define MHTRI_UNSPLIT_LOBBY_AREA_VIEW_H

#include "types.h"
#include "ef.h"   /* Vec */

/* One placement: a position and the facing angle word.  size: 0x10 */
typedef struct LbAreaViewPos {
    /* +0x0 */ Vec pos;
    /* +0xC */ u32 angle;
} LbAreaViewPos;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x805BA540 (0x110): the placements by area, 17 records (GUESS name, registry). */
extern LbAreaViewPos lb_area_view_pos_tbl[17];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_LOBBY_AREA_VIEW_H */
