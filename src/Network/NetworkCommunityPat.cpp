/*
 * Network/NetworkCommunityPat.cpp - STUB (no bodies yet).
 *
 * SECTIONS. extab 0x8001B178..0x8001B780; extabindex 0x8003B544..0x8003B8F8; .text 0x803F0294..0x803F6458; .data 0x805FC4D0..0x805FC820;
 *   .sdata 0x80793948..0x80793950; .sdata2 0x8079C7A8..0x8079C7B8.
 *
 * WHAT IT IS. the `NetworkCommunityPat` class (constructor 0x803F02C4, table 0x805FC728, allocation 0x25EC): the community/friend
 *   request code (`syncFriends`, `inviteFriend`, `removeFriend`, `requestBlockList`, `sendFriendRequest`, `acceptFriendRequest`),
 *   the profile writers and `sendNtcCollectionLog`.  The class declaration lives in `include/Network/NetworkCommunityPat.h`.
 *
 * WHY IT SITS HERE. the network pilot round 3 recut shrank the phase 4 stub (0x803E4888..0x803FCC34, which phase1-e.md had flagged
 *   as several TUs) to the class's own run: `NetworkLayerPat` (to 0x803EF668) and its base `NetworkCommunity` (to 0x803F0294)
 *   come before it, `NetworkFileFetcher`, `NetworkSocketWii`, `NetworkUniqueId`, `NetworkUnitPacket` and `PatConnection` after.
 *   The `.data` run is the class's strings and table, the V->S seam at 0x805FC4D0 is its left edge.
 *
 * RESIDUAL (splitcheck). `pool` FAILs: the pool word 0x8079C7AC is first read at 0x803F0800, earlier than the word before it
 *   (0x8079C7A8, first read at 0x803F0CE4) - not text order for one TU, or one read is a false decode; both words are read only
 *   from this range.
 *
 * UNKNOWN. every body (the inventory is `ledger.py unit Network/NetworkCommunityPat.cpp`, the map and `splits.txt`).
 *
 * FLAGS. the `Network` lib's `cflags_network`, unmeasured: there is no body to measure them on.
 */

#include "Network/NetworkCommunityPat.h"
