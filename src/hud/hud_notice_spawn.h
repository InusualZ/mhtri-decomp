/* hud/hud_notice_spawn.h - leaf header: `hud_notice_spawn` (0x802DA454), owned by `hud/cockpit.cpp`. */
#ifndef MHTRI_HUD_HUD_NOTICE_SPAWN_H
#define MHTRI_HUD_HUD_NOTICE_SPAWN_H

#include "types.h"

struct HudNotice;

#ifdef __cplusplus
extern "C" {
#endif
/* Spawns HUD notice `id` of `kind` at (x, y). */
struct HudNotice* hud_notice_spawn(s32 kind, s32 id, s16 x, s16 y, s32 a, s32 b, s32 c);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_HUD_NOTICE_SPAWN_H */
