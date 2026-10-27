/*
 * Symbols of the homebutton band that no registered unit owns (docs/plan.md 6.5 rule 2).
 *
 * The text writer's character-width cache (.bss 0x80790250, 0x580 bytes) is read and written only by
 * `homebutton/hbm_value.cpp`, but its link slot (after hbm_text_panel's .bss, before hbm_anim_record's) places
 * its definition in one of hbm_text_panel / hbm_kb_widget / hbm_kb_list / hbm_kb_child, so hbm_value cannot
 * claim it; the declaration moves to the owner's header once that unit claims the range.
 */
#ifndef MHTRI_UNSPLIT_HOMEBUTTON_H
#define MHTRI_UNSPLIT_HOMEBUTTON_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80790250 (0x580): The glyph widths the text writer caches for the codes 0x20..0x17F (-1: not measured). */
extern s32 hbmTextCharWidths[0x160];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_HOMEBUTTON_H */
