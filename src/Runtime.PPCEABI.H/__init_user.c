/*
 * Runtime.PPCEABI.H/__init_user.c - the MW runtime user hooks kept in `.text`: `__init_user`, `__init_cpp` and
 *    `exit`, the `.text` half of the SDK's `__ppc_eabi_init` source.
 * RANGE. .text 0x804D7FF0-0x804D80B0 (3 functions, 16-byte function alignment with gap padding): `__init_user`
 *    (0x20), static `__init_cpp` (0x48), weak `exit` (0x4C). `__init_cpp` runs the null-terminated `_ctors` table;
 *    `exit` runs the null-terminated `_dtors` table and calls `PPCHalt`. `_ctors`/`_dtors` are the `.ctors$10`/
 *    `.dtors$10` heads `Runtime.PPCEABI.H/__init_cpp_exceptions.cpp` owns.
 * FLAGS. `cflags_base` (configure.py), Wii/1.3, plus `#pragma optimize_tail_call off` in this file: without it
 *    `__init_user` compiles to a tail `b __init_cpp` (4 B) where retail keeps a frame and `bl` (0x20 B), which
 *    also shifts `exit`. `#pragma opt_tail_call off` has no effect.
 * NAMES. `__init_user`, `__init_cpp`, `exit` are the SDK names (map rows); the file name is a GUESS (the `.text`
 *    half of `__ppc_eabi_init`, whose `.init` half is registered at 0x800065C0, if the two are one source file).
 * RESIDUALS. the last 4 bytes of the `.text` range (0x804D80AC) are alignment padding this object does not
 *    emit; the next unit `PAD/PADClamp.c` (16-byte aligned) supplies them in the link.
 * SHAPES. both table walks are `for (p = table; *p; p++) (*p)();` over `voidfunctionptr`.
 */
#include "types.h"
#include "OS/PPCHalt.h"
#include "Runtime.PPCEABI.H/__init_cpp_exceptions.h"
#include "Runtime.PPCEABI.H/exit.h"

#pragma optimize_tail_call off

void __init_user(void);
static void __init_cpp(void);

/* 0x804D7FF0 (0x20): runs the user-level initialisation (the static constructors). */
void __init_user(void)
{
    __init_cpp();
}

/* 0x804D8010 (0x48): calls every static constructor in `_ctors` until the null terminator. */
static void __init_cpp(void)
{
    voidfunctionptr* constructor;

    for (constructor = _ctors; *constructor; constructor++) {
        (*constructor)();
    }
}

/* 0x804D8060 (0x4C): calls every static destructor in `_dtors`, then halts the processor. */
__declspec(weak) void exit(s32 status)
{
    voidfunctionptr* destructor;

    for (destructor = _dtors; *destructor; destructor++) {
        (*destructor)();
    }
    PPCHalt();
}
