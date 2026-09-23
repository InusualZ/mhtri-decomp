/* auto/80287244_fn_80287244.c - placeholder attribution, 15 function(s), 0x80287244..0x80288848.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .data run jump jumptable_805CB1FC -> jumptable_805CBFC0
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x800305D0..0x8003066C  13 labels  proposed    (dataclaim: no queue run)
 *   .data        0x805CB0B0..0x805CB894  13 labels  proposed    (dataclaim: unowned)
 *   .sdata       0x80792190..0x807921A8   3 labels  proposed    (dataclaim: unowned)
 *   .sbss        0x80794B60..0x80794B68   1 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sdata2      0x8079A228..0x8079A270  17 labels  proposed    (dataclaim: unowned)
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x80280000 0x80410000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-1.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-1.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/80287244_fn_80287244.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
