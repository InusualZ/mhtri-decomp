/*
 * Runtime.PPCEABI.H declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The MSL runtime helper `__construct_array` (0x80455418) is called by units whose range the compiler
 * generated a static array constructor for: `ef/effect.cpp` builds two `nw4r::ef` arrays with it,
 * the `sound` band a third, and `light`'s two light work records a fourth and fifth.
 * The registered bands bracketing its address name `Runtime.PPCEABI.H` on both sides, and the module
 * has no `include/unsplit/`-visible owner for it (the module's registered units are `memcpy.c`,
 * `memset.c`, `__start.c`, `__ppc_eabi_init.cpp`, `global_destructor_chain.c` and
 * `__init_cpp_exceptions.cpp`, none of which defines it), so this file is its home.  The three callers
 * disagree on the middle parameters only in name and width; the map's name is unmangled, so the
 * declaration is C linkage.
 *
 * Added with the `light/light.cpp` registration.
 */
#ifndef MHTRI_UNSPLIT_RUNTIME_PPCEABI_H
#define MHTRI_UNSPLIT_RUNTIME_PPCEABI_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80455418 - constructs `count` elements of `size` bytes at `array` with `ctor`, then registers
 * `dtor` for them. */
void __construct_array(void* array, void* ctor, void* dtor, u32 size, u32 count);

/* The MSL C string/conversion helpers the SDK links instead of a `stdlib` (`-nosyspath` leaves the
 * prototypes to the unit that calls them).  They sit in the MSL `.text` band 0x8045xxxx below
 * `Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp` and are owned by no registered unit; the bracketing
 * range below names another module (a game unit), so stylelint's rule 2 reports them as an
 * unplaceable gap rather than a module - the module is the runtime this header already stands for,
 * and the registered `memcpy`/`memset` string helpers live in the same library.  Moved here from
 * `src/DWCi/fn_805113B0.c` and `src/DWCi/DWCi_NatNeg.c` with the networking conformance pass; the
 * other units that still declare them locally (`src/g3d/g3d_resanmtexsrt.cpp`,
 * `src/homebutton/keyboard_ui.cpp`, `src/light/light.cpp`, `src/g3d/g3d_anmchr.cpp`) can adopt this
 * header when they are next touched. */
u32 strlen(const char* s);                    /* 0x804565C0 */
int sprintf(char* dst, const char* fmt, ...); /* 0x8045DECC */
char* strchr(const char* s, int c);           /* 0x8045F7E0 */
int atoi(const char* s);                      /* 0x804616B4 */
char* strcpy(char* dst, const char* src);     /* 0x8045F554, added with `menu/arena_result.cpp` */
char* strcat(char* dst, const char* src);     /* 0x8045F658, likewise */
int printf(const char* fmt, ...);          /* 0x8045EDBC, added with `NHTTP/NHTTP_bgnend.c` */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_RUNTIME_PPCEABI_H */
