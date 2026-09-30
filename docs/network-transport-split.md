# `Network/network_transport` is eight translation units

Survey for the split of `src/Network/network_transport.cpp` (`.text` 0x803CCDF8..0x803D3CE8, `.data`
0x805F94E0..0x805FA4EC). Method: `docs/data-order-seams.md` (a TU's `.data` is globals, out-of-line strings,
vtables in reverse class order, inline strings), `callers.py` for every `.data`/`.bss`/`.sbss` referrer, the
`extabindex` entry order of the target object (one entry per function, so it tiles by TU), and
`tudiscover.py at` for the `.text` intervals each seam pins. `datagap.py` independently reports "at least 8 TUs".

## Evidence

| retail `.data` | what | referrer (function, `.text`) | reading |
| --- | --- | --- | --- |
| 0x805F94E0 `__vt__15NetworkPeerBase` | vtable | ctor 0x803CCE54 | starts a TU (zigzag from the unclaimed vtable at 0x805F9490) |
| 0x805F9510 `__vt__17NetworkPeerBuffer` | vtable | ctor 0x803CCF30 | zigzag: adjacent vtables rise, a TU emits them in reverse |
| 0x805F9540 `__vt__14NetworkPeerUdp` | vtable | ctor is unowned (0x803CA3BC, another TU) | zigzag again |
| 0x805F9570 | string `NetworkPeerMcs::put: ...` | `put__14NetworkPeerMcs` 0x803CDC2C | V->S seam: the Mcs strings precede the Mcs vtable |
| 0x805F95E0 `__vt__14NetworkPeerMcs` | vtable | ctor 0x803CD7C0 | ends the Mcs TU (`D* S* V*`) |
| 0x805F9610..0x805F98F4 (13 strings) | `NetworkSingleTcp::*`, `NetworkMultipleUdp::*` | 0x803CE064..0x803CEBAC | V->S seam: a new TU's strings |
| 0x805F9938 `__vt__19NetworkResolverBase` | vtable | ctor 0x803CF0A8 | closes that TU |
| 0x805F9958 | string `NetworkResolverWii::check` | `check__18NetworkResolverWii` 0x803CF3B8 | V->S seam: strings between two vtables |
| 0x805F9980 `__vt__18NetworkResolverWii` | vtable | ctor 0x803CF170 | ends the resolver TU |
| 0x805F99A0 `__vt__18NetworkSessionBase` | vtable | ctor 0x803CF674 | zigzag: rises above the Wii table |
| 0x805F9A40.. (41 strings, a jump table, the `NetworkUnitPacket`/`NetworkSessionStable` vtables at 0x805FA6A8/0x805FA6E8) | state machine | first referrer `NetworkSessionStable_init` 0x803CFA44, none earlier | strings after the last vtable, all referrers in one function run: the next TU. Its data runs to 0x805FA788 (a V->S seam into `Network/fn_803D3CE8.cpp`'s TU) |

`.bss` (`networkUdpPacketBuffer` 0x806D2C60 read by the Udp peer, `networkMcsPacketBuffer` 0x806D3240 read by the
Mcs peer) and `.sbss` (`networkMcsRetryTime` 0x80794C98, Mcs `move`) follow the same order. `.sdata`/`.sdata2` are
owned by `Network/network_shared_data.cpp` and are not part of the split.

## Split table

| unit (GUESS names) | `.text` | `.data` | `.bss` / `.sbss` | `extabindex` (entries) | `extab` | classes / functions |
| --- | --- | --- | --- | --- | --- | --- |
| `Network/NetworkPeerBase.cpp` | 803CCDF8-803CCF30 | 805F94E0-805F9510 | | 8003A0BC-8003A0E0 (3) | 800198C8-800198E0 | `NetworkPeerBase` (ctor, `destroy`), `networkPeerError_*`, `networkSmallObject_dtor` |
| `Network/NetworkPeerBuffer.cpp` | 803CCF30-803CD248 | 805F9510-805F9540 | | 8003A0E0-8003A134 (7) | 800198E0-80019940 | `NetworkPeerBuffer` |
| `Network/NetworkPeerUdp.cpp` | 803CD248-803CD764 | 805F9540-805F9570 | .bss 806D2C60-806D3240 | 8003A134-8003A170 (5) | 80019940-80019968 | `NetworkPeerUdp` |
| `Network/NetworkPeerMcs.cpp` | 803CD764-803CE060 | 805F9570-805F9610 | .bss 806D3240-806D3650, .sbss 80794C98-80794CA0 | 8003A170-8003A1DC (9) | 80019968-800199B0 | `NetworkPeerMcs` |
| `Network/network_socket_streams.cpp` | 803CE060-803CF14C | 805F9610-805F9958 | | 8003A1DC-8003A2F0 (23) | 800199B0-80019A68 | `NetworkSingleTcp`/`NetworkMultipleUdp` users, `NetworkByteStream` methods, `NetworkResolverBase` |
| `Network/NetworkResolverWii.cpp` | 803CF14C-803CF654 | 805F9958-805F99A0 | | 8003A2F0-8003A338 (6) | 80019A68-80019AAC | `NetworkResolverWii`, thread entry, lookup |
| `Network/NetworkSessionBase.cpp` | 803CF654-803CF6D8 | 805F99A0-805F9A40 | | 8003A338-8003A344 (1) | 80019AAC-80019AB4 | `NetworkSessionBase` destructor (vtable key), mutex wrappers, `setNotifyValue` |
| `Network/NetworkSessionStable.cpp` | 803CF6D8-803D3CE8 | 805F9A40-805FA4EC (claim as before; the TU's data runs to 805FA788) | | 8003A344-8003A524 (40) | 80019AB4-80019E5C | the four `NetworkSessionBase` setters, nonce helpers, small-object dtors, the session state machine |

## Confidence per cut

| cut | that a TU boundary exists | where exactly |
| --- | --- | --- |
| left edge 0x803CCDF8 | unchanged, a discovery cap (the TU starts in 0x803CC7CC..0x803CCE54) | `networkSmallObject_dtor` stays with the Base TU: no evidence either way |
| Base / Buffer 0x803CCF30 | strong (zigzag 0x805F9510) | medium: interval 0x803CCE54..0x803CCF30; the error accessors stay with `NetworkPeerBase` |
| Buffer / Udp 0x803CD248 | strong (zigzag 0x805F9540) | medium: a class boundary (Buffer `destroy` ends there, Udp `setContext` starts); the Udp ctor is in another TU |
| Udp / Mcs 0x803CD764 | strong (V->S 0x805F9570) | medium-high: class boundary, Mcs `destroy` is its first function |
| Mcs / streams 0x803CE060 | strong (V->S 0x805F9610) | weak: the 4-byte `NetworkSingleTcp::disconnect` stub is placed with the Tcp members it is called beside |
| streams / ResolverWii 0x803CF14C | strong (V->S 0x805F9958) | medium: interval 0x803CF0A8..0x803CF170; `networkResolver_threadEntry` is address-taken only by `check` and calls `networkResolver_lookup`, which only it calls |
| ResolverWii / SessionBase 0x803CF654 | medium-strong (zigzag 0x805F99A0) | weak: interval 0x803CF3B8..0x803CF694; the four 8-byte accessors (mutex, `setNotifyValue`) are session-side and stay with the base |
| SessionBase / Stable 0x803CF6D8 | medium (strings after the last vtable, first referrer is `NetworkSessionStable_init`) | interval 0x803CF694..0x803CF7FC; `tudiscover at 0x803CF6F4` gives one certain match set 0x803CF6E4..0x803D3CE8 (60 functions) that includes `setHostTimeout`/`setSubhostTimeout`/`setRate`; `setLimits` (0x803CF6D8) is the same kind (writes globals only Stable reads, called only from Stable) and joins them; the destructor 0x803CF694 is the vtable key function and stays in the base. The cut was 0x803CF730 before this re-cut; `setRate` carries an extab record (0x80019AB4, 8 B) and one extabindex row, both moved with it |

Not split: `network_socket_streams` might hide one more cut at `NetworkResolverBase`'s constructor (0x803CF0A8), but
the `.data` there is `S* V` with no seam, and nothing else separates the socket users from `NetworkResolverBase`.
The state machine (0x803CF6D8..0x803D3CE8) has no interior seam: its strings, jump table and vtables are one run.

## Result (2026-09-29)

Split executed as one commit; the whole `report.json` was compared function by function before and after: 94
function rows unchanged (0 lower, 0 higher), `matched_code` 619,028 both, no other unit moved, `ninja -k 0` 0 FAILED,
`ninja build/RMHE08/ok` green, DOL sha1 unchanged.  Matched data 26,876 -> 28,956 B.

| unit | rows at 100 % | unit % | `.data` (datagap) | flipcheck |
| --- | --- | --- | --- | --- |
| `NetworkPeerBase` | 6 of 6 | 100.00 | exact | READY |
| `NetworkPeerBuffer` | 9 of 10 | 99.95 | exact | `extab` 0x38 of 0x60, 3 B of `.text` (relocations) |
| `NetworkPeerUdp` | 5 of 9 | 99.73 | exact | 33 B of `.text` (relocations) |
| `NetworkPeerMcs` | 6 of 10 | 99.79 | exact | 36 B of `.text`; `.sbss` 4 of 8 B |
| `network_socket_streams` | 33 of 35 | 98.45 | exact | `.text` 4 B short, `takeRecord`/`receive` residuals |
| `NetworkResolverWii` | 7 of 8 | 98.70 | exact | `check` residual |
| `NetworkSessionBase` | 4 of 6 | 69.70 | exact | constructor and `getSomething5` unwritten |
| `NetworkSessionStable` | 11 of 61 | 3.83 | 0 of 2732 B (unwritten); 8 B `.sdata2` ours-extra (`setRate`'s constant) | state machine unwritten |

Re-cut of the SessionBase / Stable edge (0x803CF730 -> 0x803CF6D8): the whole report compared again, 20,509 rows
identical, DOL sha1 unchanged, `ninja -k 0` 0 FAILED; only the two units' own percentages moved (Base 81.82 -> 69.70,
Stable 3.36 -> 3.83) because four matched setters changed unit.

Before, the combined unit's `.data` was 10.4 % of 4108 B (1376 B emitted, out of order); each fragment is now in
its own TU, so the seven units that emit data match it byte for byte in size and order.  Functions are emitted in
address order in each file (the source's old definition order was a permutation of the target's layout).
