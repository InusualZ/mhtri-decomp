/*
 * Network/NetworkCommunityPat.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x803E4888..0x803FCC34.  Sections of the candidate unit: extab 0x8001ACC8..0x8001BCAC; extabindex 0x8003B01C..0x8003BF40; .text 0x803E4888..0x803FCC34; .ctors 0x8056F3BC..0x8056F3C0; .data 0x805FBC78..0x805FC324; .sdata 0x80793940..0x80793968; .sdata2 0x8079C790..0x8079C7D0.
 *
 * WHAT IT IS. the `NetworkCommunityPat` band: the community/friend request code (`syncFriends`, `inviteFriend`, `removeFriend`,
 *   `requestBlockList`, `sendFriendRequest`, `acceptFriendRequest`), the layer slot notifications (`pollLayerSlots`,
 *   `notifyLayerSlotSummary`, `notifyLayerEvent`), the collection-log setters and the many `dtor_` helpers (423 functions,
 *   126 named in the map, `NetworkCommunityPat`'s manglings among them).  Its `.ctors` word is the band's static initialiser.
 *
 * WHY IT SITS HERE. the phase 1 reconciliation (docs/splits/proposals/phase1-e.md) cut no seam inside 0x803E4888..0x803F73C0 and folded the
 *   `Network/network_packet` guess (0x803F73C0..0x803FCC34) into it; the left edge is the registered end of
 *   `Network/NetworkLayerPatStep.cpp` (grade guess).  It is probably several TUs (the 76 KB run has no pool or `.data` pin).
 *
 * UNKNOWN. every body; the TU extent (phase1-e.md open question 3: the NetworkLayer TU spans the registered `NetworkLayerPatStep.cpp`);
 *   the class declaration lives in `include/Network/NetworkCommunityPat.h`.
 *
 * FLAGS. the `Network` lib's `cflags_network` (the registered neighbours add `-O3`/`-inline noauto` per unit; unmeasured here).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit Network/NetworkCommunityPat.cpp`), and the pass that writes the bodies defines them.
 */
