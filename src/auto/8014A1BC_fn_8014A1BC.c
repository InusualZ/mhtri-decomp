/* auto/8014A1BC_fn_8014A1BC.c - placeholder attribution, 60 function(s), 0x8014A1BC..0x801502C8.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (pool): .data run jump jumptable_805A30B0 -> jumptable_805A4020
 *   pinned seam (pool): .sdata2 run jump lbl_80796FD4 -> lbl_80796FD8
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x80028728..0x80028968  48 labels  proposed    (dataclaim: no queue run)
 *   .rodata      0x8056F960..0x8056FA38   3 labels  proposed    (dataclaim: unowned)
 *   .data        0x805A1CC8..0x805A2BD8  47 labels  NOT CLAIMED (dataclaim: unowned)
 *   .bss         0x806585E0..0x8065903C   1 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sbss        0x80794B60..0x80794B68   1 labels  NOT CLAIMED (dataclaim: unowned)
 *   .sdata2      0x80796E0C..0x80796FB8  87 labels  NOT CLAIMED (dataclaim: unowned)
 *
 *   The `extab`/`extabindex` fragment sections are the exception: they travel with the code unit
 *   (`dataqueue.py`'s `FRAGMENT_SECTIONS`) and are claimed in `splits.txt` with its `.text`;
 *   `.ctors`/`.dtors` are added after the split. Everything else above waits for the measured pass.
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x800f0000 0x801e0000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-4.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-4.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/8014A1BC_fn_8014A1BC.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
