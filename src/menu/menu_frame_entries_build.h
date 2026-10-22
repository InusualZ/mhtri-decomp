/* `menu_frame_entries_build`, defined by `menu/menu_message.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_MENU_FRAME_ENTRIES_BUILD_H
#define MHTRI_MENU_MENU_FRAME_ENTRIES_BUILD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Builds and draws the frame entries of the two item runs at layout sprite `lsp_index`. */
void menu_frame_entries_build(u16* src_a, u16* src_b, s8 kind, u16 lsp_index, u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_FRAME_ENTRIES_BUILD_H */
