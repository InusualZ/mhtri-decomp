/* enemy/net_entry_post.h - leaf header (docs/plan.md 6.5 rule 2) for `enemy/em_prog_support.cpp`'s `net_entry_post`.
 *   C linkage (the map row is a plain name). */
#ifndef MHTRI_ENEMY_NET_ENTRY_POST_H
#define MHTRI_ENEMY_NET_ENTRY_POST_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80384048 (0xAC): posts the player's entry with `text` through the network control; 1 when it went out. */
u32 net_entry_post(struct _PLW* plw, u8* text);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_NET_ENTRY_POST_H */
