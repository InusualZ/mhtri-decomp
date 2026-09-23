/* auto/803B177C_fn_803B177C.c - placeholder attribution, 54 function(s), 0x803B177C..0x803B43B0.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .data run jump jumptable_805F7BE8 -> jumptable_805F7C68
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x800390F0..0x80039300  44 labels  proposed    (dataclaim: no queue run)
 *   .data        0x805F76A0..0x8060DAF0   7 labels  NOT CLAIMED (dataclaim: unowned)
 *   .bss         0x806585E0..0x806CC410   5 labels  NOT CLAIMED (dataclaim: lowers-score, unowned)
 *   .sbss        0x80794C24..0x80794CFC   5 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sdata2      0x8079C520..0x8079C560   8 labels  NOT CLAIMED (dataclaim: unowned)
 *
 *   The `extab`/`extabindex` fragment sections are the exception: they travel with the code unit
 *   (`dataqueue.py`'s `FRAGMENT_SECTIONS`) and are claimed in `splits.txt` with its `.text`;
 *   `.ctors`/`.dtors` are added after the split. Everything else above waits for the measured pass.
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x80370000 0x80410000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-6.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-6.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/803B177C_fn_803B177C.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
