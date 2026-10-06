/* Leaf header: `quest_marker_draw` (0x802EBED8), the one symbol of `src/hud/cockpit_quest.cpp`'s quest-window band
 * that the unit's top half calls (the band's full header `hud/cockpit_quest_marker.h` redefines `CockpitWork` and is
 * included only mid-file). */
#ifndef MHTRI_HUD_QUEST_MARKER_DRAW_H
#define MHTRI_HUD_QUEST_MARKER_DRAW_H

#include "types.h"
#include "hud/cockpit_quest.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802EBED8 - draws the quest marker ring for the cockpit work record: the unfaded arm of
 * `quest_marker_draw_faded` (`id` is the ring's layout record, `mode` 0 for the first view). */
void quest_marker_draw(CockpitWork* work, u16 id, u32 mode, const _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_QUEST_MARKER_DRAW_H */
