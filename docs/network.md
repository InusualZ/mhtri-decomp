# The Network units

The `src/Network/` units: how the transport band was split, the evidence behind each unit's flags, the folds and
recuts, and the symbols renamed on the way.

## Transport split

The former `Network/network_transport` is eight translation units.  Survey for the split of
`src/Network/network_transport.cpp` (`.text` 0x803CCDF8..0x803D3CE8, `.data`
0x805F94E0..0x805FA4EC). Method: `docs/data-order-seams.md` (a TU's `.data` is globals, out-of-line strings,
vtables in reverse class order, inline strings), `callers.py` for every `.data`/`.bss`/`.sbss` referrer, the
`extabindex` entry order of the target object (one entry per function, so it tiles by TU), and
`tudiscover.py at` for the `.text` intervals each seam pins. `datagap.py` independently reports "at least 8 TUs".

### Evidence

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
| 0x805F9A40.. (41 strings, a jump table, the `NetworkUnitPacket`/`NetworkSessionStable` vtables at 0x805FA6A8/0x805FA6E8) | state machine | first referrer `NetworkSessionStable::init` 0x803CFA44, none earlier | strings after the last vtable, all referrers in one function run: the next TU. Its data runs to 0x805FA788 (a V->S seam into `Network/NetworkSessionManager.cpp`'s TU) |

`.bss` (`networkUdpPacketBuffer` 0x806D2C60 read by the Udp peer, `networkMcsPacketBuffer` 0x806D3240 read by the
Mcs peer) and `.sbss` (`networkMcsRetryTime` 0x80794C98, Mcs `move`) follow the same order. `.sdata`/`.sdata2` are
owned by `Network/network_shared_data.cpp` and are not part of the split.

### Split table

| unit (GUESS names) | `.text` | `.data` | `.bss` / `.sbss` | `extabindex` (entries) | `extab` | classes / functions |
| --- | --- | --- | --- | --- | --- | --- |
| `Network/NetworkPeerBase.cpp` | 803CCDF8-803CCF30 | 805F94E0-805F9510 | | 8003A0BC-8003A0E0 (3) | 800198C8-800198E0 | `NetworkPeerBase` (ctor, `destroy`), `networkPeerError_*`, `networkSmallObject_dtor` |
| `Network/NetworkPeerBuffer.cpp` | 803CCF30-803CD248 | 805F9510-805F9540 | | 8003A0E0-8003A134 (7) | 800198E0-80019940 | `NetworkPeerBuffer` |
| `Network/NetworkPeerUdp.cpp` | 803CD248-803CD764 | 805F9540-805F9570 | .bss 806D2C60-806D3240 | 8003A134-8003A170 (5) | 80019940-80019968 | `NetworkPeerUdp` |
| `Network/NetworkPeerMcs.cpp` | 803CD764-803CE060 | 805F9570-805F9610 | .bss 806D3240-806D3650, .sbss 80794C98-80794CA0 | 8003A170-8003A1DC (9) | 80019968-800199B0 | `NetworkPeerMcs` |
| `Network/network_socket_streams.cpp` | 803CE060-803CF14C | 805F9610-805F9958 | | 8003A1DC-8003A2F0 (23) | 800199B0-80019A68 | `NetworkSingleTcp`/`NetworkMultipleUdp` users, `NetworkByteStream` methods, `NetworkResolverBase` |
| `Network/NetworkResolverWii.cpp` | 803CF14C-803CF654 | 805F9958-805F99A0 | | 8003A2F0-8003A338 (6) | 80019A68-80019AAC | `NetworkResolverWii`, thread entry, lookup |
| `Network/NetworkSessionBase.cpp` | 803CF654-803CF6D8 | 805F99A0-805F9A40 | | 8003A338-8003A344 (1) | 80019AAC-80019AB4 | `NetworkSessionBase` destructor (vtable key), mutex wrappers, `setNotifyValue` |
| `Network/NetworkSessionStable.cpp` | 803CF6D8-803D4904 | 805F9A40-805FA788 | | 8003A344-8003A5A8 (51) | 80019AB4-80019F3C | the four `NetworkSessionBase` setters, nonce helpers, small-object dtors, the session state machine |

### Confidence per cut

| cut | that a TU boundary exists | where exactly |
| --- | --- | --- |
| left edge 0x803CCDF8 | unchanged, a discovery cap (the TU starts in 0x803CC7CC..0x803CCE54) | `networkSmallObject_dtor` stays with the Base TU: no evidence either way |
| Base / Buffer 0x803CCF30 | strong (zigzag 0x805F9510) | medium: interval 0x803CCE54..0x803CCF30; the error accessors stay with `NetworkPeerBase` |
| Buffer / Udp 0x803CD248 | strong (zigzag 0x805F9540) | medium: a class boundary (Buffer `destroy` ends there, Udp `setContext` starts); the Udp ctor is in another TU |
| Udp / Mcs 0x803CD764 | strong (V->S 0x805F9570) | medium-high: class boundary, Mcs `destroy` is its first function |
| Mcs / streams 0x803CE060 | strong (V->S 0x805F9610) | weak: the 4-byte `NetworkSingleTcp::disconnect` stub is placed with the Tcp members it is called beside |
| streams / ResolverWii 0x803CF14C | strong (V->S 0x805F9958) | medium: interval 0x803CF0A8..0x803CF170; `networkResolver_threadEntry` is address-taken only by `check` and calls `networkResolver_lookup`, which only it calls |
| ResolverWii / SessionBase 0x803CF654 | medium-strong (zigzag 0x805F99A0) | weak: interval 0x803CF3B8..0x803CF694; the four 8-byte accessors (mutex, `setNotifyValue`) are session-side and stay with the base |
| SessionBase / Stable 0x803CF6D8 | medium (strings after the last vtable, first referrer is `NetworkSessionStable::init`) | interval 0x803CF694..0x803CF7FC; `tudiscover at 0x803CF6F4` gives one certain match set 0x803CF6E4..0x803D3CE8 (60 functions) that includes `setHostTimeout`/`setSubhostTimeout`/`setRate`; `setLimits` (0x803CF6D8) is the same kind (writes globals only Stable reads, called only from Stable) and joins them; the destructor 0x803CF694 is the vtable key function and stays in the base. The cut was 0x803CF730 before this re-cut; `setRate` carries an extab record (0x80019AB4, 8 B) and one extabindex row, both moved with it |

Not split: `network_socket_streams` might hide one more cut at `NetworkResolverBase`'s constructor (0x803CF0A8), but
the `.data` there is `S* V` with no seam, and nothing else separates the socket users from `NetworkResolverBase`.
The state machine (0x803CF6D8..0x803D4904; the twelve op-code writers and rate functions at 0x803D3CE8..0x803D4904 joined it once the `.data` order showed their strings before the tables) has no interior seam: its strings, jump table and vtables are one run.

### Result (2026-09-29)

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

## Flag evidence

Each `configure.py` row of a `src/Network/` unit carries one pointer line (`# Flags: unit header of ...; measurements
in docs/network.md`): the unit header's FLAGS line names the flags, and the measurements behind them are here, one
bullet per row (function `match_percent` over the same source, flag against flag).

* **The lib group `cflags_network`** (`cflags_base` + `-func_align 4 -Cpp_exceptions on`).  `-func_align 4`: the retail
  objects (`Network/NetworkWiiMediator.cpp` - then the 20-byte `Network/NetworkWiiMediator.c` of the worker/net-capcom
  batch - and `OS/OSAlarm.c`, whose `cflags_os` carries the same flag) are `.text align 2**2`, while `cflags_base`'s
  `-O4,p` implies `-func_align 16`, which our objects emitted.  Flipping NetworkWiiMediator with that alignment made the
  linker round the object's start up to the next 16-byte boundary: `dtk dol diff` reported fn_80413F3C expected at
  0x80413F3C but found at 0x80413F40, every following symbol shifted by 4, and main.dol stopped matching build.sha1;
  section sizes were already identical, so the alignment was the entire difference (the same finding as
  `cflags_ppceabi`).  `-Cpp_exceptions on` (flags-audit 2026-09-28): every one of the lib's 7 registered objects then
  carried extab/extabindex in the target (24 B .. 1260 B), and 7 of 7 spelled it out as a file-wide
  `#pragma exceptions on`; with the lib flag on and all 7 pragmas deleted, all 7 objects' allocatable sections were
  byte-identical, every unit score identical, and main.dol still BF4850739478CAAEDFE675949EB7C28595A7FDE9.  The
  pragmas were the symptom of the lib default (`cflags_base`'s `off`) disagreeing with the lib's own bytes.
* **Transport band** (`NetworkPeerBase`, `NetworkPeerBuffer`, `NetworkPeerUdp`, `NetworkPeerMcs`,
  `network_socket_streams`, `NetworkResolverWii`, `NetworkSessionBase`, `NetworkSessionStable`): `-O3 -pool off`.
  `-O3`: on the combined unit the lib's `-O4,p` put 14 rows at 100 % (unit 4.05 %), `-O3` 37 of 38 (unit 7.37 %).
  `-pool off` (playbook 43): the target materialises each log string with its own `lis`/`addi` pair
  (`NetworkMultipleUdp::receive`: `lis r4,lbl_805F988C@ha` / `addi r4,r4,lbl_805F988C@l`, then
  `lis r4,lbl_805F98C4@ha`), where pooling addresses every string of a function through one `@stringBase0` register.
  `-pool` (not `-str`) is the lever: with it on, a function referencing three or more distinct `@NNN` strings
  addresses the second off the first's base (`addi r28,r5,<first>` then `addi r4,r28,<delta>`).  It moved one row
  (`NetworkMultipleUdp_receive`, now `NetworkMultipleUdp::receive`, 89.53 -> 94.06) and lowered none; it is a no-op
  on the other seven objects, which are TUs of the same retail family, so all eight rows carry it.
* **`NetworkSessionManager`**: `-O3` (`-func_align 4` kept: the 4-byte `NetworkRequest_clear` 0x803D4B5C and
  `NetworkRequest_isCancelled` 0x803D5D64).  Every framed function is `-O3` scheduling: `request376` (0x803D53B0)
  opens `stw r31,28; stw r30,24; mr r30,r3; mr r31,r4` and copies the global descriptor with a plain `lwz/stw`
  block, both of which `-O4,p` destroys (it interleaves the saves and folds the copy into `lwzu`).  Measured:
  0x803D53B0 59.79 -> 95.15, the constructor 0x803D4904 70.42 -> 93.24, `NetworkSessionManager` 81.63 -> 92.23.
* **`NetworkSessionManagerPat`** (the game-root `main` lib, `cflags_main`): `initNetworkSessionStable` (0x803DEA30)
  scored 81.39 at `-O4,p` and 100.00 at `-O3`; its 264-byte `.text`, `extab` 0x18 and `extabindex` 0xC match with
  all ten relocations (`__nw__FUl` at .text+0x34, `constructNetworkSessionObject` at +0x44,
  `networkSessionReflectCallback` at +0x66/+0x6A, the three float constants at +0x8C/+0xA4/+0xBC; `__dl__FPv` at
  extab+0x14; `initNetworkSessionStable` and `@etb_8001A630` in extabindex).  `-pool off` moves no row (94/106 at
  100 either way), so the row takes the lib's flags.
* **`NetworkLayer`** (the `main` lib, `cflags_main` + `-pool off`): the layer id helpers address each of their three
  warning strings with their own `lis`/`addi`; with the flag `NetworkLayerIdImportFrom` 82.13 -> 100.00 and
  `NetworkLayerIdExportTo` 83.05 -> 100.00, no other row moves.  The Network lib's flags score every row identically.
* **`NetworkLayerPat`**: `-O3 -inline noauto`.  `stepRequest` (0x803E44C8) scores 90.14167 at the lib's `-O4,p` and
  100.00000 at `-O3`; with `-inline auto` the member `getRecord` (0x803E3598, the same TU) is inlined into it (90.74
  against 100.00; retail calls it).
* **`NetworkCommunity`**: `-O3 -inline noauto`.  Over the unit's 27 rows (peephole off) the lib's
  `-O4,p -inline auto` scores 67.22 % (5/27 at 100) against 99.97 % (26/27).
* **`NetworkCommunityPat`**: the same flags.  Over 87 rows `-O4,p -inline auto` scores 84.24 % (4/87 at 100) against
  99.97 % (84/87).
* **`PatConnection`**: `-O3 -inline noauto`, the `PatInterface` row's flags.  Over the unit's 37 written rows the
  lib's `-O4,p -inline auto` scores 76.78 % (6/37 at 100) against 96.48 % (20/37) before any source tuning:
  `-inline auto` folds the 4- and 8-byte tail branches `writeBool`/`writeInt16`/`writeUInt32Shared` and the reader
  wrappers into copies of their callees, and `-O4,p` hoists the constant setup above the callee-save stores.  The
  file-scope pragmas (`peephole off`, `pool_data off`) are in the unit header; with them every row is 100 % and a
  trial `Matching` link keeps main.dol's SHA-1.
* **`NetworkStreamSink`**: `-O3 -inline noauto -pool off`, the transport band's flags plus `noauto`.  Over the buffer
  class's 15 rows the lib's `-O4,p -inline auto` schedules the stores out of source order (the constructor 66.88,
  `attach` 45.43) and inlines the 8-byte `getNetworkLogger` into its callers (`fill`/`put`/`copyFrom` 96.3, a `lwz` of
  the singleton where retail calls); `-O3 -pool off` alone left those three at 96.3, and with `-inline noauto` and the
  file's `peephole off` every written row is 100 %.
* **`PatInterface`** (with the former `network_state` and the head of the former `network_layer_io`):
  `-O3 -inline noauto`.  At the lib default `setTermVersion`/`setAnnounceBuffer`/`increment60d4` schedule their
  stores out of source order (60.00/77.78/80.00, 100 at `-O3`), and every framed function hoists its
  constant-argument setup into the prologue's `mflr`->`stw` slot (`sendReqCommonKey`: the target is
  `stw r0,20; stw r31,12; stw r30,8; mr r30,r3; li r4,18; li r5,0; bl` while `-O4,p` puts the two `li` before the
  saves) with the FMP/request switch tails laid out unsorted: the `network_state` part 69.42 -> 81.93 at `-O3`
  (five more rows at 100), `recvReqLineCheck`/`recvAnsServerTime`/`recvAnsShut` 82.13/78.25/72.67 -> 100.
  `-inline auto` folds `getServerTime`/`getGameTime`/`getFmpSize`/`setSomething` into their callers
  (`getServerDateTime` 53.71, `copyFmpSlot` 82.18, `setPatByte6138On` 17.50; 100 at `noauto`) and would fold the
  accessors into the packet layer's callers, which retail calls.
* **`NetworkPool`**: the `network_layer_io` row's `-O3` plus `-inline noauto` (`onNANDCheckDone`, 0x80412424, calls
  the 2-instruction `getNetworkPool` defined before it); unmeasured until a body lands.
