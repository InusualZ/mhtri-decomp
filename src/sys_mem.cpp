/*
 * The C++ allocation group: `operator new` / `operator delete` and the two unnamed helpers beside them.
 *
 * .text 0x80040478-0x80040598 - `__nw__FUl` (0x44, operator new), `fn_800404BC` (0x44), `__dl__FPv` (0x4C,
 * operator delete), `fn_8004054C` (0x4C), in that order and nothing else. Both operators work over the game's
 * single experimental heap (the handle `lbl_80794788`, which `main.cpp` creates with
 * `MEMCreateExpHeapEx`, and the alloc/free wrappers that stay with `main.cpp` call into).
 *
 * Attribution (batch 4, step 1): the unit is defined by its exception-table group rather than by a file name -
 * the four `extab` records at 0x80006770/0x80006798/0x800067C0/0x800067E8 and their `extabindex` entries are
 * exactly these four functions in this order (every one of them has a try/catch), and the call graph is closed
 * (only intra-cluster `bl`s). Its neighbours are cut the same way: `main.cpp` ends at 0x80040478 because
 * `tudiscover at 0x8003F218` puts its right boundary there and the group before it (`fn_80040420`'s
 * exp-heap wrapper) shares `main.cpp`'s globals, while `fn_80040598` (`game_load_rso`, the next unit) is the
 * first function of the following region.
 *
 * The retail source file's name is **not evidenced**: no `Panic(__FILE__, line)` string, no `.data`/`.rodata`
 * reference and no pool label exists anywhere in the range (the scout checked), and the map carries only
 * `__nw__FUl`/`__dl__FPv` plus two `fn_*` placeholders. `sys_mem.cpp` is therefore descriptive, not a
 * recovered name - if the module turns out to be the SDK's `new.cpp`/`delete.cpp` pair or a game memory file,
 * the unit is renamed (map + source in one edit, `symedit.py`).
 *
 * Flags: shares the `main` lib's measured set (`-O3`, `-inline noauto`, no implied `-func_align 16`), because
 * the region packs on 4-byte boundaries with no `lmw`/`stmw` and sits in the same module.
 * Residual: source not written yet, 0 %.
 */
