/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_PUT_BUTTON_GUIDE_H
#define MHTRI_HUD_PUT_BUTTON_GUIDE_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DF7CC - the button guide's base sprite and the table-A sprite of a kind, when the pointer is not in use */
void put_button_guide(s32 kind, const _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_PUT_BUTTON_GUIDE_H */
