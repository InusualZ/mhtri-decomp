/* auto/802D0F34_fn_802D0F34.cpp - placeholder attribution, 89 function(s), 0x802D0F34..0x802D4248.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .data run jump jumptable_805D4F3C -> jumptable_805D5000
 *   pinned seam (pool): .sdata2 run jump lbl_8079A7B8 -> lbl_8079A7BC
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x80032ACC..0x80032C7C  36 labels  proposed    (dataclaim: no queue run)
 *   .data        0x805D3AD8..0x805D4F3C   5 labels  NOT CLAIMED (dataclaim: unowned)
 *   .bss         0x806BD360..0x806BD808   1 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sdata       0x80792508..0x80792510   1 labels  proposed    (dataclaim: unowned)
 *   .sbss        0x80794974..0x80794978   1 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sdata2      0x8079A670..0x8079A7B8  19 labels  NOT CLAIMED (dataclaim: unowned)
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x80280000 0x80410000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-1.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-1.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/802D0F34_fn_802D0F34.cpp`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
