/*
 * Network/constructNetworkWiiMediator.cpp - `constructNetworkWiiMediator`
 * (`.text` 0x80418988..0x804189C8, 64 B).
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
 * construct it (`constructNetworkLibrary`), publish it to the singleton slot `sNetworkWiiMediatorInstance`.  The `.sbss` slot is
 * *not* claimed: `leak 1` says another TU references it.
 *
 * LANGUAGE.  The task proposed this unit as `.c`, but the target allocates through `__nw__FUl`, the
 * C++ global `operator new` (defined in `sys_mem.cpp`), which a C translation unit cannot name.  The
 * file is therefore `.cpp`; the function keeps C linkage (`extern "C"`) so the unmangled
 * `constructNetworkWiiMediator` symbol is unchanged.  That also satisfies rule 2/9: the allocator is
 * declared once in `include/sys_mem.h` and called as `operator new`.
 *
 * The range's one owned symbol already has its real name (`constructNetworkWiiMediator`); the one
 * callee it makes (`constructNetworkLibrary`) and the singleton slot it publishes into
 * (`sNetworkWiiMediatorInstance`) are named from their use here, so no generated spelling survives
 * in this file.
 *
 * BODY.  Reconstructed from the disassembly; the 4-instruction shape is exact bar the alloc call.
 */
#include "types.h"
#include "sys_mem.h"
#include "unsplit/Network.h"

extern "C" {

void constructNetworkWiiMediator(void);

}

void constructNetworkWiiMediator(void)
{
    void* object = operator new(0x1408);

    if (object != NULL) {
        constructNetworkLibrary();
    }
    sNetworkWiiMediatorInstance = object;
}
