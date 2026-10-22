/* Leaf header (docs/plan.md 6.5 rule 2): symbols `fn_8004CAD8.cpp` defines, for `hud/cockpit_quest.cpp`. */
#ifndef MHTRI_TEX_UV_COPY_H
#define MHTRI_TEX_UV_COPY_H

#include "types.h"
#include "hud/layout_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800526E4: copies one texture coordinate pair. */
void tex_uv_copy(_mh_tex_uv_* dst, const _mh_tex_uv_* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_TEX_UV_COPY_H */
