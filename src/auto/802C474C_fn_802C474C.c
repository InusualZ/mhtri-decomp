/* auto/802C474C_fn_802C474C.c - placeholder attribution, 9 function(s), 0x802C474C..0x802C58DC.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .sdata2 run jump lbl_8079A6AC -> lbl_8079A6BC
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x800324A8..0x80032514   9 labels  proposed    (dataclaim: no queue run)
 *   .data        0x805D4784..0x805D4964   4 labels  proposed    (dataclaim: unowned)
 *   .sbss        0x80794B60..0x80794B68   1 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sdata2      0x8079A670..0x8079A6AC   5 labels  NOT CLAIMED (dataclaim: unowned)
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x80280000 0x80410000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-1.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-1.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/802C474C_fn_802C474C.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
