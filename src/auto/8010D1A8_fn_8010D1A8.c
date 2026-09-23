/* auto/8010D1A8_fn_8010D1A8.c - placeholder attribution, 37 function(s), 0x8010D1A8..0x801121DC.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .sdata2 run jump lbl_807969D4 -> lbl_807969D8
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x80026274..0x800263AC  26 labels  proposed    (dataclaim: no queue run)
 *   .data        0x8059F530..0x8059F670   8 labels  proposed    (dataclaim: unowned)
 *   .sdata       0x80791828..0x80791840   3 labels  proposed    (dataclaim: unowned)
 *   .sbss        0x80794974..0x80794B68   2 labels  NOT CLAIMED (dataclaim: lowers-score, unowned)
 *   .sdata2      0x80796838..0x807969D8 101 labels  proposed    (dataclaim: unowned)
 *
 *   The `extab`/`extabindex` fragment sections are the exception: they travel with the code unit
 *   (`dataqueue.py`'s `FRAGMENT_SECTIONS`) and are claimed in `splits.txt` with its `.text`;
 *   `.ctors`/`.dtors` are added after the split. Everything else above waits for the measured pass.
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x800f0000 0x801e0000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-4.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-4.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/8010D1A8_fn_8010D1A8.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
