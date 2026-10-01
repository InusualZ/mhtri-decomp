# Phase 1, band `e`: `.text` 0x80380000 .. 0x80460000

Proposal file: `docs/splits/proposals/phase1-e.json` (format: `docs/splits-program.md`, plus band `a`'s extension keys
`replaces_tail_of`, `absorbs`, `removes_cuts`, top-level `open_questions`). Tree `15dc2cf7d`. The band holds 3,594 functions: 37
registered units and **2,019 unowned functions in 13 runs** (0x803C4BA0..0x803CCDF8, 0x803D72F4..0x803DEA30, ..., 0x804578FC..0x80460000).

## Counts

| | |
| --- | --- |
| proposal units in the file / after the renderer folds the `guess` ones | 26 / 23 |
| cuts emitted | 26: **strong 8**, **medium 5**, **guess 13** (3 guess cuts fold a unit away, 10 are registry edges kept as they are) |
| of the 13 strong+medium cuts: tails cut off a registered unit | 4 (`0x80383148`, `0x80385EE0`, `0x803B936C`, `0x803C3A5C`) |
| of them: new cuts inside unowned runs | 6 (`0x803F73C0`, `0x803FCC34`, `0x80432104`, `0x80437270`, `0x8043D524`, `0x80456300`) |
| of them: a registered edge the evidence confirms / moves | 2 confirmed (`0x803AB3BC`, `0x80448404`: the sinit closure lands exactly on it) + 1 moved (`arenatask`'s start 0x804459E4 -> `0x804459DC`) |
| registered cuts removed (`removes_cuts`) | 7: **strong 1** (`0x803DEA30`, a **Matching** unit, see below), **medium 6** |
| registered units recut (a tail cut off) | 3: `enemy/fn_80382310` (2 tails), `enemy/em_pop`, `menu/get_pop_dat_ptr` |
| registered units merged | 6 -> 3: `quest_entry + arena_result`, `NetworkSessionManagerPat + initNetworkSessionStable` (Matching), `fn_80423E74 + network_pat_control`; `arenatask` extended by 8 B |
| unowned functions covered / left open | **2,018 of 2,019 covered** (0x73CAC of 0x74FD0 B); 1 left: `fn_8045F9E8` (0x1324 B) crosses the band end, handed to band `f` |
| candidate cuts kept as `open_questions` (never applied) | 35 rows: 25 generated from the constraint clusters (each says "a TU boundary lies in [a, b], n legal positions") + 10 notes (cross-band fold, Network pool group, the MSL file layout, ...) |

At least **25 more TU boundaries** exist in the band (greedy hitting set over the unsatisfied `pooldup`/`.data`/`__FILE__`
intervals after the cuts above), none of them locatable to fewer than 4 positions: that is why the rendered units are large
(`menu/menu_plsearch` 57 KB, `Network/network_layer_io` 80 KB, `Network/NetworkCommunityPat` 76 KB).

## Grade rubric (the same as band `a`)

* **strong**: an exact invariant plus an independent soft interval that holds the cut and no other known cut.
* **medium**: the exact invariant alone, or one narrow soft interval (at most 3 positions).
* **guess**: anything else; never applied. A `guess` cut folds its unit into the abutting **proposal** unit; next to a
  registered unit (a registry edge nothing contradicts) the edge is kept (`splitcheck.py` fix, below).

The exact invariants used: (1) a `.ctors` word's TU ends at `L`, the end of the closure of the sinit's calls, branches and
address-taken functions that lie after it (`callers.py` shows each step), and `L` is referenced by nothing before it; (2) a
pooled literal read on both sides of a cut puts the cut inside one TU (`removes_cuts`); (3) a `__FILE__` string's readers are
one TU. Soft intervals: `pooldup` (one value at two pool addresses), `.data` V->S / zigzag seams, source-file changes.

## Units proposed / recut / merged

| unit (derived) | `.text` range | left cut | grade | note |
| --- | --- | --- | --- | --- |
| `enemy/fn_80383148` | 0x80383148..0x80385EE0 | 0x80383148 | strong | tail of `enemy/fn_80382310.cpp`; placeholder name |
| `enemy/fn_80385EE0` | 0x80385EE0..0x803868DC | 0x80385EE0 | strong | tail of `enemy/fn_80382310.cpp`; placeholder name |
| `quest/quest_entry` | 0x803AB3BC..0x803B465C | 0x803AB3BC | strong | folds `quest/quest_entry.cpp` + `menu/arena_result.cpp` (pooled 50.0f / 60.0f read on both sides); removes 0x803B0F98 |
| `enemy/em_roster` | 0x803B936C..0x803BA6F0 | 0x803B936C | strong | tail of `enemy/em_pop.cpp`; the next row folds into it (rendered as `enemy/em_model`, the larger half) |
| `enemy/em_model` | 0x803BA6F0..0x803BE30C | 0x803BA6F0 | guess | roster / model split, merged away by the renderer |
| `menu/demo_work_clear` | 0x803C3A5C..0x803C4BA0 | 0x803C3A5C | medium | tail of `menu/get_pop_dat_ptr.cpp`; the next row folds into it |
| `lobby/lb_server_sel_trans` | 0x803C4BA0..0x803CCDF8 | 0x803C4BA0 | guess | unowned run; its `guess` left edge (the old discovery cap) folds it into the row above |
| `Network/NetworkSessionManagerPat` | 0x803D70B8..0x803DF5FC | 0x803D70B8 | guess | absorbs the registered Pat unit and the registered **Matching** `initNetworkSessionStable.cpp`; removes 0x803D72F4 (medium), 0x803DEA30 (strong), 0x803DEB38 (medium) |
| `Network/NetworkLayerPat` | 0x803DF5FC..0x803E44C8 | 0x803DF5FC | guess | Pat / NetworkLayer split in 0x803DF5FC..0x803E0C18, merged into the row above |
| `Network/NetworkCommunityPat` | 0x803E4888..0x803F73C0 | 0x803E4888 | guess | unowned run, registry edge |
| `Network/network_packet` | 0x803F73C0..0x803FCC34 | 0x803F73C0 | medium | width-1 zigzag `.data` pin |
| `Network/PatInterface` | 0x803FCC34..0x803FE8E4 | 0x803FCC34 | strong | ctors closure empty + V->S interval ends at the cut |
| `Network/network_layer_io` | 0x804006A8..0x80413C64 | 0x804006A8 | guess | 415 functions, no pool or `.data` constraint at all |
| `Network/network_opening` | 0x804155D4..0x80418988 | 0x804155D4 | guess | unowned run, registry edge |
| `Network/constructNetworkLibrary` | 0x804189C8..0x80419EC4 | 0x804189C8 | guess | unowned run, registry edge |
| `Network/NetworkReflectService` | 0x8041A194..0x8041A87C | 0x8041A194 | guess | unowned run, registry edge |
| `Network/network_pat_control` | 0x80423E74..0x80432104 | 0x80423E74 | guess | absorbs `fn_80423E74.cpp` + `Network/network_pat_control.cpp` and the unowned head 0x8043065C..0x80432104 (the unsigned->double magic is read by all three); removes 0x80429B94, 0x8043065C |
| `Network/net_session_close` | 0x80432104..0x80437270 | 0x80432104 | medium | ctors closure of the sinit at 0x80431CD8 (9 functions) |
| `menu/menu_placeinfo` | 0x80437270..0x8043D524 | 0x80437270 | medium | ctors closure; name from the `__FILE__` anchor |
| `menu/movie` | 0x8043D524..0x804459DC | 0x8043D524 | strong | ctors closure + `__FILE__` anchor `movie.cpp` starts at the cut |
| `quest/arenatask` | 0x804459DC..0x80448404 | 0x804459DC | strong | extends `quest/arenatask.cpp` by the 8-byte accessor of its own `.sbss` object; moves 0x804459E4 to 0x804459DC |
| `menu/menu_plsearch` | 0x80448404..0x80456300 | 0x80448404 | strong | closure lands exactly on the registered end of `quest/arenatask.cpp` (confirms that edge) |
| `main/fn_80456300` | 0x80456300..0x804566A4 | 0x80456300 | medium | ctors closure (4 functions); placeholder name |
| `Runtime.PPCEABI.H/CPlusLibPPC` | 0x80456704..0x80456C88 | 0x80456704 | guess | unowned run, registry edge |
| `Runtime.PPCEABI.H/runtime` | 0x80456CE0..0x80457420 | 0x80456CE0 | guess | unowned run, registry edge |
| `MSL_C/alloc` | 0x804578FC..0x8045F9E8 | 0x804578FC | guess | unowned run, registry edge; merged library block (Gecko + >= 8 MSL files) |

**It touches a Matching unit, with strong evidence.** `Network/initNetworkSessionStable.cpp` (264 B) lies inside the
`NetworkSessionManagerPat` TU: the pooled literal `networkSessionPeriodSeconds` (0x8079C764) is read by
`networkPatAttachBuffer` (0x803DAA30) and by it (0x803DEABC), and own slots of `__vt__24NetworkSessionManagerPat` lie
both before (0x803DE5F4) and after (0x803DEF38..0x803DF178) it. Its Matching flag measures the bytes, not the edge (its pool
is unclaimed, the three constants are undefined relocations). Caveat: if the literal is an `extern const` scalar of the
Network constants TU (the `network_shared_data` pattern) the pool half falls and only the slot argument remains.

Names: modules from the neighbours and the data (`menu_placeinfo`, `movie`, `menu_plsearch` from `__FILE__` strings:
class 1), stems from the dominant class or first named function; `fn_80383148`, `fn_80385EE0`, `fn_80456300` are
placeholders; `module main` is a guess for a root-level unit.

## Top open questions (all `guess`; 35 rows in the JSON)

1. `em019_ai + em019_prog + em_act_mot + enemy/fn_80382310` head: a 56-literal pool fold that starts below this band (0x80378F9C), so the band below owns it.
2. `Network/NetworkPeerMcs .. NetworkSessionManager`: the pool says one TU (28+ shared literals), the `.data` seams say
   several; the shared literals may be `extern const` scalars. Not moved.
3. The NetworkLayer TU spans the registered `NetworkLayerPatStep.cpp` (`lbl_8079C780` read at 0x803E1320, 0x803E2DD4,
   0x803E55D0); its start is in 0x803DF5FC..0x803E0C18, its end in 0x803E5630..0x803F07F8.
4. The em_pop head shares `frames_per_second_60f` with `quest_entry` up to 0x803B6CEC: the fold end is in 0x803B6DEC..0x803B7CE8.
5. `lb_quest_ui`, `lb_quest_board`, `multi_result`, `lb_quest_screen` each hold a second TU (55 / 16 / 16 / 12 positions).
6. `menu/movie` holds >= 3 TUs (movie, `menu_friendlist.cpp` at 0x8043F398, charmake); `menu/menu_plsearch` >= 3
   (`menu_plsearch.cpp`, `menu_message.cpp` at 0x8044E340, the save-data code).
7. `MSL_C/alloc`: candidate file boundaries from the MSL layout (13 addresses in the JSON), the Gecko | alloc edge near 0x80458BD0.
8. `Network/fn_8041A87C` holds NetworkReflectService + NetworkTimedHandler (no `.text` interval: the seam's referrers interleave).

## Remaining splitcheck failures (rendered candidate, `--proposal ... --emit-splits`)

Candidate 322 units (baseline 306). `order`, `coverage`, `text-cut`, `extab`, `dtors`, `vtable`, `bss`: 0 FAIL; `data-order` 3
and `jumptable` 1 unchanged. **New failures outside the proposal units: 0**; lint: none; `ctors` 45 -> 47, `pool` 130 -> 133.
Proposal units that still FAIL, all explained:

* `ctors` (4: `fn_80383148`, `network_pat_control`, `menu_placeinfo`, `menu_plsearch`): the checker reads the sinit's own
  end; the closure `L` lands on the unit end in each (the word targets `fn_80385E7C`, `fn_80431CD8`, `fn_8043D48C`,
  `fn_8045622C`: a sinit followed by its deferred functions). False positives.
* `pool`: `quest_entry` (the fold continues into em_pop, open question 4), `em_model` (the guess-merged roster + model: two
  TUs by construction), `lobby/lb_server_sel_trans` (`networkSessionDefaultDelay` of `network_shared_data`, read by the
  Network tail and by `NetworkSessionStable`), `NetworkSessionManagerPat` and `NetworkCommunityPat` (the NetworkLayer
  literals above, `networkRequestTimerReset`), `menu_placeinfo` / `menu/movie` (one value at two pool addresses: the merged
  runs are several TUs, open question 6).

## Measured, not assumed (clean registered units: 150 starts, 4,576 interior positions, base rate 3.2 %)

| signal | start hits / interior hits | verdict |
| --- | --- | --- |
| `pooldup` interval of 1 position | 7 / 1 | usable (0.88) |
| `pooldup` forced: one position left after the must-link closure | 8 / 1 | usable (0.89); 32 DOL-wide, none left in this band after the sinit cuts |
| `pooldup` <= 4 positions | 22 / 35 | 0.39, not enough |
| adjacent-label `pool` run jump, 1 position | 13 / 98 | noise |
| `.data` seam <= 4 positions | 2 / 5 | 0.29, not enough |
| class change (both sides named) | 3 / 4 | 0.43, not enough |
| codegen change / alignment gap / "called only from far" | 43 / 806, 6 / 174, 53 / 1052 | no enrichment |
| must-link closure vs registered starts (span <= 0x4000) | 1 contradiction in 1,648 (11 / 2,311 at 0x8000, 44 / 2,616 at 0x10000) | cap kept |
| string `pooldup` (`.sdata`, `.data`) | 27 / 143 and 61 / 367 intervals contradicted | not usable |
| vtable own-slot span as a must-link | 34 of 48 spans cross a true cut (SDK libraries) | not usable |

## Tool gaps hit

* **`splitcheck` `ctors`** reads the sinit's own end (the coordinator's finding); four proposal units FAIL on it falsely.
  `callers.py` cannot show a `.ctors` word either (no per-word symbol): the evidence rows read the word with
  `splitcheck.Dol.word`.
* **`merge_guess`** folded a `guess` unit into the previous proposal unit even when they do not abut. Fixed here
  (`tools/splits/splitcheck.py`, one selftest added): it folds only an abutting proposal unit, and a `guess` edge next to a
  registered unit keeps the registry's edge. Band `a`'s workaround (restate the neighbour) still works. Still missing: a
  way to say "a boundary exists somewhere in [a, b]" without folding (25 such intervals here), and "stay merged with the
  *registered* neighbour".
* **`tudiscover` / `datagap` must-link**: `classify()` treats any `.sdata` scalar whose value is copied elsewhere as a
  pooled literal (178 names, 51 of the 1,648 links, e.g. `networkSessionNotifyValue`, span 9,620 B); only strings are pooled
  there. Dropping them leaves the contradiction count unchanged (1) and removed the one forced position this band had
  (0x803CA484).
* **`splitcheck` `pool`** cannot tell a pooled literal from an `extern const` scalar: the whole Network pool group, the
  em_pop / quest_entry group and `networkSessionDefaultDelay` are candidates; a `const` flag in the map would settle it.
* **`tudiscover dataorder --addr`** prints the interval from the last referrer of the earlier vtable run; the first legal
  cut is the next function (the evidence rows state both).
