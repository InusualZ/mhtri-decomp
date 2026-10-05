/*
 * `lobby_world_block` - the one declaration of the cached user-data pointer (.sbss 0x80794880, 4 B, rule 2),
 * defined by `src/fn_80047398.cpp` (its `.sbss` 0x80794870-0x80794884).  `get_userdata()` (0x8004D120) and the
 * user-data init `fn_800497B4`/`fn_800498EC` store the 0x6000-byte user-data block in it; the lobby band reads
 * the block through byte offsets.  Every reader loads it as a pointer (`lwz ...@sda21`), so the declaration
 * is a pointer; units that view the block through a record type keep their own pointer spelling until the
 * views are folded.
 */
#ifndef MHTRI_FN_80047398_LOBBY_WORLD_BLOCK_H
#define MHTRI_FN_80047398_LOBBY_WORLD_BLOCK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u8* lobby_world_block;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FN_80047398_LOBBY_WORLD_BLOCK_H */
