/* `lb_npc_func`, the NPC sound-hook object (.bss 0x806AA710, 0x10 B) defined by `src/lobby/lb_npc.cpp`: the word at
 * +0x04 is the callback the per-frame sound driver fires when the SE system reports mode 2.
 */
#ifndef MHTRI_LOBBY_LB_NPC_FUNC_H
#define MHTRI_LOBBY_LB_NPC_FUNC_H

#include "types.h"

/* size: 0x10 */
struct LbNpcFunc {
    /* +0x00 */ u8 pad_0x00[4];
    /* +0x04 */ void (*field_0x04)(void);
    /* +0x08 */ u8 pad_0x08[8];
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct LbNpcFunc lb_npc_func;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_FUNC_H */
