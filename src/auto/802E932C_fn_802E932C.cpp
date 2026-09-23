/* auto/802E932C_fn_802E932C.cpp - placeholder attribution, 39 function(s), 0x802E932C..0x802EBBD0.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .sdata2 run jump lbl_8079A980 -> lbl_8079A988
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x80033888..0x80033A38  36 labels  proposed    (dataclaim: no queue run)
 *   .data        0x805D60F0..0x805E6EF8  11 labels  NOT CLAIMED (dataclaim: lowers-score, unowned)
 *   .bss         0x8065903C..0x806BE078   3 labels  NOT CLAIMED (dataclaim: lowers-score, unowned)
 *   .sdata       0x807927CC..0x80792F70   3 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sdata2      0x8079A8E8..0x8079A974  18 labels  NOT CLAIMED (dataclaim: unowned)
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x80280000 0x80410000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-1.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-1.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/802E932C_fn_802E932C.cpp`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
