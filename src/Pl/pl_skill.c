/*
 * Player skill module (Pl_skill): the cluster the previous session split off between pl_master and pl_act.
 * .text 0x80270018-0x80273B14 (50 functions, 0x3AFC B) with its own exception tables - extab
 * 0x800126D4-0x8001280C, extabindex 0x8002F8BC-0x8002FA90.
 *
 * Left edge pinned by the `.sdata2` pool run (`lbl_8079A03C`), right edge by the closure; the reasoning is in
 * configure.py beside the Pl lib entry.
 *
 * Open question for whoever writes this: registered as C - if the range holds mangled names it was C++.
 * Flags: Pl lib (`cflags_base`); probe the -O level per playbook 27 before writing source.
 * Residual: source not written yet, 0 % (attribution goal, docs/plan.md).
 */
