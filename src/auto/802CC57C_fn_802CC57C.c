/* auto/802CC57C_fn_802CC57C.c - placeholder attribution, 1 function(s), 0x802CC57C..0x802CC794.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .sdata2 run jump lbl_8079A778 -> lbl_8079A77C
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x80032814..0x80032820   1 labels  proposed    (dataclaim: no queue run)
 *   .sdata2      0x8079A778..0x8079A77C   1 labels  proposed    (dataclaim: unowned)
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x80280000 0x80410000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-1.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-1.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/802CC57C_fn_802CC57C.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
