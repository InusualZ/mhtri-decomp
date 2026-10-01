# Phase 1, band `c`: `.text` 0x801C0000 .. 0x802A0000

Proposal file: `docs/splits/proposals/phase1-c.json` (format: `docs/splits-program.md`). Tree `15dc2cf7d`.
The band holds 40 registered units that start in it (41 overlap it; `enemy/fn_801BD6C0` starts in band `b`), 2,435 functions:
enemy 0x801C29F8.., `lobby/`, `Pl/` and the head of `menu/menu_item`. There is **no unowned `.text`** in it (`coverage_gaps` is
empty here; the 5,407 unowned functions are in other bands), so it is a recut audit, not a fill: 25 of the 40 units carry a
`splitcheck` FAIL (none of the four enemy ones does).

## Counts

| | |
| --- | --- |
| proposal units (rendered candidate) | 11 (23 registered units replaced; candidate 294 units, baseline 306) |
| cuts emitted | 5: **strong 4** (`0x8021F020`, `0x80220038`, `0x8028F44C`, `0x80297E34`), **medium 1** (`0x802673A4`), guess 0 |
| registered cuts removed (`removes_cuts`) | 17, all **strong**: one pooled literal read on both sides of each edge (`callers.py <literal>`) |
| registered units merged | 4 -> 1 (`lb_npc`), 2 -> 1 (`fn_80220038` = `fn_80224AC4` + the tail of `fn_8021E1EC`), 6 -> 1 (`pl_act_step`), 7 -> 1 (`pl_act`), 2 -> 1 each for `pl_coll` and `menu_item` |
| registered units recut | `fn_8021E1EC` (3 pieces), `fn_80262940`, `fn_80288CEC`, `fn_80295EF4` (head/tail) |
| candidate cuts kept as `open_questions` (never applied) | 10 `guess` intervals (8 in unit rows, 2 top-level), 4 to 81 positions each |
| unowned functions covered / left open | 0 / 0 (none in the band) |
| `Matching` units touched | 1: `Pl/pl_master` lies inside the `pl_act` merge (strong evidence, flagged `matching_conflict`) |

Grades use the rubric of band `a` (strong = an exact invariant plus an independent soft interval that holds the cut and no
other known cut; medium = the exact invariant alone or a narrow soft interval; guess = the rest). `0x802673A4` is medium
because its pool interval (37 positions) also holds the registered `0x802693C4`.

## Method (what the tools could not do, done in scratch scripts that are not committed)

1. Decode every function's `.sdata2` reads (splitcheck's own `scan_refs`). A position between two functions is **pool-separable**
   iff every pool address read on the left is below every one read on the right (link order: one TU's pool is contiguous and
   TUs follow text order). A maximal run with no separable interior position is **one TU** by the pool: 53 such runs in the
   band; 8 of them span registered starts. Those 17 starts are the removed cuts.
2. A value held at two pool addresses needs a cut between the two readers (idea 94). Between two consecutive runs that hold a
   common value that is a **forced interval**; 38 of them. 23 already hold a registered start (nothing to do), 15 hold none:
   4 are placed by the sinit closure below (strong) and 11 remain as `guess` rows (10 cuts: two rows are one cut).
3. `.ctors` words follow band `a`'s closure rule (R1, `docs/splits-program.md` end): the TU ends at `L` = the end of the
   contiguous run of functions after the sinit that the sinit chain references, and only if the function at `L` is referenced
   by nothing before it in the unit. Six sinit functions in the band: five give a cut (`0x8021F020`, `0x80220038`,
   `0x802673A4`, `0x8028F44C`, `0x80297E34`), the sixth (`fn_801FF700`, `L = 0x801FF9E0`) does not, because `fn_801FF9E0`
   is called from `fn_801FD174` before it.

Every `evidence` command was run while generating the file and its output contains the functions named in the finding
(`splitcheck ... --json F.json` rows are read from the JSON).

## Units proposed / recut / merged

