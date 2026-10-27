/* `lb_s13_help_line_draw` and the other screen-13 draw helpers, defined by `lobby/lb_companion_ui.cpp` (leaf header,
 * rule 2).  C linkage: the map rows are the plain names. */
#ifndef MHTRI_LOBBY_LB_S13_HELP_LINE_DRAW_H
#define MHTRI_LOBBY_LB_S13_HELP_LINE_DRAW_H

#include "types.h"
#include "lobby/lb_screen13_work.h" /* LbScreen13Work (rule 1) */

#ifdef __cplusplus
extern "C" {
#endif

void lb_subpage_arrow_draw(void);
void lb_s13_menu_draw(LbScreen13Work* work);
void lb_s13_help_draw(void);
void lb_s13_entry_list_draw(LbScreen13Work* work, bool active);
void lb_s13_entry_status_draw(u8 entry);
void lb_s13_dance_list_draw(LbScreen13Work* work, bool active);
void lb_s13_dance_pick_draw(LbScreen13Work* work);
void lb_s13_dance_values_draw(LbScreen13Work* work);
void lb_s13_help_line_draw(LbScreen13Work* work);
void lb_s13_slot_list_draw(LbScreen13Work* work, u32 active);
void lb_s13_slot_action_draw(LbScreen13Work* work);
void lb_s13_skill_list_draw(LbScreen13Work* work, bool active);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_S13_HELP_LINE_DRAW_H */
