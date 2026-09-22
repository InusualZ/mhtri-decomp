/*
 * Player action module (Pl_act): the largest of the three Pl clusters. .text 0x80276B58-0x8027D684
 * (115 functions, 0x6B2C B) with its own exception tables - extab 0x8001294C-0x80012B54, extabindex
 * 0x8002FC70-0x8002FF7C.
 *
 * Left edge pinned by the `.sdata2` pool run (`lbl_8079A0AC`), right edge by the closure; the reasoning is in
 * configure.py beside the Pl lib entry.
 *
 * Open question for whoever writes this: registered as C - if the range holds mangled names it was C++.
 * Flags: Pl lib (`cflags_base`); probe the -O level per playbook 27 before writing source.
 * Residual: source not written yet, 0 % (attribution goal, docs/plan.md).
 */
