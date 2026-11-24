/* Leaf header (docs/plan.md 6.5 rule 2): `lsp_slide_depth_scale`, which `enemy/em022_prog` defines - the 2.0f at
 * 0x8079AF48, the first word of that unit's .sdata2, read by `hud/lobby_hud`'s `draw_lsp_anim_ary`. */
#ifndef MHTRI_ENEMY_LSP_SLIDE_DEPTH_SCALE_H
#define MHTRI_ENEMY_LSP_SLIDE_DEPTH_SCALE_H

#include "types.h"

extern const f32 lsp_slide_depth_scale;

#endif
