/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `qnpc_res_table` (`.data` 0x80582AB8, 0x20 B), the quest NPC model list that
 * `draw_shape.cpp`'s `.data` range owns and `lobby/lb_quest_screen.cpp`'s `quest_npc_res_load` walks.
 */
#ifndef MHTRI_DRAW_SHAPE_QNPC_RES_TABLE_H
#define MHTRI_DRAW_SHAPE_QNPC_RES_TABLE_H

#include "types.h"

/* One entry of the quest NPC model list: the file's size and its path (`"14/npc011.brres"`, ...); an entry whose
 * size is 0 ends the list.  size: 0x8 */
struct QnpcResEntry {
    /* +0x0 */ u32 size;
    /* +0x4 */ char* path;
};

#ifdef __cplusplus
extern "C" {
#endif

extern QnpcResEntry qnpc_res_table[4];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DRAW_SHAPE_QNPC_RES_TABLE_H */
