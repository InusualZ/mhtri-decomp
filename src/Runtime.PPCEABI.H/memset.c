/*
 * MSL C runtime: memset, the wrapper the rest of the runtime calls into __fill_mem.
 *
 * .init 0x80004350-0x80004380 - one function, memset, 48 B / 12 instructions, matching the target exactly
 * (fuzzy_match_percent 100.0, matched_code 48/48, R_PPC_REL24 -> __fill_mem at the same offset).
 * Registered NonMatching in configure.py under the Runtime.PPCEABI.H lib (Wii/1.3, cflags_runtime);
 * the two sibling entries in that lib are still stubs with no source.
 *
 * Two load-bearing source shapes, both found by first divergence:
 *   - __fill_mem takes an int, not a byte: an (unsigned char) cast makes the compiler emit an extra
 *     `clrlwi r4, r4, 24` before the call.
 *   - the target holds memset in .init, so the section is forced with __declspec(section ".init");
 *     compiled as plain .text the section pair does not line up and the unit measures as unmatched.
 * Residual: none.
 */

extern void __fill_mem(int dst, int val, unsigned int count);

/* The target object holds memset in .init, not .text, so the section is forced here; without it the
 * section pair (ours .text vs the target's .init) makes objdiff report no match at all. */
__declspec(section ".init") void *memset(void *dst, int val, unsigned int count)
{
    __fill_mem((int)dst, val, count);
    return dst;
}
