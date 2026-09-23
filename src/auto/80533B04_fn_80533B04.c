/* auto/80533B04_fn_80533B04.c - placeholder attribution, 77 function(s), 0x80533B04..0x80537B2C.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .data run jump jumptable_8064D678 -> jumptable_8064D6B0
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   .rodata      0x8057A690..0x8057AB98   3 labels  NOT CLAIMED (dataclaim: unowned)
 *   .data        0x8064A8F8..0x8064D584   5 labels  NOT CLAIMED (dataclaim: unowned)
 *   .bss         0x807901C0..0x807901F0   2 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sdata       0x807944B8..0x80794540   5 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sbss        0x80794D60..0x80795A50   4 labels  NOT CLAIMED (dataclaim: lowers-score, unowned)
 *   .sdata2      0x8079D608..0x8079D620   6 labels  proposed    (dataclaim: unowned)
 *
 *   The `extab`/`extabindex` fragment sections are the exception: they travel with the code unit
 *   (`dataqueue.py`'s `FRAGMENT_SECTIONS`) and are claimed in `splits.txt` with its `.text`;
 *   `.ctors`/`.dtors` are added after the split. Everything else above waits for the measured pass.
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x804E0000 0x80570000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-7-alt.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-7-alt.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/80533B04_fn_80533B04.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
