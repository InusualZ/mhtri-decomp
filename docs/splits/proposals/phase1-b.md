# Phase 1, band `b`: `.text` 0x800E0000 .. 0x801C0000

Proposal file: `docs/splits/proposals/phase1-b.json` (format: `docs/splits-program.md`; the fields it adds are listed at the
end). Tree `15dc2cf7d`. The band holds 69 registered units (3,345 functions, 961 of them read a pooled literal) and **no
unowned function**: every `.text` byte is registered (the 5,407 unowned functions of the program lie in other bands), so this
band is a recut audit. The registered cuts here were made by size caps and first-pool-reader heuristics, so the audit found
both directions: units that are several TUs (sound, ef) and units that are one TU (enemy).

## Counts

| | |
| --- | --- |
| registered units overlapping the band / with a FAIL | 69 / 47 (`pool` 44, `ctors` 14; a unit can have both) |
| proposal units | 27 (candidate: 304 units, baseline 306; 67 overlap the band, 20 with a FAIL: `pool` 11 - one is the remainder of `sound/fn_800DD1F0`, band `a`'s - and `ctors` 9) |
| cuts emitted | 27: **strong 12**, **medium 12**, **baseline 3** (a retained registered edge, no claim; `mhchar`'s 0x800E0504 is strong now: both its ends are pinned) |
| of which new cuts | 23 (strong 10, medium 13); 4 restate a registered start as a merge's left edge |
| registered cuts removed (`removes_cuts`) | 25: **strong 23**, **medium 2** (`0x80137604`, `0x80182D5C`: one straddling literal each) |
| registered units merged into another | 23 whole units into 8 proposal units, plus pieces of 7 more units (heads and tails) |
| registered units recut (tail cut off or moved) | 18 (`sound/fn_800DD1F0`, `fn_800E46E8`, `fn_800E8E60`; `ef/effect`, `eft004`, `eft007`, `eft029`, `fn_80105314`, `fn_80114E34`, `fn_801173AC`; `enemy/enemy_control`, `fn_80137604`, `fn_80165FC8`, `fn_801679B0`, `fn_80171194`, `fn_80181C88`, `fn_80191598`, `fn_801B0010`) |
| unowned functions covered / left open | 0 / 0 (none in the band) |
| candidate cuts kept as `open_questions` (never applied) | 10 intervals, all `guess` (each a unit that is 2 TUs until pinned) |

Registered `Matching` units touched, both absorbed in a merge with strong evidence: `enemy/fn_80149D6C.c` (into `em001_prog`) and
`enemy/fn_80177608.cpp` (into `em015_prog`). A `Matching` text says nothing about where the pool and the data belong, and a
merge that leaves the function bytes alone does not regress it; the phase-4 lane must still register the merged TU as one
unit. The other `Matching` units of the band keep their ranges: `ef/fn_800FD520.c`, `fn_800FD718.c`, `fn_80101DF4.cpp`,
`fn_80104BD0.c`, `fn_8011722C.c`. Extension mix: `fn_8012BA00.c`, `fn_8013BE60.c`, `fn_80149D6C.c` and
`fn_8014A1BC.c` are `.c` units folded into TUs whose other members are `.cpp`; the extension was a lane's guess and is not evidence.

## Grade rubric (the same as band `a`, plus two rules)

* **strong**: an exact invariant (the R1 closure bound, a pooled literal read on both sides of a cut, the compiler-made
  `0x4330000080000000` read on both sides) **plus** an independent observation that holds the same cut: a minimal pool
  interval that holds it and no other known cut, or the lowest slot of a code-pointer table exactly at it.
* **medium**: the exact invariant alone, or two soft observations that agree on one address (a minimal pool interval that
  ends at the first function of an `eftNNN_` name run, plus the call/vtable facts).
* **guess**: anything else; never emitted, kept in the top-level `open_questions` with the interval and the position count.
* **baseline** (new): a registered start the proposal keeps; the linter refuses it unless it is a registered `.text` start.

Rule 1, for a **merge**: a registered start is removed when a pooled literal is read on both sides of it (MWCC pools one
literal per TU, `docs/pool-seams.md`); strong with two such literals, or the compiler-made int->float constant, or a per-enemy
program table that points at functions on both sides; medium with one. Every one of the 9 multi-unit tie spans of the band has
**0 first-use inversions** (pool order follows text order inside each), a check a wrong merge would fail.

## The evidence families (all address-keyed, all reproducible)

1. **R1 closure** (`ctors`): the TU ends at `L`, the end of the closure of the sinit's local callees and address-taken functions
   after it (`callers.py <member>` shows the site in the sinit or an earlier member; `callers.py <L>` shows the function at `L` is
   referenced by nothing before `L` in its TU). 18 sinits in the band give 18 cuts; `L` is the checker's sinit end in 9 of them
   and 0x34..0x278 later in the other 9 (the seven sound ones, `0x800FACAC`, `0x80147C94`): `0x800E3B3C`, `0x800E5430`, `0x800E7D34`, `0x800E8E48`, `0x800E9D00`, `0x800ED780`,
   `0x800EE014`, `0x800FACAC`, `0x801153D0`, `0x8011AD58`, `0x8013791C`, `0x80147C94`, `0x801663E4`, `0x8016D1C4`, `0x80176C30`,
   `0x80182C40`, `0x80192348`, `0x801B4348`. The registered start was the `L` in **0 of 18** (band `a`: 4 of 36 tree-wide).
2. **Minimal pool intervals** (idea 94, `tudiscover at ADDR --pool-model on`, "pool dedupe" rows): a value held at two pool
   addresses puts a TU start between the last reader of the first and the first reader of the second. 70 minimal intervals in the
   band; 60 hold a cut, 10 do not (the open questions). Only **minimal** rows count: a row that contains another is implied by
   it, and intersecting rows (what a first pass did) invents narrow intervals that contradict the tie spans (5 "clusters" that
   vanished).
3. **Tie spans** (function level, from the decoded `lfs/lfd/lwz` through r2/r13): 52 shared-literal spans in the band, 9 of
   them cross a registered start. They are what merges the enemy units.
4. **Per-enemy program tables**, found while checking: `em0NN_prog_tbl` (`.data`, unowned) lists the functions of one TU. 18 of
   the 32 tables lie in the band; **no applied cut lies inside any table's span**, and the 18 registered starts that do lie
   inside one are exactly the 18 merges these tables corroborate (18 of the 25 removed cuts carry a table row). The lowest slot is
   exactly the cut at `0x80147C94` (em001), `0x8016D1C4` (em011), `0x80170600` (em012, a retained start, upgraded to strong)
   and `0x80176C30` (em015); it also names the merged TUs `em001_prog` .. `em018_prog` (the scheme of `em019_prog.cpp`).
   `em016`, `em017` and `em021` list the same functions: one TU, three enemies.
5. **Class vtables** (soft): `lbl_80597DA8`'s lowest slot is `0x800E5430` (strong there), and `lbl_80597CA0`'s slot 0 (the deleting
   destructor `0x800E0504`) moves the MHchar cut from the first named function (`0x800E0560`) to `0x800E0504`.

## Units proposed (derived names, all `src/<module>/<stem>.cpp`; placeholders `fn_<addr>` are stated in the JSON)

| unit | `.text` | cut | grade | note |
| --- | --- | --- | --- | --- |
| `sound/mhchar` | 800E0504..800E3B3C | 800E0504 | medium | tail of `fn_800DD1F0.cpp` |
| `sound/sound_job` | 800E3B3C..800E3CBC | 800E3B3C | medium | tail of `fn_800DD1F0.cpp`; micro (0x180) |
| `sound/fn_800E5430` | 800E5430..800E7D34 | 800E5430 | strong | tail of `fn_800E46E8.cpp` |
| `sound/fn_800E7D34` | 800E7D34..800E8E48 | 800E7D34 | medium | tail of `fn_800E46E8.cpp` |
| `sound/fn_800E8E48` | 800E8E48..800E8E60 | 800E8E48 | medium | tail of `fn_800E46E8.cpp`; micro (0x18, one function) |
| `sound/fn_800E9D00` | 800E9D00..800ED780 | 800E9D00 | strong | tail of `fn_800E8E60.cpp` |
| `sound/fn_800ED780` | 800ED780..800EE014 | 800ED780 | medium | tail of `fn_800E8E60.cpp` |
| `sound/quest_snd` | 800EE014..800EF7D8 | 800EE014 | medium | tail of `fn_800E8E60.cpp` |
| `ef/fn_800FACAC` | 800FACAC..800FAE08 | 800FACAC | medium | tail of `effect.cpp`; micro (0x15C) |
| `ef/eft004_fx` | 80100448..80101DF4 | 80100448 | medium | tail of `eft004.cpp`; 2 TUs (open) |
| `ef/eft007_fx` | 80102994..80103D28 | 80102994 | medium | tail of `eft007.cpp` |
| `ef/eft013_fx` | 80107250..8010BDE4 | 80107250 | medium | tail of `fn_80105314.cpp`; holds `eft014`, `eft015` names |
| `ef/eft022_fx` | 801153D0..8011722C | 801153D0 | strong | tail of `fn_80114E34.cpp`; 2 TUs (open) |
| `ef/eft026_fx` | 80117DA8..80119C44 | 80117DA8 | medium | tail of `fn_801173AC.cpp`; holds `eft028` name |
| `ef/eft029_fx` | 8011AD58..8011D448 | 8011AD58 | strong | tail of `eft029.cpp`; 2 TUs (open) |
| `enemy/em_common` | 801251D0..8013791C | 801251D0 | baseline | absorbs 5, removes 5 cuts |
| `enemy/fn_8013791C` | 8013791C..80138074 | 8013791C | medium | tail of `fn_80137604.cpp`; micro (0x758) |
| `enemy/em_kind` | 8013BE60..801411B8 | 8013BE60 | baseline | absorbs 2, removes 1 cut (the int->float constant) |
| `enemy/em001_prog` | 80147C94..80154E40 | 80147C94 | strong | tail of `enemy_control.cpp`; absorbs 4, removes 4 |
| `enemy/em008_prog` | 8015D860..801663E4 | 8015D860 | baseline | absorbs 2, removes 2 |
| `enemy/em010_prog` | 801663E4..8016D1C4 | 801663E4 | strong | tail of `fn_80165FC8.cpp`, head of `fn_801679B0.cpp`; removes 1 |
| `enemy/em011_prog` | 8016D1C4..80170600 | 8016D1C4 | strong | tail of `fn_801679B0.cpp` |
| `enemy/em012_prog` | 80170600..80176C30 | 80170600 | strong | absorbs 2, removes 2 |
| `enemy/em015_prog` | 80176C30..80182C40 | 80176C30 | strong | absorbs 5, removes 6 |
| `enemy/em016_prog` | 80182C40..80192348 | 80182C40 | strong | absorbs 2, removes 3 |
| `enemy/em018_prog` | 80192348..8019E670 | 80192348 | strong | absorbs 1, removes 1 |
| `enemy/fn_801B4348` | 801B4348..801B4458 | 801B4348 | medium | tail of `fn_801B0010.cpp`; micro (0x110) |

The five **micro** units sit between an R1 `L` and the registered start that follows it (an earlier tool's first-pool-reader
cut). R1 says the TU before ends at `L`; nothing says the registered start after it is a boundary, so each is either a real
micro-TU or the head of the TU that follows (for `fn_8013791C`: `fn_80137C9C`, `fn_80137DD0` and `fn_80138024` are called only
from the TU that starts at `0x80138074`). The alternative is one edit: remove the registered start.

## Top open questions (full list, with intervals, in the JSON; all `guess`)

1. The 5 micro units above: micro-TU, or head of the next TU.
2. `ef/eft004_fx`: a second TU in `0x801011B4..0x801017B0` (10 positions); the first pair's interval (0x800FF8D4, 0x80100448] tightens to the 4 positions (0x80100088, 0x80100448] when intersected with the 0x40a00000 pair. `eft022_fx`: one in `0x80116368..0x801168D8` (12 positions
   after the code-pointer table `lbl_805A0360` is excluded). `eft029_fx`: one in `0x8011CB74..0x8011CD78` (7).
3. `ef/eft001` (`0x800FBD68..0x800FBE64`, 2 positions), `fn_80105314` (`0x801063A8..0x80106BB4`, 13), `fn_8010BDE4` (`0x8010C0E0..
   0x8010C554`, 5), `fn_8010D1A8.c` (`0x8010D29C..0x8010E2A8`, 16), `eft019` (`0x80114B20..0x80114C20`, 2): each is 2 TUs.
4. `enemy/fn_8011D448`: one start in `0x8011E51C..0x801233C8` (8 positions after ties): this 0x7D88-byte unit is 2 TUs.
5. `eft013_fx` / `eft026_fx` hold `eft014`/`eft015` / `eft028` name runs with no separating pool interval (1 to 3 TUs each).
6. `em_common` (108 `em_*` functions) and `em_kind` have no program table: both names are guesses; `em_common`'s registered edges
   (`0x801251D0`, `0x8013791C`/`0x80138074`) are consistent with an interval of 7 and 8 positions, not pinned.

## Remaining splitcheck failures (rendered candidate: `--proposal ... --emit-splits`)

Candidate 304 units (baseline 306). `order`, `coverage`, `text-cut`, `extab`, `dtors`, `vtable`, `bss`: 0 FAIL. `ctors` 39 -> 25,
`pool` 129 -> 96, `jumptable` 1 -> 1, `data-order` 3 -> 3 (both outside the band). **New failures outside the proposal units: 0**; lint: none.

* Proposal units that still FAIL: three ef units on `pool` (`eft004_fx`, `eft022_fx`, `eft029_fx`), the three open "2 TUs" questions. The five sound units that used to FAIL `ctors`
  (`mhchar`, `fn_800E5430`, `fn_800E7D34`, `fn_800E9D00`, `fn_800ED780`) PASS: the checker reads the closure `L` now (`0x800E3B3C`, `0x800E7D34`, `0x800E8E48`, `0x800ED780`, `0x800EE014`),
  which is each unit's text end (`splitcheck.py --baseline --only ctors --unit X` prints the `detail` line per word).
* Band units outside the proposal that still FAIL: `ctors` on `fn_800E46E8`, `fn_800E8E60`, `effect`, `enemy_control` (each `L` is
  the unit end: a false positive); `pool` on `eft001`, `fn_80105314`, `fn_8010BDE4`, `fn_8010D1A8`, `eft019`, `fn_8011D448` (the open
  intervals) and `em040_ai` (`lbl_80798E20` is the r13 base read by `__start.c`: a false decode, as in band `a`).
* The `.data`/`.rodata`/`.bss` ranges of a merged TU are listed in the proposal (the whole ranges of the absorbed units), and
  the jump tables of `fn_801679B0` follow their reader (`0x805A7D54..0x805A8370` to `em011_prog`). Without them the
  candidate gains 14 `jumptable`/`vtable` failures in units that lost their text; this is phase-2 data attachment done only as far
  as the checker needs, and phase 2 re-derives it.

## Tool gaps hit

* **`splitcheck` `ctors`** (fixed since): it reads R1's `L` (see band `a`) and the own vtable slots, also past the unit end, and prints the numbers (`--baseline --only ctors --unit X`).
  `--baseline --only pool --intervals --unit X` prints each pooled value held at two addresses with its last/first reads and the interval a TU starts in: the `reproduce` of the
  `eft*_fx` pooldup rows. The five micro units' right edges are registered-only (open questions in their rows; no source before they are pinned).
* **Format** (changed in this commit, `tools/splits/splitcheck.py`): grade `baseline` (a merge's left edge was otherwise
  forced to claim a grade); `merge_guess` now folds a `guess` unit only into an **adjacent** proposal unit, else drops it and
  prints it as merged into the registered neighbour (band `a` worked around the old behaviour); `--baseline --unit REGEX` now
  prints each FAIL item's finding (the table alone named no evidence, so no `reproduce` could show one). Selftest: 5 new checks (33 -> 38).
  Fields the renderer ignores and this file uses: `absorbs`, `takes_part_of` (a registered unit only partly inside),
  `replaces_tail_of`, `removes_cuts`, top-level `open_questions` (band `a`'s), so a reviewer can see what a merge swallows.
* **No tool reports function-level tie spans or code-pointer tables.** `splitcheck` prints 130 unit-level pool FAILs; the structure
  is 52 spans and 70 minimal intervals, computed with throw-away scripts (`.pi/tmp`, not committed). A `--ties` view and a
  `tableseams` (per-enemy program tables and vtables as must-link spans, 32 tables, 0 contradictions) are the next tool requests.
* **`tudiscover` pool dedupe** prints every adjacent pair of a value, most of them non-minimal; the minimal rows are the only ones
  that force a cut (see family 2).
* **`callers.py --pointers`** counts the `@eti_` extabindex entry as a pointer table; a vtable check has to skip it.
* **Pool decode**: a `lfs` of an object the map types `float size 8` (`lbl_80796430`) is read by one function and fits no dedupe row.
