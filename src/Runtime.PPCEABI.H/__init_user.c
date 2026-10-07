/*
 * Runtime.PPCEABI.H/__init_user.c - the MW runtime user hooks kept in `.text`: `__init_user`, `__init_cpp` and
 *    `exit`.
 * RANGE. .text 0x804D7FF0-0x804D80B0 (3 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence:
 *    `__init_cpp` reads the `.ctors` head and `exit` the `.dtors` head and `__destroy_global_chain`; no OS
 *    library file reads or defines data here; `fn_804D80B0` (0x804D80B0) is called only by the PAD code.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. GUESS: the file name (it is the `.text` half of `Runtime.PPCEABI.H/__ppc_eabi_init.cpp`, whose `.init` half
 *    is registered at 0x800065C0, if the two are one source file; unproven).
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
