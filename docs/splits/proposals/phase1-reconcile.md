# Phase 1 reconciliation: the seven band proposals as one candidate

Files: `phase1-a.json` .. `phase1-g.json` (the lanes' proposals, imported from their branches with only the format drift
below fixed), `phase1-reconcile.json` (the cross-band resolutions). Check with

```
python tools/splits/splitcheck.py --proposal docs/splits/proposals/phase1-a.json ... --proposal docs/splits/proposals/phase1-g.json \
    --proposal docs/splits/proposals/phase1-reconcile.json [--emit-splits OUT]
```

Tree `8a384a266` plus the `splitcheck` change of this lane. `phase1-f.json` alone is not a closed proposal: its first unit
(`strtoul`, 0x80460D0C) abuts `fn_8045F9E8`, which neither f nor e proposes, so f alone keeps one `lint:` line; with the
reconcile file (and e) it lints clean.

## 1. Combined candidate (all seven bands + reconcile)

| | baseline | candidate |
| --- | --- | --- |
| units | 306 | **362** |
| cuts emitted (proposal-wide) | - | strong 42, medium 87, guess 190 (folded, never emitted), keep_registered_edge 18 |
| lint / warnings | - | **0 / 0** |
| `order` FAIL | 0 | 0 |
| `coverage` / `text-cut` / `extab` / `dtors` / `vtable` / `bss` FAIL | 0 | 0 |
| `ctors` FAIL | 39 | **0** |
| `pool` FAIL | 129 | **63** |
| `data-order` FAIL | 3 | 3 (unchanged) |
| `jumptable` FAIL | 1 | 1 (unchanged) |
| new failures outside the proposal units | - | **0** |

