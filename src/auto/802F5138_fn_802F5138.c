/* auto/802F5138_fn_802F5138.c - placeholder attribution, 61 function(s), 0x802F5138..0x802F9994.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .data run jump jumptable_805D89EC -> jumptable_805D8A48
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x80033E04..0x8003402C  46 labels  proposed    (dataclaim: no queue run)
 *   .rodata      0x80570700..0x805707C0   3 labels  proposed    (dataclaim: unowned)
 *   .data        0x805D6D7C..0x805D8210  27 labels  proposed    (dataclaim: unowned)
 *   .bss         0x806585E0..0x806BE0C0   5 labels  NOT CLAIMED (dataclaim: lowers-score, unowned)
 *   .sdata       0x80792884..0x807928D0  16 labels  proposed    (dataclaim: unowned)
 *   .sdata2      0x8079AA88..0x8079ABC0  75 labels  proposed    (dataclaim: unowned)
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x80280000 0x80410000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-1.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-1.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/802F5138_fn_802F5138.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
