/* auto/80500E34_SinFIdx__Q24nw4r4mathFf.cpp - placeholder attribution, 5 function(s), 0x80500E34..0x805012C4.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .sdata2 run jump lbl_8079D4C0 -> lbl_8079D4C8
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   .rodata      0x80573CD8..0x80574CE8   1 labels  NOT CLAIMED (dataclaim: unowned)
 *   .data        0x8062F9C0..0x8062FAC8   1 labels  proposed    (dataclaim: unowned)
 *   .sdata2      0x8079D4A0..0x8079D4C8   9 labels  proposed    (dataclaim: unowned)
 *
 *   The `extab`/`extabindex` fragment sections are the exception: they travel with the code unit
 *   (`dataqueue.py`'s `FRAGMENT_SECTIONS`) and are claimed in `splits.txt` with its `.text`;
 *   `.ctors`/`.dtors` are added after the split. Everything else above waits for the measured pass.
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x804E0000 0x80570000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-7-alt.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-7-alt.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/80500E34_SinFIdx__Q24nw4r4mathFf.cpp`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
