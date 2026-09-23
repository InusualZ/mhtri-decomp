/* auto/800BFFD4_fn_800BFFD4.c - placeholder attribution, 118 function(s), 0x800BFFD4..0x800C9540.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (source): source file change ef_drawstrategyimpl.cpp -> ef_torus.cpp
 *   pinned seam (source): source file change ef_torus.cpp -> ef_cube.cpp
 *   pinned seam (pool): .sdata2 run jump lbl_80796238 -> lbl_80796240
 *   WARNING: over --max-bytes with no legal cut - kept whole
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x80023904..0x80023BA4  56 labels  proposed    (dataclaim: no queue run)
 *   .data        0x805944E0..0x80594BA7  40 labels  proposed    (dataclaim: unowned)
 *   .bss         0x806945B8..0x80694C68  16 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sbss        0x80794928..0x80794937  11 labels  proposed    (dataclaim: unowned)
 *   .sdata2      0x807961C0..0x80796208  14 labels  proposed    (dataclaim: unowned)
 *
 *   The `extab`/`extabindex` fragment sections are the exception: they travel with the code unit
 *   (`dataqueue.py`'s `FRAGMENT_SECTIONS`) and are claimed in `splits.txt` with its `.text`;
 *   `.ctors`/`.dtors` are added after the split. Everything else above waits for the measured pass.
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x80070000 0x801e0000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-2.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-2.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/800BFFD4_fn_800BFFD4.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
