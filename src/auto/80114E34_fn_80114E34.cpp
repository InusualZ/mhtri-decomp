/* auto/80114E34_fn_80114E34.cpp - placeholder attribution, 45 function(s), 0x80114E34..0x8011722C.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .sdata2 run jump lbl_80796A84 -> lbl_80796A88
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x800264A8..0x800265C8  24 labels  proposed    (dataclaim: no queue run)
 *   .data        0x805A00B8..0x805A0460  14 labels  NOT CLAIMED (dataclaim: unowned)
 *   .bss         0x806A4538..0x806A4548   1 labels  proposed    (dataclaim: unowned)
 *   .sdata       0x80791930..0x80791938   1 labels  proposed    (dataclaim: unowned)
 *   .sdata2      0x80796A28..0x80796A80  18 labels  proposed    (dataclaim: unowned)
 *
 *   The `extab`/`extabindex` fragment sections are the exception: they travel with the code unit
 *   (`dataqueue.py`'s `FRAGMENT_SECTIONS`) and are claimed in `splits.txt` with its `.text`;
 *   `.ctors`/`.dtors` are added after the split. Everything else above waits for the measured pass.
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x800f0000 0x801e0000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-4.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-4.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/80114E34_fn_80114E34.cpp`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
