/* `cockpit_notice_update`, defined by `hud/cockpit.cpp` (leaf header, rule 2). */
#ifndef MHTRI_HUD_COCKPIT_NOTICE_UPDATE_H
#define MHTRI_HUD_COCKPIT_NOTICE_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Resets the cockpit notices. */
void cockpit_notice_init(void);
/* Steps the cockpit notices once a frame. */
void cockpit_notice_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_NOTICE_UPDATE_H */
