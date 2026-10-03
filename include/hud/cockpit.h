/* The cockpit HUD band `hud/cockpit.cpp` (`.text` 0x802D9EA4..0x802E0740): the entries other units call
 * (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_HUD_COCKPIT_H
#define MHTRI_HUD_COCKPIT_H

#include "types.h"

extern "C" {

/* 0x802DFC6C - a cockpit entry with no arguments that the note band's pane code calls (no body yet). */
void fn_802DFC6C(void);

}

/* Declarations moved here from `include/unsplit/lobby.h, menu.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

s8* str_tbl_33_get(u8 id);

void draw_lsp_parts(void);                 /* 0x802DFEBC */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_COCKPIT_H */
