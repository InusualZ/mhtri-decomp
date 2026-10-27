/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_pane_ui.cpp`'s `lobby_enter_seq_step`, which `lobby/lb_menu_pos_tbl.cpp` calls. */
#ifndef MHTRI_LOBBY_LOBBY_ENTER_SEQ_STEP_H
#define MHTRI_LOBBY_LOBBY_ENTER_SEQ_STEP_H

#include "types.h"

struct LbChgSeqWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801FB524 (0x2E8): one step of the lobby's entry sequence (save, area load, map setup); returns 1 when done. */
s32 lobby_enter_seq_step(struct LbChgSeqWork* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LOBBY_ENTER_SEQ_STEP_H */