| unit (derived) | range | cut | grade | note |
| --- | --- | --- | --- | --- |
| `lobby/lb_npc` | 0x801FBF78..0x80212810 | - | - | `lb_npc`+`fn_802029B4`+`fn_802076D4`+`fn_8020C588`; 3 removed cuts; holds >= 3 TUs (2 open intervals) |
| `lobby/lb_menu_scratch` | 0x8021E1EC..0x8021F020 | - | - | head of `fn_8021E1EC`; named from its sinit's global (guess) |
| `lobby/lb_menu_pos_tbl` | 0x8021F020..0x80220038 | 0x8021F020 | strong | sinit `fn_8021FF5C` + local ctor `fn_8021FFFC`; the 4330.. dup interval ends exactly at the cut |
| `lobby/fn_80220038` | 0x80220038..0x80229ECC | 0x80220038 | strong | absorbs `Pl/fn_80224AC4` (removed cut 0x80224AC4: `lbl_80799CD8` read on both sides) |
| `Pl/pl_act_step` | 0x802430E8..0x802673A4 | - | - | 6 units, 5 removed cuts (`pl_float_zero` etc.); ends on the sinit `fn_80267328` |
| `Pl/player_control` | 0x802673A4..0x802693C4 | 0x802673A4 | medium | tail of `fn_80262940`; its right edge is a registered cut with no evidence of its own |
| `Pl/pl_act` | 0x802693C4..0x802840DC | - | - | 7 units, 6 removed cuts (`lbl_8079A008`, `lbl_8079A084`); holds 4 TUs (3 open intervals) |
| `Pl/pl_motion` | 0x80288CEC..0x8028F44C | - | - | head of `fn_80288CEC`; 2 TUs (1 open interval) |
| `Pl/fn_8028F44C` | 0x8028F44C..0x8028F66C | 0x8028F44C | strong | sinit `fn_8028F400` has no local callee; `3f000000` dup interval holds only this cut |
| `Pl/pl_coll` | 0x8028F66C..0x80297E34 | - | - | `fn_8028F66C` + head of `fn_80295EF4` (removed cut 0x80295EF4); ends at `L` of the sinit `fn_80297C30` |
| `menu/menu_item` | 0x80297E34..0x802A6624 | 0x80297E34 | strong | tail of `fn_80295EF4` + `menu_item` (removed cut 0x8029F3C8: `lbl_8079A3B8`); its end is band `d`'s |

Data: four units carry the `.data` of the registered unit they absorb (jump tables: `lb_npc` 0x805B9770, `pl_act_step`
0x805C4134, `pl_act` 0x805C5FA0..0x805C6100 incl. two unowned objects between its two halves, `pl_motion` 0x805CBFC0), so
`jumptable` does not report a new failure. Two data-only entries remain (`lobby/fn_8021E1EC`: `.data`/`.bss`; `Pl/fn_80288CEC`:
`.bss`/`.sbss`). Everything else in `.data`/`.bss`/pools is phase 2.

## Top open questions (all `guess`; full rows in the JSON)

1. `lb_npc`: cut in 0x801FF9E0..0x801FFE44 (6 positions; closure end up to the first reader of the next pool) and one in
   0x80207E9C..0x80208B94 (25 positions, nothing narrows it).
2. `pl_act`: cuts in 0x802752C8..0x802756F0 (8), 0x8027DC64..0x80280248 (81; it holds the dense equipment-helper cluster
   0x8027E29C..0x8027FD7C) and 0x802829B4..0x80282CCC (4).
3. `fn_801E0ADC` (unchanged): one cut in 0x801E2864..0x801E2A9C (3; a weak `.data` V->D seam 0x805B76D0 agrees, does not narrow).
4. `fn_802840DC` (unchanged): one cut in 0x80286DF8..0x802874E0 (7).
5. `pl_motion` 0x8028B298..0x8028C168 (9), `pl_coll` 0x8029135C..0x80291664 (4), `menu_item` 0x80299C14..0x80299EF8 (4).
6. `menu_item`: the registered `menu_item` really starts after the `hit_*` functions (0x8029F61C `get_item_data_ptr`); a further cut
   inside this unit, not placeable from here.
