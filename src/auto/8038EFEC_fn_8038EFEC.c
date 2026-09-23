/* auto/8038EFEC_fn_8038EFEC.c - placeholder attribution, 58 function(s), 0x8038EFEC..0x80394038.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .sdata2 run jump lbl_8079C2B0 -> lbl_8079C2B8
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x80038130..0x80038340  44 labels  proposed    (dataclaim: no queue run)
 *   .data        0x805BFFF8..0x805F13D0  44 labels  NOT CLAIMED (dataclaim: lowers-score, unowned)
 *   .bss         0x806585E0..0x806AACC0   3 labels  NOT CLAIMED (dataclaim: lowers-score, unowned)
 *   .sdata       0x807933D8..0x8079346F  20 labels  proposed    (dataclaim: unowned)
 *   .sbss        0x80794880..0x80794884   1 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sdata2      0x8079C2A4..0x8079C2B4   4 labels  proposed    (dataclaim: unowned)
 *
 *   The `extab`/`extabindex` fragment sections are the exception: they travel with the code unit
 *   (`dataqueue.py`'s `FRAGMENT_SECTIONS`) and are claimed in `splits.txt` with its `.text`;
 *   `.ctors`/`.dtors` are added after the split. Everything else above waits for the measured pass.
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x80370000 0x80410000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-6.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-6.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/8038EFEC_fn_8038EFEC.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
