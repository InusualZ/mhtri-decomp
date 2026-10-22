/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit_quest.cpp`'s spawn notice, for a consumer that cannot include the
 * owner's full header (it declares `get_move_work_adrs` with a return type `quest/quest_entry.h` spells differently).
 * C linkage (the map row is the plain name). */
#ifndef MHTRI_HUD_QUEST_NOTICE_SPAWN_H
#define MHTRI_HUD_QUEST_NOTICE_SPAWN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E88BC - shows the spawn notice of a large monster in area `idx`; the first argument is ignored. */
void quest_notice_spawn(s32 unused_ctx, u8 idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_QUEST_NOTICE_SPAWN_H */
