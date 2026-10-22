/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_infomation` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C++ linkage (the map row is this declaration's mangling, rule 9). */
#ifndef MHTRI_MENU_PUT_STATUS_INVORK_SKILL_SEP_H
#define MHTRI_MENU_PUT_STATUS_INVORK_SKILL_SEP_H

#include "types.h"
#include "hud/layout_types.h"   /* _mh_ivec2_, passed by value */

struct _PLW;

/* 0x8030B868 - draws one page of the status screen's active skills */
void Put_status_invork_skill_sep(_PLW* plw, _mh_ivec2_ pos, s8 page);

#endif /* MHTRI_MENU_PUT_STATUS_INVORK_SKILL_SEP_H */