`.text`: no unowned function is left (every band's backlog is a unit or folded into one). Unowned data after the candidate
(unchanged from the baseline, phase 2's input): `.data` 61 runs / 8,454 symbols, `.bss` 17 / 439, `.sdata` 13 / 2,052, `.sdata2` 9 / 6,802,
`.sbss` 22 / 904, `.rodata` 11 / 277, `.init` 1 / 2, `extab` 1 / 5, `extabindex` 1 / 6.

The baseline column is this checker's (`pool` is 129, not the 122 the earlier build printed: the decoded data extent now
includes the unowned `.sdata2` pool past the last owned range, 602 more literals have readers). The command exits 1 because 28
proposal units still FAIL `pool` (below); that is the open-question list, not an unreconciled conflict.

### Proposal units that still FAIL `pool` (28, all documented open questions of the band that proposed them)

* **A unit holds two TUs** (`pooldup`: one value at two pool addresses read by one unit; the band's `open_questions` carry the
  candidate interval): `eft004_fx`, `eft022_fx`, `eft029_fx`, `lb_npc`, `pl_act`, `pl_motion`, `pl_coll`, `menu_item`, `hud_notice`,
  `eft035`, `fn_8031DAA8`, `em020_prog`, `em_model`, `menu_placeinfo`, `movie` (bands b/c/d/e), and the f/g units the new pool
  decode now sees: `MSL_C/alloc` (the guess block of Gecko + MSL files, `0x0` read at 0x8079C9A8 and 0x8079C9F8), `THP/fn_804DF200`,
  `WPAD/wpad`, `nw4r/fn_80501CE8`, `SO/soi`, `homebutton/fn_80533474`, `homebutton/fn_8055B710`, `homebutton/fn_80566440`.
* **One pool per TU across a kept edge**: `quest_entry` (the fold continues into `em_pop`), `lb_server_sel_trans`
  (`networkSessionDefaultDelay`), `NetworkSessionManagerPat` and `NetworkCommunityPat` (e's open question 2/3: the Network pool group,
  possibly `extern const` scalars), `RVLGX/GXTexture_tail` (`__GXData`, a `.sdata` scalar read as a literal: the tool cannot tell an
  `extern const` scalar from a pooled literal).

`lb_npc` also reports `ctors` UNKNOWN (the sinit's closure end is a function called from before it: not confirmed).

## 2. What was imported and what was fixed per band (format drift only; evidence rows untouched)

| band | file | proposal units | at import (alone) | fixes |
| --- | --- | --- | --- | --- |
| a | 0x80000000..0x800E0000 | 8 | clean | none |
| b | ..0x801C0000 | 27 | 6 lint, 26 warn | 3 cuts graded `baseline` -> `keep_registered_edge` (`em_common` 0x801251D0, `em_kind` 0x8013BE60, `em008_prog` 0x8015D860; `kind: registered-edge`, a `reproduce` added); `takes_part_of` on 26 units removed (20: the names are already in `absorbs`/`replaces_tail_of`; 6 kept as an `open_questions` line: `em_common`, `em008_prog`, `em010_prog`, `em012_prog`, `em015_prog`, `em016_prog`) |
| c | ..0x802A0000 | 11 | 5 lint, 29 warn | 5 units that start on a registered start (`lb_menu_scratch`, `pl_act_step`, `pl_act`, `pl_motion`, `pl_coll`) got a `keep_registered_edge` cut; `name_note` (11), `data_note` (4), `matching_conflict` (1) moved into the unit's `open_questions` as `key: text`; 18 `absorbs` and 3 `replaces_tail_of` names completed with the registered unit's extension, 4 prose entries (`the head/tail of X`) rewritten as `X.cpp (head|tail)` |
| d | ..0x80380000 | 17 | clean | none (its `em019_ai` is superseded below) |
| e | ..0x80460000 | 26 | 1 lint, 3 warn | 10 `guess` cuts that restate a registered edge (`NetworkSessionManagerPat`, `NetworkCommunityPat`, `network_layer_io`, `network_opening`, `constructNetworkLibrary`, `NetworkReflectService`, `network_pat_control`, `CPlusLibPPC`, `runtime`, `alloc`) -> `keep_registered_edge` (the lane's own renderer kept the registry edge; the landed renderer folds a guess cut into the registered unit it touches: `Camellia`, `Gecko_ExceptionPPC`, ... grew) |
| f | ..0x80540000 | 229 | 1 lint, 230 warn | `class` -> `kind` on 229 cuts, `text_band` -> `text_range`; the remaining lint is the e/f gap (section 1) |
| g | 0x80540000..0x8056F2B4 | 20 | 0 lint, 20 warn | `class` -> `kind` on 20 cuts; `replaces_tail_of: "band f: fn_8053E7D0"` (a proposal unit, not a baseline name) moved to an `open_questions` line |

Tool changes that made the rest clean (not file edits): `merge_guess` chains through a registered unit (e's `NetworkLayerPat`
behind its folded `NetworkSessionManagerPat`), `keep_registered_edge` and `removes_cuts` accept a registered range **end**,
the `ctors` closure takes a unit's own vtable slots after the sinit (band g's second kind of deferred code: g's 7 units, all
PASS now), `Ctx.scan` decodes the unowned `.sdata2` pool (above), `supersedes` exists for this file.

## 3. Cross-band decisions (`phase1-reconcile.json`)

1. **`fn_8053E7D0`, f vs g: g supersedes f.** Both propose `homebutton/fn_8053E7D0` from 0x8053E7D0; f ends at 0x8054032C (inside the
   function run g measured), g at 0x805425E4 with the ctors-closure evidence (the sinit `fn_8054254C` closure ends 0x805425B4 and the 7 slots after it are
   the unit's own). `supersedes: {band f, unit fn_8053E7D0}`. The renderer prints `superseded (dropped before rendering): band f unit fn_8053E7D0`.
2. **`menu/menu_item`, c/d at 0x802A6624: no conflict, no resolution needed.** c's unit is 0x80297E34..0x802A6624 (the tail of
   `Pl/fn_80295EF4` + `menu_item`, the cut 0x8029F3C8 removed: `lbl_8079A3B8`); d proposes nothing in that range, so its end stays
   the registered edge against `menu/menu_message` (0x802A6624). Recorded as a reconcile open question only. c's own residual
   stays: the unit holds `hit_*` code and menu code (`pool` FAIL, interval 0x80299C14..0x80299EF8).
3. **`em019_ai + em019_prog + em_act_mot + fn_80382310`, d/e: folded into one unit, `enemy/em019_ai` 0x80378F9C..0x80383148.** Evidence in both
   files: d counts 56 literals read on both sides of 0x8037EA64 and 50 on both sides of 0x8037F940, and 5 more shared with the registered `fn_80382310` (readers to
   0x80383148); e names the same 56-literal fold "owned by the band below" and starts its first unit with a *strong* cut at 0x80383148. Measured here: `lbl_8079BC88`
   is read by all four registered units (`splitcheck.py --baseline --only pool --unit "fn_80382310|em_act_mot"`). The reconcile file's `em019_ai` extends
   d's by the 0x80382310..0x80383148 head of `fn_80382310` (cut 0x80382310 removed, strong) and supersedes d's. e's `fn_80383148` / `fn_80385EE0` stay (they are
   the tails of `fn_80382310`). The unit holds up to four TUs worth of code; whether a cut survives inside needs the first-use order of the shared literals (phase 2).
4. **`fn_8045F9E8` (0x1324 B, 0x8045F9E8..0x80460D0C), e/f edge.** e handed it to f, f starts at `strtoul` 0x80460D0C, so nobody owned it. The reconcile file adds it
   as a `guess` unit (module `MSL_C`, like its left neighbour `alloc`): it folds into e's `alloc` block (a library block of Gecko + MSL files, a documented
   open question in e) and so does f's `strtoul` chain up to the first medium cut (`fn_804642C8`). Module naming differs between the bands (`MSL_C` in e, `MSL` in f): a phase-2 naming item.