* **`NetworkWiiMediator`** (with the former `network_opening`): `-O3 -inline noauto`.  Over the mediator's 79 rows
  the lib's `-O4,p` leaves `getAccountBan`/`Warning`/`WaitQueue` at 42.86 and `getReflectName3C` at 51.28 where
  `-O3` puts all four at 100.00, and `-inline auto` scores `validateReflectName` 0.00 against 88.28 at `noauto`; no
  other row moves.  The opening part: at `-O4,p` every framed body hoists its setup above the callee-save stores
  (`openingStart` 85.88, `openTransferSlot` 70.12, `clearTransferQueue` 65.25, `pushTransferRecord` 73.76,
  `popTransferRecord` 78.74 against 100/100/100/98.71/97.19 at `-O3`); its file-scope `#pragma auto_inline off`
  already was `-inline noauto` for its part.
* **`sNetworkLibrary`**: `-O3`: every row scored 77-93 % at `-O4,p` and 100 % at `-O3` except `init`, which the
  file-scope `#pragma pool_data off` takes from 94.04 to 100.
* **`sNetworkLibraryWii`**: `-O3 -inline noauto`.  75.54058 % at `-O4,p` (every framed body hoists its constant setup
  above the callee-save stores) against 91.81757 % at `-O3`, every row equal or higher; the folded
  `constructNetworkWiiMediator` (0x80418988) 75.00000 -> 100.00000 (`li r3,0x1408` after the callee-save stores).
  `-inline noauto`: retail's `initNetworkLibrary` calls `constructNetworkWiiMediator`; `-inline auto` folds it and
  the inlined constructor in (256 B against 120 B, 100.00 -> 0.00); with `noauto` no row moves.
