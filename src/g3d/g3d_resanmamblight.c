/*
 * nw4hbm g3d: an accessor of the ResAnmAmbLight cluster.
 *
 * .text 0x800680A8-0x800680CC - one function, fn_800680A8, 9 instructions: resolve the object through the
 * library's checked helper and read the word at +0xC of the result.
 * The range is provisional - the surrounding functions are unsplit, so this unit covers one function taken
 * so far, and the module was named from evidence rather than a symbol name: the map carries only fn_* here
 * (20 502 of the 20 524 functions outside Camellia/RSO/Runtime do), the Ghidra dump has its own placeholder,
 * the caller at 0x80067EFC asserts out of g3d_anmscn.cpp and the helper this calls asserts out of
 * g3d_resanmamblight_ac.h. The retail TU was C++; the symbol here is unmangled, so the file stays C until
 * more of the TU is reconstructed.
 * Registered NonMatching in configure.py, lib g3d (Wii/1.3, cflags_g3d); the flag evidence sits beside that
 * override in configure.py, the idea is docs/matching.md 27.
 * Measured: fuzzy_match_percent 100.0 - 36 B / 9 instructions, byte-identical.
 * Residual: none now. Under cflags_base's -O4,p the unit was 97.56 % with the same nine instructions in the
 * other order (`lwz r0, 0x14(r1)` before `lwz r3, 0xc(r3)`); -O3 restores the retail order.
 */

extern void *fn_80066C8C(void *obj);

unsigned int fn_800680A8(void *obj)
{
    return *(unsigned int *)((char *)fn_80066C8C(obj) + 0xC);
}