5. **`Network/initNetworkSessionStable` (Matching) in e's `NetworkSessionManagerPat`:** kept as e proposed (strong cut 0x803DEA30 removed). Section 4.

Not reconciled (left as the bands' own open questions): the e/f MSL naming; e's Network pool group (`NetworkPeerMcs .. NetworkSessionManager`, 28+ shared literals that may be
`extern const` scalars, and the `NetworkLayerPatStep` span of the NetworkLayer TU); the two band-edge straddles that need no decision
(`sound/fn_800DD1F0` between a and b: a takes the head to 0x800DD40C, b the tail from 0x800E0504, the middle stays the registered remnant; `enemy/fn_801BD6C0` is registered and untouched across b/c).

## 4. Matching units inside a merge

The set is "a registered `Object(Matching, ...)` whose range the candidate no longer keeps" (all 37 `Matching` objects compared against the emitted candidate). Six:

| registered `Matching` unit | folded into | evidence (band, cut, grade) | what the lane that registers the merged unit must do |
| --- | --- | --- | --- |
| `ai/fn_802D0DCC.c` 0x802D0DCC..0x802D0F34 | `ai/ai_npc` (0x802C2700..0x802D9EA4) | d, cuts 0x802D0DCC and 0x802D0F34 removed, strong: 39 literals (e.g. 0x8079A670) read on both sides | the merged unit is the one object; demote `fn_802D0DCC` (`Object(NonMatching)` or fold its source into the merged file) and re-match the function inside it |
| `enemy/fn_80149D6C.c` 0x80149D6C..0x8014A1BC | `enemy/em001_prog` (0x80147C94..0x80154E40) | b, 0x80149D6C / 0x8014A1BC, strong: 46 literals both sides + `em001_prog_tbl` points at both sides | same |
| `enemy/fn_80177608.cpp` 0x80177608..0x80177890 | `enemy/em015_prog` (0x80176C30..0x80182C40) | b, 0x80177608 / 0x80177890, strong: 5-12 literals both sides + `em015_prog_tbl` | same |
| `enemy/em020_handlers.cpp` 0x80375084..0x80375424 | `enemy/em020_prog` (0x8036CF64..0x80378F9C) | d, 0x80375084 / 0x80375424, strong: 6 / 1 literals both sides | same |
| `Pl/pl_master.cpp` 0x8026BA1C..0x8026FFBC | `Pl/pl_act` (0x802693C4..0x802840DC) | c, 0x8026BA1C / 0x8026FFBC, strong: `lbl_8079A010` read by `fn_8026A3EC` (left) and `fn_802707B4` (right) | same; c's `matching_conflict` says the int->double magic is the strongest evidence |
| `Network/initNetworkSessionStable.cpp` 0x803DEA30..0x803DEB38 | `Network/NetworkSessionManagerPat` (0x803D70B8..0x803E44C8) | e, 0x803DEA30 strong + 0x803DEB38 medium: `networkSessionPeriodSeconds` read by `networkPatAttachBuffer` and by it; own vtable slots on both sides | same; e's caveat: if the literal is an `extern const` scalar the pool half falls and only the slot argument remains |

A `Matching` flag measures the bytes of the object, not the edge: its pool is typically unclaimed (the constants are undefined relocations), so the object
matching says nothing against the pool evidence. The registering lane does not keep the old object linked: the merged unit's source contains the
function, the old `Object(Matching, ...)` entry and its source file go away in the same change, and the function is re-measured inside the merged object.

## 5. Phase 2 work list (data attachment) and the lane grouping

Input per combined unit: `.ctors`/`.dtors`/`extab`/`extabindex` are derived from the text cuts (219 ranges attached by the renderer, 17 recut data runs
assigned provisionally by reader); what remains is the data the cuts do not determine. Unowned data after the candidate, assigned to the gap it
sits in (last owner before it and next owner after it, in link order); `span` = the units between those two owners in link order + 1, i.e. how many units could
take the run (the `datagap.py` classes: span 2 is "isolated/ambiguous owner", span 3+ is "span-blocked"):

| lane | `.text` window | units | `.data` runs / syms / bytes | `.rodata` | `.sdata` | `.sdata2` | `.bss` | `.sbss` | widest span |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| P2-1 | 0x80000000..0x800E0000 (band a) | 79 | 5 / 3,179 / 0x22F9C | 1 / 5 | - | **1 / 616 / 0xC10** | 2 / 84 / 0x3CEC8 | 4 / 142 | 104 |
| P2-2 | 0x800E0000..0x801C0000 (b) | 66 | 13 / 87 / 0x1800 | 2 / 4 | - | **2 / 3,664 / 0x3B30** | - | 1 / 29 | 73 |
| P2-3 | 0x801C0000..0x802A0000 (c) | 28 | 8 / 849 / 0x11AF4 | 1 / 8 | - | - | 3 / 27 / 0x4698 | 1 / 2 | 65 |
| P2-4 | 0x802A0000..0x80380000 (d) | 47 | 14 / 1,281 / 0x19468 | 2 / 3 | 3 / 670 | 3 / 1,719 / 0x1CC0 | 4 / 36 / 0x9128 | 3 / 13 | 34 |
| P2-5 | 0x80380000..0x80460000 (e) | 52 | 13 / 1,636 / 0x1BE73 | 3 / 94 / 0x2EA7 | 4 / 274 | 3 / 803 | 5 / 134 / 0x7A010 | 7 / 290 | 94 |
| P2-6 | 0x80460000..0x8056F2B4 (f+g) | 78 | 8 / 1,422 / 0x4513E | 1 / 130 / 0x79F8 | 5 / 349 | - | 3 / 158 / 0x4891C | 5 / 425 | 29 |

(`.sdata2`'s 9 runs / 6,802 literals sum to 30,132 B over the lanes' rows; the 3 `.sbss2` symbols and the 3 `.rodata`/`.sdata`/`.sbss` runs before the first owner
(0x8056F4A0, 0x80790E20, 0x80794760, 0x8079D7E0) belong to P2-6 / the start of the link order.)

Per lane, the phase 2 job is: for every unowned run, pick the owner among its span with the readers (`callers.py`), the first-use order (pool) and the data-order seams
(`docs/data-order-seams.md`), claim it in the lane's proposal `ranges` (`.data`, `.sdata`, `.sdata2`, `.bss`, `.sbss`, `.rodata`), and record a cut-by-cut grade as phase 1 does;
the provisional by-reader assignment of the 17 recut runs is replaced by evidence. Specific items:

* **P2-1 (a)**: `.data` 0x22F9C B in 5 runs (3,179 symbols; span up to 67), `.bss` 0x3CEC8 (two runs, spans 68), the 616 unowned `.sdata2` literals in the window.
* **P2-2 (b)**: the 3,664-literal `.sdata2` pool (two runs, span up to 73; the em-program and `eft*_fx` literals, where the `pool` FAILs
  of `eft004_fx`, `eft022_fx`, `eft029_fx` need the same first-use order); 13 small `.data` runs, mostly span 2.
* **P2-3 (c)**: `.data` 849 symbols in 8 runs (the player tables), no `.sdata2` backlog; c's three jump tables are attached whole (`pl_act_step`, `pl_act`, `pl_motion`) and
  their remnants stay data-only entries (c's `data_note`s).
* **P2-4 (d)**: the only lane with all of `.data`, `.sdata`, `.sdata2`, `.bss`, `.sbss`; `.sdata` 670 symbols (span up to 28) and `.sdata2` 1,719 (span up to 34) over the hud / menu /
  enemy units; the em019 fold's pool (decision 3) decides 56 shared literals here.
* **P2-5 (e)**: `.bss` 0x7A010 and `.data` 0x1BE73 around the Network units; the Network pool group and the 94-symbol `.rodata` run; the e `open_questions` (the NetworkLayer TU
  spanning `NetworkLayerPatStep`, 35 rows of candidate cuts) are resolved by the data attachments or stay guesses.
* **P2-6 (f+g)**: the biggest bytes: `.data` 0x4513E and `.bss` 0x4891C over the SDK libraries (BTE 57 units, OS, DWCi) and the homebutton units; `.rodata` 0x79F8 (130 symbols);
  the `.sdata2` pool past the last owned range (608 literals at 0x8079C998..0x8079D7F0, read by f's and g's text) is invisible to the old scan and now decoded; by address the table books
  it to P2-5 (its last owner), but its readers are P2-6's units, so P2-6 attaches it per unit (the `pooldup` findings are the 9 `pool` FAILs of the f/g units listed in section 1) and turns the guess candidates in f's `open_questions` into cuts or keeps them.

Also for phase 2: `.init` (2 symbols, 164 B), `extab` (5) and `extabindex` (6) the renderer could not attach; the module naming split (`MSL` vs `MSL_C`, `Runtime` vs
`Runtime.PPCEABI.H`) between bands e and f; and the `lb_npc` ctors UNKNOWN.
