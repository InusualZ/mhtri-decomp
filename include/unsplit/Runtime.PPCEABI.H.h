/*
 * Runtime.PPCEABI.H declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The MSL runtime helper `__construct_array` (0x80455418) is called by units whose range the compiler
 * generated a static array constructor for: `ef/effect.cpp` builds two `nw4r::ef` arrays with it,
 * `sound/fn_800E46E8.cpp` a third, and `light/light.cpp`'s `fn_802BEEE0` the two light work records.
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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_RUNTIME_PPCEABI_H */
