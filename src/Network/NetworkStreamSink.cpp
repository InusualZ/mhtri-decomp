/*
 * Network/NetworkStreamSink.cpp - the stream sink with its frame scrambler and CRC (`NetworkStreamSink`), the
 *   manager logger accessor, the Udp peer's constructor, `NetworkConnectionStable`, `NetworkSlotQueues`, the send
 *   pool and the `NetworkStreamWriter` frame writers.
 * RANGE. .text 0x803C987C-0x803CCDF8 (74 functions); extab 0x80019554-0x800198C8, extabindex 0x80039E64-0x8003A0BC
 *   (50 entries), .rodata 0x80570C20-0x80570E20 (the CRC-16 table `checksum` reads), .data 0x805F8DB0-0x805F94E0.
 *   No .bss, .sdata, .sbss or .sdata2 of its own: its pool reads go to `Network/network_shared_data.cpp`.
 * RANGE. Left edge 0x803C987C: cut from `lobby/lb_server_sel_trans.cpp`.  No `.text` reference crosses it in either
 *   direction, every `.data` label below 0x805F8DB0 is read only from below the edge and every label from it only
 *   from above, the `.rodata` table is read only by `checksum`, and the extab/extabindex records split at the same
 *   function (`~NetworkStreamSink`, the first framed one).  Right edge: `Network/NetworkPeerBase.cpp` at 0x803CCDF8.
 * RANGE. More than one TU, not split: `tudiscover` reads a `.data` zigzag at 0x805F9190 (a TU start in
 *   0x803C987C-0x803CA1D4) and a V->S seam at 0x805F91F0 (a TU start in 0x803CA3F8-0x803CA49C); neither `.text`
 *   edge is pinned, so the range stays one unit.
 * FLAGS. the `Network` lib's `cflags_network` (unmeasured until bodies exist; the transport band's `-O3 -pool off`
 *   is the first candidate, `docs/network.md`).
 * NAMES. The file name is a GUESS from the range's first class, `NetworkStreamSink`.  `networkInstance_destroyMutex`
 *   (0x803CA338, dtk's `dtor_`) is a GUESS in the scheme of its constructor `networkInstance_initMutex`: every caller
 *   destroys a record's member mutex with it (flags -1), and the extab cleanup records name it for that member.
 * RESIDUALS. Unwritten: every function in the range.
 */

#include "Network/NetworkStreamSink.h"
