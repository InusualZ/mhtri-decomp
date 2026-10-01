# Phase 1, band `g`: `.text` 0x8054032C .. 0x8056F2B4 (end of `.text`)

Proposal file: `docs/splits/proposals/phase1-g.json` (format and extension keys: `docs/splits-program.md` with the additions of bands `a` and `f`).
Tree `15dc2cf7d`. The band holds 1,008 functions: 147 unowned (0x8054032C..0x805482CC) and 861 in seven registered units
(`fn_805482CC.cpp`, `homebutton/fn_8054E894.cpp`, `fn_80555374.cpp`, `keyboard_ui.cpp`, `keyboard.cpp`, `gui.cpp`, `tiHKBManager.cpp`, all
`NonMatching`), i.e. the whole Home-button software-keyboard block. There is **no `extab` record and no function name** here; every function is `fn_XXXXXXXX`.
`0x8054032C` is band `f`'s edge, not a cut: the 20-function sdata2 must-link block `lbl_8079D620` (readers `0x8053F22C`, `0x80540684`) runs across it,
so the first unit starts at `0x8053E808` (band `f`'s cut `0x8053E7D0` moved past the seven 8-byte slots of `lbl_8064DC30`, which close the TU that holds the sinit `fn_8053E5F8`) and **supersedes band `f`'s last unit** `fn_8053E808` (drop that unit when both files are rendered; with it dropped
`--proposal f --proposal g` renders 363 units, no lint, 0 new failures outside the proposal units).

## Counts

| | |
| --- | --- |
| cuts emitted | 21: **strong 2**, **medium 13**, **guess 5**, **keep_registered_edge 1** (`0x80569DAC`) |
| cuts by class | strong: ctors+vtable-run+pooldup 2, pool+name 1; medium: ctors 11, inherited 1; guess: seam 3 (zigzag), pool 1 (`pooldup`), anchor 1 |
| proposal units in the file / rendered (guess cuts merged) | 21 / **16** (candidate 315 units; `homebutton/gui` is the remnant of the registered gui.cpp head) |
| registered units recut / merged | all 7 replaced (every one is wholly inside the band and `NonMatching`); `removes_cuts` rows: 5 (`0x805482CC` and `0x805632BC` strong; `0x8054E894`, `0x80555374`, `0x8055C894` medium, and these three are removals by ABSENCE of evidence of `--max-bytes` caps, not contradictions); `0x80569DAC` is KEPT as a registered edge (a TU tail: slots of `lbl_80657F10` end in `b fn_80566440`); `0x8056BBF0` is restated as a guess candidate |
| functions covered | 1,008 of 1,008 (+56 in `0x8054032C..0x805425E4`, which band `f` left to this unit's tail); 306 sit in rendered units with no open interval, 702 in units that keep a guess candidate or an unresolved interval |
| splitcheck, rendered candidate (band alone) | `order`, `coverage`, `text-cut`, `extab`, `dtors`, `vtable`, `bss`, `local-static`: 0 FAIL; `pool` 129 -> 126 (one proposal unit FAILs: `fn_8055B710`), `data-order` 3, `jumptable` 1 (unchanged); `ctors` 39 -> 34 (all the 7 recut units PASS); **new failures outside the proposal units: 0**; lint: none |

## Grade rubric (band `a`'s)

* **strong**: an exact invariant plus an independent soft interval holding the cut and no other known cut.
* **medium**: the exact invariant alone (a `.ctors` word's deferred closure end, an inherited edge).
* **guess**: one soft interval; never emitted, listed with its positions. A guess cut is placed at the interval's last position, which is the constructor
  that first reads the vtable the seam is about (a position choice, not evidence).

## What is proven, and the finding this band adds

* **13 `.ctors` words = 13 TU ends** (`0x8056F3F8..0x8056F428`). Each sinit's end is a TU end only after the deferred code is accounted for (band `a`'s rule R1), and
  this band adds a second kind of deferred code: **inline virtual functions the TU emits after its `__sinit`**. For 7 of the 13 sinits a run of 3..24 small functions
  follows (`0x805425B4..0x805425DC`, `0x8054F6AC..0x8054F7B8`, `0x80554664..0x805546AC`, `0x80555AF4..0x80555B04`, `0x80556FA4..0x80556FBC`, `0x80558E84..0x80558EAC`,
  `0x8055EAB8..0x8055EAE8`); each is a slot of a vtable whose constructor lies in the unit and whose other slots lie below the sinit, and nothing in the unit calls them
  (`callers.py <addr> --pointers`). The unit therefore ends at the end of the run (`L'`), not at the sinit end. The 13 cuts are `0x805425E4`, `0x8054F7BC`, `0x805546B4`,
  `0x80555B0C`, `0x80556FC4`, `0x80558EB4`, `0x8055EAF0`, `0x8055F728`, `0x8055FB70`, `0x8055FD58`, `0x8056083C`, `0x80566440`, `0x8056D814` (the last through band `a`'s R1:
  the sinit registers the dtor `fn_8056D7D4`). They are consistent with the pool: none lies inside an sdata2 must-link span (10 blocks, 34 shared literals; probability that 13 random
  positions all avoid them about 0.1 %).
* **The sdata2 must-link blocks contradict two registered cuts**: `0x805482CC` (the int-to-float bias double `lbl_8079D630`, read at `0x80543110` and `0x80548504`, plus `0.0f`, `1.0f`, `0.5f`
  runs) and `0x805632BC` (`0.0f` `lbl_8079D738`, 55 sites in 9 functions across it). Both registered headers say the edge was a `--max-bytes` cut.
* **Exact `pooldup` cut `0x8056A478`**: `0.0f` held at `0x8079D788` and `0x8079D7A0`, one admissible position; the registered `gui.cpp` header names this function `homebutton::gui::drawLine_`.
* `0x80556FC4` is strong because an independent interval holds it alone (the `V->S` seam `0x80650B40`). `0x80566440` was strong and is now **medium**: `fn_80566440` is slot[2] of `lbl_80657F10`, whose other slots lie below it, so the sinit closure (0x80566440) and the own-slot reading (0x80567004) disagree, and the pool allows one of {0x80566440, 0x805664F0, 0x80566618}.
* Three TUs are one function each, the sinit alone (`0x8055F728` 0x448 B, `0x8055FB70` 0x1E8 B, `0x8055FD58` 0xAE4 B): static object tables with constructors and no function of their own.

## Units (rendered)

| unit (derived) | range | cut / grade | note |
| --- | --- | --- | --- |
| `fn_8053E808` | 0x8053E808..0x805425E4 | inherited / medium | band `f`'s unit, start moved from 0x8053E7D0 (own vtable slots of `lbl_8064DC30`), extended 0x2288 B |
| `fn_80542D8C` | 0x805425E4..0x8054F7BC | ctors / medium (+ guess 0x80542D8C) | panel class; absorbs `fn_805482CC.cpp` |
| `fn_8054F7BC` | 0x8054F7BC..0x805546B4 | ctors / medium (+ guess 0x80554278) | |
| `fn_805546B4`, `fn_80555B0C`, `fn_80556FC4` | ..0x80555B0C, ..0x80556FC4, ..0x80558EB4 | ctors / medium, medium, strong | |
| `fn_8055B710` | 0x80558EB4..0x8055EAF0 | ctors / medium (+ guess 0x8055B710, `pooldup`) | VK layout classes |
| `fn_8055EAF0`, `fn_8055F728`, `fn_8055FB70`, `fn_8055FD58` | 0x8055EAF0..0x8056083C | ctors / medium | sinit-only tail |
| `fn_8056083C` | 0x8056083C..0x80566440 | ctors / medium | |
| `fn_80566440` | 0x80566440..0x80569DAC | ctors / medium | ends at the registered gui.cpp start (kept) |
| `gui` | 0x80569DAC..0x8056A478 | keep_registered_edge / guess | the head of the registered gui.cpp up to the exact cut 0x8056A478 |
| `fn_8056A478` / `tiHKBManager` | 0x8056A478..0x8056D814 | pool+name / strong (+ guess 0x8056BBF0, `__FILE__` anchor) | `homebutton::gui` start; the second half is named from the anchor `tiHKBManager.cpp` |
| `fn_8056D814` | 0x8056D814..0x8056F2B4 | ctors / medium (+ guess 0x8056F02C) | the last `.text` TU |

Module `homebutton` (the registered neighbours' directory; the strings are `P_*`/`B_*`/`T_*` pane names and `fs_VK_*.brlyt`). Stems are `fn_<addr>` placeholders.

## Top open questions

1. **The ctors rule** used to FAIL 8 of the 13 `.ctors` words because deferred code follows the sinit; the checker now reads the closure and the unit's own vtable slots, so every proposal unit PASSes (`ctors` 39 -> 34, band alone). The own-slot run past the unit end is capped to an inline-sized first slot (0x40 B): a larger function is another TU's member, which is why `0x80566440` (0xB0 B, slot[2] of `lbl_80657F10`) is read both ways in its row.
2. `0x805427E8..0x80542D8C` (zigzag, 6 positions) conflicts with three vtables of one reverse-ordered run (`lbl_8064EF04`, `lbl_8064F0BC`, `lbl_8064F1C0`) read by `0x805427E8` and `0x80542D8C`: one TU or two.
3. `0x8055CB34..0x8055E42C` (zigzag, 29 positions) and `0x80569554..0x80569E64` (three pooled values held twice, 46 positions; `0x80569DAC` is one, kept as a registered edge) each need a cut; no candidate proposed. `0x8055C894` lies BELOW the first interval, so it is not one of its positions.
4. The `0x8054E894..0x8054F7BC` tail, the HBM class split inside `fn_8056D814` (`0x8056EA18..0x8056F02C`, 8 positions) and the `tiHKBManager` start (15 positions after the gui block).
5. `0x80542D8C`, `0x80554278`, `0x8055B710`, `0x8056BBF0`, `0x8056F02C` stay merged (guess); the strong zigzag `0x806579C8` (`0x805615F8..0x80566340`) is contradicted by `lbl_8079D738` and unused.

## Tool gaps hit

* (fixed since) **`splitcheck` decodes no unowned `.sdata2`**: `Ctx.scan` keeps a data address only below the highest *registered* range end, so the literals of this band (`0x8079D620..0x8079D7A8`) were never decoded; its `pool`
  rows are vacuous for unowned text and the registered cuts `0x805482CC`/`0x805632BC` passed. Scratch rescan with `hi = 0x807B0000` agrees with `callers.py` site for site (`0x8079D738`: 55 sites, 9 functions).
* (fixed since) **`splitcheck` `ctors`** read the sinit's own end; the closure and the deferred virtual runs are read now, with selftests for a slot run, a cut over a unit's own slots (FAIL) and a large non-inline function after the end (not a slot).
* **`tudiscover` jump-table seam** (`jumptable_8064E4E0 -> jumptable_8064E500`, `strong x1`, cut `0x80541DA0`) has no grammar in `docs/data-order-seams.md` and sits inside one vtable's slot span: not used.
* **Map**: `lbl_8079D620`/`lbl_8079D624` are 1-byte `.sdata2` objects that the code reads with `lbz`; tudiscover's must-link reads them as pool anchors correctly, the typing is only odd.
* **New evidence class, not in the tools**: a function-local static's destructor is passed to `__register_global_object` by `lis/addi`; a destructor taken by exactly one function chain is a
  must-link (`0x8056BC5C..0x8056CC54`), one taken by several TUs (`fn_8056F234`, taken from `0x8054358C`, `0x8056D838`, `0x8056D8DC`) is the dtor of a class that lives in the last TU.
* **`tudiscover at` costs 1.5..12 s per address**; a sweep of match sets over a band is not practical (killed after 9 minutes, 300 single-function sets); the scratch scripts read `splitcheck`'s decode instead.
