# Window a: data the land gate's data-closure row refuses (none)

`python tools/units/datagap.py --row <the 74 units> --root . --base-snapshot <MAIN .pi/land-base.json orphans> --base-ref main` passes on the window a tree
(`row: PASS`, no `--allow-orphan`): the 127 sole-owned pairs the first run refused are claimed by the nine `attach` rows added to
`docs/splits/proposals/phase2-overrides.json` (rows 49-57; `phase2-reconcile.json` regenerated with `--linker`).

What the row still reports as **deferred** (not refused, no claim needed; every pair is a shared or ambiguous word whose owner the engine cannot pin):

| class | pairs | where |
| --- | ---: | --- |
| ambiguous-owner | 12 | `ef/ef_effectsystem` lbl_80594840 (the strategy class table: text-window order does not bracket it), `ef/system_core` lbl_8058AF78, `fn_8004CAD8` lbl_80581180/lbl_805812C8/lbl_80581398, `mh3_pad` (lbl_8058B0CB, lbl_805E5000, lbl_80634417, lbl_8070DF3E, lbl_8078B614 and one more) |
| isolated-run | 4 | `g3d/g3d_resmat` lbl_80791280..lbl_80791288 (.sdata) |
| span-blocked | 36 | `g3d/fn_80063888` / `g3d/fn_800680CC` (.sdata), `sound/fn_800D7F54` (9), `g3d/g3d_resmat` (23, .sdata/.sdata2) |

None of them lies in another window's zone as a **refused** pair, so no window b..fg lane inherits a data debt from this window.

Two claims went beyond the pairs the row refused, because ninja's link needed them:

* `ef/ef_effectsystem.cpp` `.sbss` 0x8079491C..0x80794920 (override row 57): the word `ef/ef_effect.cpp`'s constructor reads and writes fell in a 4-byte gap between the effect system's and the emitter's `.sbss`, and the link failed with `undefined: 'lbl_8079491C'`.
* two `force` rows (49, 53) settle the data-order seams inside the merged `ef_drawbillboardstrategy` (billboard + directional TUs) and `fn_80075DCC` units; they add two `data-order` failures to the candidate (4 -> 6), the same class the unit-level seams record.
