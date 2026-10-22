/* Leaf header (docs/plan.md 6.5 rule 2): `cockpit_work`, the `hud/cockpit_quest` unit's two cockpit work records, for a
 * consumer that cannot include the owner's full header (`hud/cockpit_quest.h` includes this one). */
#ifndef MHTRI_HUD_COCKPIT_WORK_H
#define MHTRI_HUD_COCKPIT_WORK_H

#include "hud/cockpit_work_types.h"

/* The two cockpit work records (`.bss` 0x806BDCC8, 0x328 B = 2 x 0x194).  Defined by the band below,
 * which initialises them (`fn_802E4978` memsets one); declared here, never defined (playbook 29's
 * rule for another unit's storage). */
extern CockpitWork cockpit_work[2];

#endif /* MHTRI_HUD_COCKPIT_WORK_H */
