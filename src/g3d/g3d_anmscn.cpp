/*
 * nw4r g3d: an accessor of the ResAnmAmbLight cluster.
 *
 * .text 0x800680A8-0x800680CC - one function, 9 instructions: resolve the object through the library's
 * checked helper and read the word at +0xC of the result.
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
 * Language and names.  The retail TU is C++ (nw4r g3d), so the file is `.cpp` and every definition
 * carries the name the C++ front-end emits - **mangled**.  `extern "C"` is not needed and would make the
 * source a lie about its front-end (playbook 48/42): the symbol map is a build input, not a description
 * of the original's symbol table, so it is renamed to the mangled spelling (playbook 31).  Here
 * `fn_800680A8` -> `fn_800680A8__FPv`, derived with `tools/units/mangle.py` from the reconstructed
 * signature (`unsigned int fn_800680A8(void *obj)`) and confirmed against the symbol this source emits.
 *
 * The callee `fn_80066C8C` (0x80066C8C..0x80066CF0) is C++ too, and is **defined in an unsplit unit** -
 * the extracted object for that region takes its symbol name from the map - so its map symbol is
 * renamed to `fn_80066C8C__FPv` in the same change, and the unit's other caller
 * (`g3d_resanmamblight.c`) declares that same mangled spelling in the same change: our relocation, the
 * map name and the defining object's symbol have to agree or the link fails.  Its signature is read off
 * the retail body (a pointer in, the resolved resource out, `Panic("g3d_resanmamblight_ac.h", 37, ...)`
 * when the library's validity check rejects it); the map stem is kept, as no better name is evidenced.
 *
 * Registered Matching in configure.py, lib g3d (Wii/1.3, cflags_g3d); the flag evidence sits beside that
 * override in configure.py, the idea is playbook 27.
 * Measured: fuzzy_match_percent 100.0 - 36 B / 9 instructions, byte-identical (`.text`, `extab` and
 * `extabindex` byte for byte against the retail split object; the rename changes no allocatable byte).
 * Residual: none now. Under cflags_base's -O4,p the unit was 97.56 % with the same nine instructions in the
 * other order (`lwz r0, 0x14(r1)` before `lwz r3, 0xc(r3)`); -O3 restores the retail order.
 *
 * Naming note: the map carries only the `fn_XXXXXXXX` stems for this range, and its renamed
 * `fn_800680A8__FPv`/`fn_80066C8C__FPv` are still those stems plus the front-end's argument list (playbook 48).
 * The source must keep the bare stem as its identifier - writing the suffix as the identifier would mangle it
 * a second time, the link failure of playbook 50.
 */

#include "types.h"
#include "g3d/fn_80063888.h" /* fn_80066C8C - the owner is g3d/fn_80063888.cpp (rule 2); its map name
                              is the mangling `fn_80066C8C__FPv`, so the declaration keeps C++ linkage. */

unsigned int fn_800680A8(void *obj)
{
    return *(unsigned int *)((char *)fn_80066C8C(obj) + 0xC);
}
