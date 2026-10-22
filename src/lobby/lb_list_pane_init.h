/* `lb_list_pane_init` and `lb_list_pane_pos_set`, defined by `lobby/lb_pane_ui.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_LIST_PANE_INIT_H
#define MHTRI_LOBBY_LB_LIST_PANE_INIT_H

#include "types.h"

struct LbEntryPane;
struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* Sets the equipment box pane `pane` up for slot kind `slot_kind` and the player `plw`. */
void lb_list_pane_init(struct LbEntryPane* pane, s16 slot_kind, struct _PLW* plw);
/* Puts the pane's cursor on the box entries `id_a`/`id_b`/`id_c` (-1: none). */
void lb_list_pane_pos_set(struct LbEntryPane* pane, s16 id_a, s16 id_b, s16 id_c);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_LIST_PANE_INIT_H */
