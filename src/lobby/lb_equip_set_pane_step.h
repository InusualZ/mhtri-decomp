/* Leaf header (docs/plan.md 6.5 rule 2): the box-pane entry points `lobby/lb_pane_ui.cpp` defines and
 * `lobby/lb_screen_step.cpp` calls, which the owner's header does not declare.  C linkage (the map rows are plain names). */
#ifndef MHTRI_LOBBY_LB_EQUIP_SET_PANE_STEP_H
#define MHTRI_LOBBY_LB_EQUIP_SET_PANE_STEP_H

#include "types.h"

struct _PLW;
struct LbEntryPane;
struct LbEquipSetPane;
struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801EF244 - sets the equipment-set pane up for slot `slot` with the box entries `box_a`/`box_b`/`box_c`. */
void lb_equip_set_pane_init(struct LbEquipSetPane* pane, s32 slot, u16 box_a, u16 box_b, u16 box_c, u8 flag,
                            struct _PLW* plw);
/* 0x801EF3B0 - steps the equipment-set pane: 1 decided, 2 cancelled. */
s32 lb_equip_set_pane_step(struct LbEquipSetPane* pane, struct LbEntryPane* box, u8 mode);
/* 0x801F865C - draws the equipment-set pane. */
void lb_equip_set_pane_draw(struct LbEquipSetPane* pane, u16 panel, struct LbEntryPane* box, u16 box_panel, u8 mode);
/* 0x801F06F8 - resets the box pane's list. */
void lb_entry_pane_list_reset(struct LbEntryPane* pane, u8 mode);
/* 0x801F1540 - starts the box pane's frame. */
void lb_entry_pane_frame_begin(struct LbEntryPane* pane);
/* 0x801F085C - steps the box pane's own menu; nonzero while it owns the input. */
s32 lb_entry_pane_menu_step(struct LbEntryPane* pane, u8 a, u8 b);
/* 0x801EF73C - the box entry under the pane's cursor. */
s16 lb_digit_pane_value(struct LbEntryPane* pane);
/* 0x801F0088 - moves the box pane's cursor. */
void lb_entry_pane_input(struct LbEntryPane* pane);
/* 0x801F6AC8 - draws the head of the walking page at `pos` for row `row`. */
void lb_page_walk_head(struct _mh_ivec2_* pos, s16 row);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EQUIP_SET_PANE_STEP_H */
