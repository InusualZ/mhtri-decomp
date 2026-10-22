/* Leaf header (docs/plan.md 6.5 rule 2): `draw_shape.cpp`'s `npc_model_res_ids` (`.data` 0x80582988, 0xB0 bytes), the
 * one declaration `lobby/lb_npc.cpp` needs.  GUESS name: the NPC body loader indexes it by model kind and hands the
 * second word to `ckResourceName`. */
#ifndef MHTRI_NPC_MODEL_RES_IDS_H
#define MHTRI_NPC_MODEL_RES_IDS_H

#include "types.h"
#include "lobby/lb_npc.h"   /* LbResId */

/* 0x80582988 - the NPC model resource names by model kind (22 eight-byte records). */
extern LbResId npc_model_res_ids[];

#endif /* MHTRI_NPC_MODEL_RES_IDS_H */
