/* `monster_mark_ck`, defined by `hud/cockpit.cpp` (leaf header, rule 2; `hud/cockpit.h` does not declare it).  C linkage
 * (the map row is a plain name). */
#ifndef MHTRI_HUD_MONSTER_MARK_CK_H
#define MHTRI_HUD_MONSTER_MARK_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DE4FC - nonzero when monster `monster` is marked. */
s32 monster_mark_ck(u8 monster);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_MONSTER_MARK_CK_H */