* **`NetworkPat`**: `-O3 -inline noauto`.  The three `deleteNetwork*Pat` helpers get the target's block order and its
  `bl` to the sibling `clearNetwork*Pat` (`-inline auto` folds the 36-byte callee and costs 4 bytes per helper);
  2.85714 at the lib's flags, 99.71429 at `-O3`/`-inline noauto` (140 B, the target size) on the first bodies.
* **`NetworkReflectService`**: `-O3 -inline noauto`.  Over the 13 written rows the lib's flags give 68.95 %, `-O3`
  81.77 % (`reflectServiceStart` then inlines `resetReflectServiceTask`, which retail calls: 40.54 -> 30.54), and
  `-O3 -inline noauto` puts 12 at 100 %.
* **`GameSpyInterfaceThread`**: `-O3 -inline noauto -pool off`.  Retail calls the small helpers from the state
  machines: `isQueued` (0x8041DCDC) is `lwz r3,0x6634; lwz r4,0x6638; bl getSlotState; extsb` (72 B) while
  `-inline auto` folds the 128-byte callee (156 B), and 0x8041DD58/0x8041CA94 grow the same way.  `-pool off`: with
  the log strings the TU's own literals, pooling addresses them off one `...data.0` base
  (`lis r5,...data.0@ha ; addi r30,r5,...@l`, then `addi r4,r30,0x218`) where the target gives each string its own
  pair (`lis r4,lbl_806033C8@ha ; addi r4,r4,lbl_806033C8@l`): 99.87 % / 64 rows at 100 with the flag, 98.21 % / 59
  without (`natNegCompletedCallback` 89.78, `gt2ConnectAttemptCallback` 88.74, `runNasLogin` 92.46, `startMatch`
  84.70 lower, none higher).
