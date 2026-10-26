/* `lb_menu_row_mark_set` and its two siblings, defined by `lobby/fn_80219260.cpp` (leaf header, docs/plan.md 6.5
 * rule 2): the eight row-mark bytes of `lb_menu_scratch` (+0x156).  C linkage (plain map rows).  GUESS names. */
#ifndef MHTRI_LOBBY_LB_MENU_ROW_MARK_SET_H
#define MHTRI_LOBBY_LB_MENU_ROW_MARK_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021D86C - stores `mark` as row `row`'s mark. */
void lb_menu_row_mark_set(u8 row, u8 mark);
/* 0x8021D884 - sets the selected bit (4) on row `row`'s mark and clears it on the others. */
void lb_menu_row_mark_select(u8 row);
/* 0x8021D9E0 - clears the eight row marks. */
void lb_menu_row_marks_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_MENU_ROW_MARK_SET_H */
