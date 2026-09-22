/*
 * Player master module (Pl_master): the actor-master cluster an earlier session carved out of the auto_*
 * scaffolding. .text 0x8026BA1C-0x8026FFBC (24 functions, 0x45A0 B) with its own exception tables - extab
 * 0x8001265C-0x800126CC, extabindex 0x8002F808-0x8002F8B0.
 *
 * Right edge pinned by the .sdata2 run jump `lbl_8079A02C -> lbl_8079A030`; left edge is the closure edge, and
 * `fn_8026FFBC` (0x8026FFBC-0x80270018) sits on the ambiguous side of that seam and is deliberately left
 * unclaimed rather than guessed in - the reasoning is in configure.py beside the Pl lib entry.
 *
 * Open question for whoever writes this: the unit is registered as C, but if the range holds mangled names it
 * was C++ and the extension has to change (a rename is configure.py + splits.txt + the source together).
 * Flags: Pl lib (`cflags_base`); the -O level is a per-unit property and has to be probed (playbook 27).
 * Residual: source not written yet, 0 % - the unit exists so its symbols have a home (docs/plan.md, the
 * attribution goal); its functions follow in address order.
 */
