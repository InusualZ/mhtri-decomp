/* auto/804E6710_fn_804E6710.c - placeholder attribution, 1 function(s), 0x804E6710..0x804E68A8.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .data run jump jumptable_8062B6B0 -> jumptable_8062B6D4
 *   pinned seam (pool): .sdata run jump lbl_80794164 -> lbl_80794168
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   .bss         0x8075B110..0x8075B188   1 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sdata       0x80794164..0x80794168   1 labels  proposed    (dataclaim: unowned)
 *   .sbss        0x80795608..0x80795688  11 labels  NOT CLAIMED (dataclaim: unowned)
 *
 *   The `extab`/`extabindex` fragment sections are the exception: they travel with the code unit
 *   (`dataqueue.py`'s `FRAGMENT_SECTIONS`) and are claimed in `splits.txt` with its `.text`;
 *   `.ctors`/`.dtors` are added after the split. Everything else above waits for the measured pass.
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x804E0000 0x80570000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-7-alt.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-7-alt.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/804E6710_fn_804E6710.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
