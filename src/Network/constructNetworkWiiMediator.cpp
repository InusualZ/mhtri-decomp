/*
 * Network/constructNetworkWiiMediator.cpp - `constructNetworkWiiMediator`
 * (`.text` 0x80418988..0x804189C8, 64 B; `extab` 0x8001CE34..0x8001CE4C;
 * `extabindex` 0x8003D800..0x8003D80C).
 *
 * BOUNDARY.  One function, both seams weak: `tudiscover at constructNetworkWiiMediator` says the
 * closure is the function alone; the nearest left candidate is 0x80417FD4 (weak, share 0.145) and the
 * right closure edge is 0x804189C8 (weak, share 0.071/0.100).  **Both seams are unproven** - the
 * two calls it makes (`constructNetworkLibrary`, the constructor body) and the `.sbss` slot it publishes
 * (`sNetworkWiiMediatorInstance`, also referenced from outside this range, `leak 1`) suggest the original file is
 * wider.  Sections: `.text` 0x80418988..0x804189C8, `extab` 0x8001CE34..0x8001CE4C,
 * `extabindex` 0x8003D800..0x8003D80C.
 *
 * WHAT IT IS.  The `sNetworkLibrary` mediator constructor thunk: allocate the 0x1408-byte object,
 * construct it, publish it to the singleton slot `sNetworkWiiMediatorInstance`.  The `.sbss` slot is
 * *not* claimed: `leak 1` says another TU references it.
 *
 * LANGUAGE.  The task proposed this unit as `.c`, but the target allocates through `__nw__FUl`, the
 * C++ global `operator new` (defined in `sys_mem.cpp`), which a C translation unit cannot name.  The
 * file is therefore `.cpp`; the function keeps C linkage (`extern "C"`) so the unmangled
 * `constructNetworkWiiMediator` symbol is unchanged.  That also satisfies rule 2/9: the allocator is
 * declared once in `include/sys_mem.h` and called as `operator new`.
 *
 * SECTIONS.  The target object is the 64-byte `.text` *plus* a 24-byte `extab` record and a 12-byte
 * `extabindex` entry, and the record exists because the allocation is a real C++ `new` expression:
 * its last word relocates to `__dl__FPv` (the delete MWCC runs if the constructor throws) and the
 * guarded range it describes ends at 0x28, the `stw` that publishes the result.  The manual
 * `operator new` + `if (p != NULL)` spelling emits the same 16 instructions but only the record's
 * 8-byte header (`08080000 00000000`), which is what `flipcheck.py` reported as "splits.txt claims
 * extab (0x18) but the object emits no such section".  `#pragma exceptions on` - the lib sets
 * `-Cpp_exceptions off` - is what makes MWCC emit the record at all.
 *
 * ALLOCATED TYPE.  `NetworkLibrary` is the 0x1408 bytes the allocation's `li r3,0x1408` sizes; its
 * constructor body is the *neighbouring* 0x804189C8 (0x98 B, outside this range), which stores a
 * vtable at +0x00, initialises the fields from +0x9A up and returns `this` - the shape of a function
 * handed the object in `r3`.  Spelling that call as `constructNetworkLibrary(this)` and as the
 * parameterless `constructNetworkLibrary()` measure identically (`.text`, `extab` and `extabindex`
 * byte-identical both ways), so the class keeps the parameterless declaration
 * `include/unsplit/Network.h` already carries and the map row stays `constructNetworkLibrary`:
 * declaring the constructor out of line instead emits `__ct__14NetworkLibraryFv` and renames another
 * band's symbol for no byte of difference.
 *
 * FLAGS.  The object deviates from the lib on one point, and it is in `configure.py` with the
 * evidence: `-O3` in place of the lib's `-O4,p`.  Retail's order is the plain source order -
 * `li r3,0x1408` lands *after* the two callee-save stores and `cmpwi r3,0` after `mr r31,r3` -
 * while `-O4,p` hoists both ahead of their producers (same 18 instructions, same multiset).
 * Measured: 75.00000 at `-O4,p`, 100.00000 at `-O3`, both with a 64 B `.text`.
 *
 * BODY.  Reconstructed from the disassembly; the 18-instruction shape is exact - the unit is
 * byte-identical in `.text`, `extab` and `extabindex` at `-O3`.
 */
#include "types.h"
#include "sys_mem.h"
#include "unsplit/Network.h"

#pragma exceptions on

extern "C" {

void constructNetworkWiiMediator(void);

}

/* The object the allocation creates.  Its constructor body is the neighbouring 0x804189C8, so the
 * type is owned here while its fields belong to that unit: nothing in this range reads one, and only
 * the size is load-bearing - it is the allocation's own `li r3,0x1408` - so the layout stays one
 * documented filler.
 * size: 0x1408 */
class NetworkLibrary {
public:
    NetworkLibrary() { constructNetworkLibrary(); }

    /* +0x000 */ u8 pad_00[0x1408];
};

void constructNetworkWiiMediator(void)
{
    sNetworkWiiMediatorInstance = new NetworkLibrary();
}
