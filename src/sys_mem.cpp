/*
 * The C++ allocation group: `operator new` / `operator new[]` / `operator delete` and the unnamed helper beside them.
 *
 * .text 0x80040478-0x80040598 - `__nw__FUl` (0x44, operator new), `__nwa__FUl` (0x44, operator new[]: the
 * standard new/new[]/delete/delete[] order and a body identical to `operator new`'s), `__dl__FPv` (0x4C,
 * operator delete), `fn_8004054C` (0x4C), in that order and nothing else. Both operators work over the game's
 *
 * `fn_8004054C` must keep its `__declspec(export)`: it is the object's trailing function and nothing in the
 * object references it, and the linker trims an unreferenced tail. The target object's `.comment` marks every
 * entry `active_flags=0x08` (dtk's `dol split` writes `export_all: true`) while MWCC writes `0x00`, so without
 * the export our object is 0x4C shorter and every later section shifts by exactly that - the failure that kept
 * this unit from flipping. Measured three ways in `.pi/notes/` (FORCEACTIVE, and swapping `.comment` each way).
 * single experimental heap (the handle `lbl_80794788`, which `main.cpp` creates with
 * `MEMCreateExpHeapEx`, and the alloc/free wrappers that stay with `main.cpp` call into).
 *
 * Attribution (batch 4, step 1): the unit is defined by its exception-table group rather than by a file name -
 * the four `extab` records at 0x80006770/0x80006798/0x800067C0/0x800067E8 and their `extabindex` entries are
 * exactly these four functions in this order (each is the specification check of a `throw()` function), and
 * the call graph is closed (only intra-cluster `bl`s). Its neighbours are cut the same way: `main.cpp` ends at
 * 0x80040478 because `tudiscover at 0x8003F218` puts its right boundary there and the group before it
 * (`fn_80040420`'s exp-heap wrapper) shares `main.cpp`'s globals, while `fn_80040598` (`game_load_rso`, the
 * next unit) is the first function of the following region.
 *
 * The retail source file's name is **not evidenced**: no `Panic(__FILE__, line)` string, no `.data`/`.rodata`
 * reference and no pool label exists anywhere in the range (the scout checked), and the map carries only
 * `__nw__FUl`/`__dl__FPv` plus two `fn_*` placeholders. `sys_mem.cpp` is therefore descriptive, not a
 * recovered name - if the module turns out to be the SDK's `new.cpp`/`delete.cpp` pair or a game memory file,
 * the unit is renamed (map + source in one edit, `symedit.py`).
 *
 * Load-bearing source shapes (batch 4, step 2 - each measured against `build/RMHE08/obj/sys_mem.o`):
 *   * all four functions are declared **`throw()`**, an empty exception specification. That single fact is the
 *     whole shape of the object: MWCC wraps each body in its implicit specification check, which is what
 *     produces the 48-byte frame-pointer prologue (`mr r31,r1`), the out-of-line handler
 *     (`addi r3,r31,8` + `bl __unexpected` + a self-branch) and the back-chain epilogue. The extab and
 *     extabindex records it emits are byte-identical (0xa0/0x30 B, both sections at 100 %).
 *   * `-Cpp_exceptions on`: `cflags_main` carries it, and flags-audit 2026-09-28 removed the now-redundant
 *     per-file pragma. Under `-Cpp_exceptions off` `throw()` is accepted but silently ignored - no handler,
 *     no extab, and a 4-byte tail-`b` body; the lib flag is what makes the records appear and touches nothing
 *     outside this file.
 *   * `#pragma peephole off` is required too, and for one instruction: with the peephole on, MWCC folds the
 *     epilogue's `lwz r31,44(r10)` base back to r31 (leaving the preceding `mr r10,r31` dead in our object);
 *     the target keeps the r10 form in all four functions. `-opt nopeephole` on the command line does the
 *     same, if this ever becomes a per-unit flag instead.
 *   * the frame holds nothing else - no locals and no MWExceptionInfo slot (`stw r1,X(r31)`), unlike a real
 *     `try`/`catch`, where MWCC emits one. A specification check never needs to unwind, which is exactly what
 *     separates this shape from a catch-based one: probed `try`/`catch(...)` (empty and non-empty),
 *     `catch(...){throw;}`, a typed catch and a destructor cleanup each give a different extab record, a
 *     bigger frame and an `__end__catch` call.
 *
 * Flags: the `main` lib's measured set (`-O3`, `-inline noauto`) - **no change**, and this unit is a second
 * independent witness for it. Probed on this source against the target's 288-byte `.text`: `-O4,p` and `-O4`
 * grow it to 316 B (both imply `-func_align 16`, so the four functions get 16-byte padding, first difference
 * at 0x44), and `-O2` keeps the size but rewrites `__dl__FPv`'s test (`cmpwi`/`beq` at 0xa0). `-inline auto`,
 * `-use_lmw_stmw on` and `-opt nopeephole` reproduce the bytes exactly. The two pragmas above carry the only
 * two settings this unit needs on top of `cflags_main`.
 * Residual (batch 4, step 2): all four functions are instruction-identical and correctly sized (17/17 and
 * 19/19 instructions, 0x44/0x44/0x4C/0x4C B) at 99.71 % / 99.74 %, and extab/extabindex are 100 %. The single
 * unmatched row in each is the handler's `bl`: our object calls it by the name the compiler hardcodes for a
 * specification violation, `__unexpected`, while the map still calls the target `fn_8045835C`. The function at
 * 0x8045835C is MSL's `__unexpected` (its region's neighbours match pikmin2's `Gecko_ExceptionPPC.cp` sizes
 * exactly: 0x50C = `ExPPC_UnwindStack`, 0x104 = `ExPPC_LongJump`, 0x408 = `ExPPC_ThrowHandler`), so renaming
 * the map symbol to `__unexpected` - one `symedit.py rename`, no source edit needed, the name is compiler-
 * generated - closes all four to 100 %.
 */

#pragma peephole off

/* The allocator pair is owned by `main.cpp` (rule 2); its declarations live in include/main.h. */
#include "main.h"

void* operator new(unsigned long size) throw()
{
    return fn_80040420(size);
}

void* operator new[](unsigned long size) throw()
{
    return fn_80040420(size);
}

void operator delete(void* ptr) throw()
{
    if (ptr) {
        fn_80040460(ptr);
    }
}

extern "C" __declspec(export) void fn_8004054C(void* ptr) throw()
{
    if (ptr) {
        fn_80040460(ptr);
    }
}
