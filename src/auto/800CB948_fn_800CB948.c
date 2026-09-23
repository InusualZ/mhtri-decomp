/* auto/800CB948_fn_800CB948.c - placeholder attribution, 2 function(s), 0x800CB948..0x800CC5B0.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (source): source file change ef_cylinder.cpp -> ef_disc.cpp
 *   pinned seam (source): source file change ef_disc.cpp -> ef_emform.cpp
 *   pinned seam (pool): .sdata2 run jump lbl_807962AC -> lbl_807962B0
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x80023BC8..0x80023BE0   2 labels  proposed    (dataclaim: no queue run)
 *   .data        0x80594D20..0x80594DCD   4 labels  proposed    (dataclaim: unowned)
 *   .sdata2      0x80796270..0x807962B0  13 labels  proposed    (dataclaim: unowned)
 *
 *   The `extab`/`extabindex` fragment sections are the exception: they travel with the code unit
 *   (`dataqueue.py`'s `FRAGMENT_SECTIONS`) and are claimed in `splits.txt` with its `.text`;
 *   `.ctors`/`.dtors` are added after the split. Everything else above waits for the measured pass.
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x80070000 0x801e0000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-2.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-2.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/800CB948_fn_800CB948.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
