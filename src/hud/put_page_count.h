/* Leaf header (docs/plan.md 6.5 rule 2): `put_page_count`, defined by `hud/cockpit.cpp`. */
#ifndef MHTRI_HUD_PUT_PAGE_COUNT_H
#define MHTRI_HUD_PUT_PAGE_COUNT_H

#include "types.h"
#include "hud/layout_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DB140: draws a page counter (`page` of `count`) from the sprite id run `ids` at `pos`. */
void put_page_count(u16* ids, u32 page, u32 count, u16 flags, const _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_PUT_PAGE_COUNT_H */