* **`network_shared_data`**: data only (`.sdata2` 0x8079C690-0x8079C758, `.sdata` 0x80793900-0x80793930), no code
  and no flag; the model is `Pl/pl_frame_data.cpp` (playbook 23/53 route 2, playbook 54).

## SDK library compilers

The NWC24, NHTTP, SSL, NCD and SO libraries carry the RVL SDK build string `0x4199_60831`, the build of
`GC/3.0a5.2` (the compiler survey over 96 units / 13 compilers ranked it first for these libraries); DWCi carries the
DWC build `0x4302_145` and stays on `Wii/1.3`. The libraries keep their flags (`-O4,p -inline auto`, `-func_align 4`).
Measured with each lib's own cflags, same sources, whole report compared row by row:

| lib | units | Wii/1.3 | GC/3.0a5.2 | rows better / worse |
| --- | --- | --- | --- | --- |
| NWC24 | `nwc24_io`, `nwc24_msg` | 80.69 % / 89.96 %, 18 rows at 100 | 81.54 % / 91.28 %, 21 rows at 100 | 5 / 0 |
| NHTTP | `NHTTP_bgnend`, `NHTTP_os_RVL`, `d_nhttp` | 97.62 % / 63.09 % / 93.56 % | 98.27 % / 63.09 % / 97.00 % | 42 / 0 |

