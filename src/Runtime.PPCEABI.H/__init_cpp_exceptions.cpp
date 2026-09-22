/*
 * MSL C++ runtime: the exception initialisation pair. .text 0x80457420-0x80457490 (2 functions, 0x70 B) -
 * `__init_cpp_exceptions` (0x3C) then `__fini_cpp_exceptions` (0x34) - plus the fragments a C++ runtime unit
 * owns: .ctors$10 0x8056F2C0-0x8056F2C4, .dtors$10 0x8056F440-0x8056F444, .dtors$15 0x8056F444-0x8056F448 and
 * .sdata 0x80793CC8-0x80793CCC, all four registered with their `rename:` forms in splits.txt.
 *
 * Allocated by an earlier session; the fragments are the C++-runtime fingerprint (the .ctors/.dtors pairs with
 * their $10/$15 ordering and the .sdata word the fini path walks).
 *
 * Open question: `Runtime.PPCEABI.H/global_destructor_chain.c` is registered in configure.py with no range and
 * no source, and the `.sdata 0x80793CC8` word may belong to it rather than here (MSL keeps the global
 * destructor chain in its own file) - settle it when either function is written, with a measurement.
 * Flags: Runtime.PPCEABI.H lib (`cflags_runtime`, Wii/1.3).
 * Residual: source not written yet, 0 % (attribution goal, docs/plan.md).
 */
