/* Leaf header (docs/plan.md 6.5 rule 2): the two companion-command senders of `lobby/lb_companion_ui.cpp` that
 * `enemy/enemy_control.cpp` calls, for a consumer that cannot include the owner's full header.  C linkage (the map rows
 * are plain names). */
#ifndef MHTRI_LOBBY_LB_SUB0F_SEND_H
#define MHTRI_LOBBY_LB_SUB0F_SEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80339278 - sends the sub-0x0F spawn command: the monster, its ground record, spawn order, stat level and scale. */
/* untyped: caller-owned payload - the ground record the command copies */
void lb_sub0f_send(u8 index, u8 value, void* text, u16 id, u8 flag, f32 scale);
/* 0x803393F4 - sends the sub-0x10 command with one signed byte. */
void lb_sub10_send(u8 index, u8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SUB0F_SEND_H */
