/* Leaf header (docs/plan.md 6.5 rule 2): `draw_shape.cpp`'s `quest_fin_tex_load`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_QUEST_FIN_TEX_LOAD_H
#define MHTRI_QUEST_FIN_TEX_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80055770 (0xC8): loads the quest-end banner texture for end kind `kind`. */
void quest_fin_tex_load(u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_QUEST_FIN_TEX_LOAD_H */
