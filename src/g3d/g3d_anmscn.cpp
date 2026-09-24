/*
 * nw4r g3d: an accessor of the ResAnmAmbLight cluster.
 *
 * .text 0x800680A8-0x800680CC - one function, fn_800680A8, 9 instructions: resolve the object through the
 * library's checked helper and read the word at +0xC of the result.
 *
 * Name: the unit was registered as `g3d/g3d_resanmamblight.c` from a guess - the helper it calls
 * (fn_80066C8C) asserts out of `g3d_resanmamblight_ac.h`, and a header string only says the caller
 * *includes* that header, not that it is that .cpp.  The wQ-recut corrected it: the retail
 * `.extabindex`/`.data` layout puts this function inside the `g3d_anmscn.cpp` translation unit - its
 * neighbours fn_80067EFC and fn_800680CC both cite `g3d_anmscn.cpp` (lbl_8058C288), and the region's
 * data fragment 0x8058C288-0x8058CC40 opens with that string and holds exactly the header cluster
 * g3d_anmscn.cpp includes (g3d_resanmamblight_ac.h among them).  The real `g3d_resanmamblight.cpp`
 * (0x80089F94-0x8008A220) is now a separate unit, so this one takes its own TU's name.  The range is
 * provisional - the surrounding functions are unsplit, so this unit covers one function taken so far.
 *
 * Registered Matching in configure.py, lib g3d (Wii/1.3, cflags_g3d); the flag evidence sits beside that
 * override in configure.py, the idea is docs/matching.md 27.
 * Measured: fuzzy_match_percent 100.0 - 36 B / 9 instructions, byte-identical.
 * Residual: none now. Under cflags_base's -O4,p the unit was 97.56 % with the same nine instructions in the
 * other order (`lwz r0, 0x14(r1)` before `lwz r3, 0xc(r3)`); -O3 restores the retail order.
 *
 * rule 7 deferred: the map carries only `fn_800680A8`/`fn_80066C8C` here (docs/plan.md 6.5 rule 7);
 * renaming a symbol needs the map and the source in one edit (playbook 31).
 */

/* The retail TU is C++ (nw4r g3d); `extern "C"` keeps the map's unmangled `fn_800680A8`. */
extern "C" void *fn_80066C8C(void *obj);

extern "C" unsigned int fn_800680A8(void *obj)
{
    return *(unsigned int *)((char *)fn_80066C8C(obj) + 0xC);
}
