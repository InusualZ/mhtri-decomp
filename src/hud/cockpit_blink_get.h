/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_COCKPIT_BLINK_GET_H
#define MHTRI_HUD_COCKPIT_BLINK_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D9EA8 - the +0x04 float of the cockpit blink record, the factor the cockpit draws scale colour by */
f32 cockpit_blink_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_BLINK_GET_H */