Under GC/3.0a5.2 two `d_nhttp` rows first came out lower (`NHTTPi_findHeaderField` 95.08 -> 94.81,
`NHTTPi_strToHex` 81.16 -> 79.71); the source shapes the GC compiler wants fix both: `strToHex` tests the first digit
as `*s > '7'` and initialises its accumulator after the range checks (100.00), `findHeaderField` declares `colon`
before `flag` and `line` before `next` (95.39).

DWCi function packing: the retail `dwc_error`, `DWCi_Np_CPUCopyFast` and `dwc_nasfunc` objects start every function on
16 bytes (`gap_*` words between them). Measured `cflags_base` (`-func_align 16`) against the lib's `cflags_dwc`
(`-func_align 4`): `dwc_error` and `DWCi_Np_CPUCopyFast` move no row (0 better / 0 worse; `dwc_error`'s `.text` is
then 2168 of 2176 B, the rest the object's tail pad), so both take `cflags_base`; `dwc_nasfunc` scores 2 rows
better (`gti2CheckResponse` 98.00 -> 100, `DWCi_socketLookupHost` 83.74 -> 84.54) and 1 worse (`DWC_SVLProcess`
78.69 -> 77.28, its member-wise copy loop gains an alignment `nop`), so it stays on `cflags_dwc`.

## Folds and recuts

Earlier unit names cited by the playbook and the notes, and where their ranges live now.

| former unit | range then | now in |
| --- | --- | --- |
| `Network/network_state.cpp` | `.text` 0x803FE8E4..0x804006A8 (8900 B, 21 functions) | `Network/PatInterface.cpp` |
| `Network/network_layer_io.cpp` | `.text` 0x803FCC34..0x80413450 | head (..0x804123F8) `Network/PatInterface.cpp`; tail (`NetworkPool`, the NHTTP wrappers, `NetworkRandom`) `Network/NetworkPool.cpp` |
| `Network/NetworkWiiMediator.cpp` (first cut) | `.text` 0x80413C64..0x804155D4 (6490 B, 79 functions; the left edge was the band's only two-signal cut) | `Network/NetworkWiiMediator.cpp` (0x80413450..0x80417BC0) |
| `Network/NetworkWiiMediator.c` | `getReflectPageBuffer` (0x80413F3C, 20 B) | `Network/NetworkWiiMediator.cpp` |
| `Network/network_opening.cpp` | `.text` 0x804155D4..0x80417BC0 | `Network/NetworkWiiMediator.cpp` |
| `Network/constructNetworkWiiMediator.cpp` | `.text` 0x80418988..0x804189C8 (64 B) | `Network/sNetworkLibraryWii.cpp` |
| `Network/initNetworkSessionStable.cpp` | `.text` 0x803DEA30..0x803DEB38 (264 B) | `Network/NetworkSessionManagerPat.cpp` |
| dtk's `auto_03_8041A170_text` | `clearNetworkLayerPat` (0x8041A170), orphaned while `Network/NetworkPat.cpp` ended at 0x8041A170 | `Network/NetworkPat.cpp` (now ends at 0x8041A194) |
| `constructNetworkLibrary.cpp` (the map stem of 0x804189C8) | the `sNetworkLibraryWii` class | `Network/sNetworkLibraryWii.cpp` |
| `Network/NetworkLayerPatStep.cpp` | `stepRequest` (0x803E44C8) | `Network/NetworkLayerPat.cpp` |
| `fn_80423E74.cpp` (the work-record / PatCamellia band) | `.text` 0x80423E74..0x80429B94 (23840 B, 77 functions) | `Network/network_pat_control.cpp` |
| the pat-control proposal at 0x80429B94 | `.text` 0x80429B94..0x8043065C (114 functions, 27336 B) | `Network/network_pat_control.cpp` (0x80423E74..0x80432104) |
| the GameSpy proposal at 0x8041A87C | `.text` 0x8041A87C..0x8041DF10 (71 functions, 13972 B) | `Network/GameSpyInterfaceThread.cpp` (0x8041B194..) and `Network/NetworkReflectService.cpp` |
| the tail of `lobby/lb_server_sel_trans.cpp` | `.text` 0x803C987C..0x803CCDF8 (74 functions, 13692 B), its extab/extabindex, `.rodata` and `.data` 0x805F8DB0..0x805F94E0 | `Network/NetworkStreamSink.cpp` (seam evidence in its header) |
| the VF/DB/PMIC/KPR/HID/KBD half of `SO/soi.cpp` | `.text` 0x80521340..0x8052A040 (174 functions) | `VF/vf.cpp`; `SO/soi.cpp` keeps 0x8051E864..0x80521340 |
| the NCD/NET half of `SSL/ssl.cpp` | `.text` 0x8051C554..0x8051D710 (14 functions) | `NCD/ncdsystem.c`; `SSL/ssl.cpp` keeps 0x8051B7FC..0x8051C554 |

## Header layout (2026-10-06)

Each transport class is declared in its owner unit's header; the former shared headers are gone.

| class / declarations | header |
| --- | --- |
| `NetworkPeerBase`, the error record and `NetworkPeerErrorSource` | `Network/NetworkPeerBase.h` |
| `NetworkPeerBuffer` / `NetworkPeerUdp` / `NetworkPeerMcs` (+ `NetworkPeerInfo`) | `Network/NetworkPeerBuffer.h` / `NetworkPeerUdp.h` / `NetworkPeerMcs.h` |
| the socket handle, `NetworkSingleTcp`, `NetworkMultipleUdp`, `NetworkByteStream`, `NetworkResolverBase` | `Network/network_socket_streams.h` |
| `NetworkSessionBase` / `NetworkResolverWii` | `Network/NetworkSessionBase.h` / `Network/NetworkResolverWii.h` |
| `NetworkStreamSink`, `NetworkStreamWriter(Default)`, `NetworkBuffer`, the logger view, `getNetworkLogger`, the mutex pair, `networkStreamWriter_attach`/`_reserve` | `Network/NetworkStreamSink.h` |
| `NetworkStreamQueue` | `Network/NetworkUnitPacket.h` |
| the GT2 API and the DWCi socket/list layer | `DWCi/dwc_nasfunc.h` |
| the OS mutex/thread calls | `NAND/nand.h` (one spelling; `unsplit/OS.h` includes it) |

`Network/network_transport.h`, `network_transport_types.h`, `network_writer_types.h` and `sGameSpyInterfaceThread.h`
were folded into these; `Network/gamespy_interface_types.h` stays a type-only header, because folding it into
`Network/GameSpyInterfaceThread.h` makes `PatInterface.h` and `GameSpyInterfaceThread.h` include each other and
`PatConnection.cpp` stops compiling.

## Renamed symbols

Spellings the notes and older unit headers used, and the map's current names.

| former spelling | now |
| --- | --- |
| `sendFriendRequest` / `acceptFriendRequest` (the consumer's spellings of the BlackAdd/BlackDelete starters) | `blockPlayer` / `unblockPlayer` |
| `NetworkLayer_VTable` | `__vt__12NetworkLayer` (0x805FB5D0) |
| `handleCircleMatchEndInfo` | `handleCircleLeave` |
| `updateTransferQueue` | `updateFriendTransferModes` |
| `networkPeer_getSocket` / `networkPeer_getPeerId` | `NetworkByteStream::getData` / `getSize` |
| `constructNetworkLibrary` | `__ct__18sNetworkLibraryWiiFv` (0x804189C8) |
| `getGameSpyInterfaceThread`, then `GameSpyInterfaceThread_getInstance` | `getInstance__22GameSpyInterfaceThreadFv` (0x803D6A98) |
| `clearPatInterface` / `isPatInterfaceReady` | `PatInterface_clear` (0x803FD2D8) / `PatInterface_isReady` (0x803FD644) |
| `constructReflectService` | `__ct__21NetworkReflectServiceFv` (0x8041A1C4) |
| `create__22GameSpyInterfaceThreadFv` | `__ct__22GameSpyInterfaceThreadFv` (0x8041C66C) |
| `reflectInit__18NetworkWiiMediatorPFllllPvPv_vPv` (a misspelling the mediator header carried) | `reflectInit__18NetworkWiiMediatorFPFllllPvPv_vPv` |
| `dtor_803CA338` (and the stale `fn_803CA338`) | `networkInstance_destroyMutex` (0x803CA338) |
| `fn_80526F00` / `fn_80529430` / `fn_80529B50` | `KPRLookAhead` / `KBDSetLedsAsync` / `KBDSetChannelValue` (GUESSes, `VF/vf.cpp`) |

The four constructor/accessor rows were renamed twice, the second time to the owners' definition spellings.