7. Registered edges with no evidence of their own (kept): `0x802693C4` (the right edge of `player_control`), `0x8028F66C` (the 0x220 B unit `Pl/fn_8028F44C` may
   belong to the next TU), `0x8021E1EC`, `0x801FBF78`.
8. `pl_act` holds a pinned zigzag seam `0x805BAB74` (a TU start in 0x80269E8C..0x80269EC8, inside its registered head): contradicted by the pool (`lbl_8079A004` is read at 0x80269CD4 before the interval and at 0x8026A158 / 0x8026A204 after it); the unit stays merged.

## Remaining splitcheck failures (rendered candidate)

Candidate 294 units (baseline 306), band alone. `order`, `coverage`, `text-cut`, `extab`, `dtors`, `vtable`, `bss`, `local-static`: 0 FAIL. `ctors` 39 -> 35
(the rest are baseline units this band does not touch), `pool` 129 -> 112, `data-order` 3 -> 3, `jumptable` 1 -> 1. **New failures outside the proposal units: 0**; lint: none.
Proposal units that PASS everything: `lb_menu_scratch`, `lb_menu_pos_tbl`, `fn_80220038`, `pl_act_step`, `player_control`, `fn_8028F44C`. The other five, and why:

* `lb_npc`, `pl_act`, `pl_motion`, `pl_coll`, `menu_item`: `pool` FAIL = the open `guess` intervals above (the unit holds two TUs, the checker says so; nothing is hidden);
  `lb_npc` also reports `ctors` UNKNOWN (`fn_801FF9E0` is called from `fn_801FD174` before it).
* Unchanged registered units that still FAIL in the band: `fn_801E0ADC` and `fn_802840DC` (`pool`, the open questions 3-4), `Pl/fn_8023C2D0` (`jumptable`, below).

The earlier `ctors` "false positives" (`lb_menu_scratch`, `lb_menu_pos_tbl`, `pl_coll`) are gone: the checker reads the closure of the sinit's callees, not the sinit's own end
(`splitcheck.py --baseline --only ctors --unit fn_8021E1EC` prints the `detail` lines with both ends).

## Tool gaps hit

* `ctors`: the sinit end is not a cut (band `a`'s finding; here 5 of 6 words). Fixed: `splitcheck` reads R1's closure (and, past the unit end, the unit's own
  vtable slots) and prints it with `--baseline --only ctors --unit X`.
* **No tool lists pool-separable runs, forced intervals or the baseline cuts they contradict.** `splitcheck --baseline` reports a
  FAIL per unit; `tudiscover at` prints at most six dedupe rows per target (the 3f000000 row for `0x8028F44C` is not among them, it is in
  the `splitcheck --baseline --only pool` JSON). The atom/interval analysis above lives in uncommitted scratch scripts; a
  `splitcheck --baseline --only pool --intervals --unit X` now prints each pooled value held at two addresses with the interval a TU starts in. The positions inside an interval
  need one more signal: the call-crossing count inside +-25 functions has a unique minimum for `0x8028B524` and `0x80282BDC`
  (not a reproducible command, so not used).
* `lint_proposal` demanded a cut for a unit that starts on a registered start; it now takes the baseline starts (changed here,
  with a selftest row that fails on the old code).
* `jumptable` false decode: `Pl/fn_8023C2D0` reports `jumptable_805C390C` as read by `lobby/fn_80219260` (`lhz r0, 0x3ac0(r3)`
  with `r3` a computed base: 0x805C3AC0 lies inside the 0x414 B symbol, whose real reader is `fn_8023FC20`, `callers.py` lists one).
* `merge_guess` folds into the previous proposal unit even if not adjacent (band `a`): no `guess` cut is emitted here.
* Merging a `Matching` unit has no representation: `matching_conflict` is a free-text key.
