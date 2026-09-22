/*
 * MSL C++ runtime: the exception initialisation pair. .text 0x80457420-0x80457490 (2 functions, 0x70 B) -
 * `__init_cpp_exceptions` (0x3C) then `__fini_cpp_exceptions` (0x34) - plus the fragments a C++ runtime unit
 * owns: .ctors$10 0x8056F2C0-0x8056F2C4, .dtors$10 0x8056F440-0x8056F444, .dtors$15 0x8056F444-0x8056F448 and
 * .sdata 0x80793CC8-0x80793CCC, all four registered with their `rename:` forms in splits.txt.
 *
 * Attribution: the SDK's `Runtime.PPCEABI.H/__init_cpp_exceptions.c`, in its Wii revision - the one whose
 * `__exception_info_constants` helper carries `asm { mr temp, r2; }` and is inlined, so retail reads `mr r4, r2`
 * with no `bl GetR2` and no helper symbol. The GCN revision pikmin2/prime carry (an out-of-line `GetR2__Fv`)
 * is 4 bytes longer per call site and is not this build; MotoGP's Wii `__init_cpp_exceptions` is byte-identical
 * to ours (0x3C, same five relocations).
 *
 * `.sdata 0x80793CC8` is this unit's own `fragmentID`: the target object defines it as a *local* 4-byte symbol
 * (0xFFFFFFFE) in `.sdata`, so it is a `static int fragmentID = -2;` here. Declaring it `extern` would leave
 * our object without the section the target has, and the fini path would still be the only reader. That closes
 * the "may belong to global_destructor_chain.c" question - the chain head is `.sbss 0x80794DF8`, a different
 * symbol in a different unit.
 *
 * Load-bearing source shapes:
 *   - `#pragma section const_type ".ctors$10"` + `__declspec(section ".ctors$10")` per reference word:
 *     `__declspec(section ...)` alone is rejected (33048) unless the name was registered first, and only the
 *     `#pragma section <kind> "<name>"` form can spell a `$`-suffixed name. `const_type` is the kind for these
 *     read-only pointer words; `code_type` would add a `.mwcats.*` companion section.
 *   - `static inline` on `__exception_info_constants`: without `inline` MWCC emits it out of line and `.text`
 *     becomes 0x94 (0x20 B helper + padding) instead of 0x70.
 *   - `(void*)_eti_init_info` (array decay, i.e. the address) is what produces the ADDR16_HA/ADDR16_LO pair.
 *   - `if (fragmentID != -2)` in fini keeps the loaded value live as `__unregister_fragment`'s argument (r3).
 *
 * Residual: both functions are 100 % (0x3C/0x34, 15/13 instructions, relocations identical). The one remaining
 * difference is a 4-byte `.long 0x0` MWCC inserts before `__fini_cpp_exceptions` (our `.text` 0x74 vs the
 * target's 0x70): `-O4,p` implies `-func_align 16`. Probing `-func_align 4` after `-O4,p` on the unit's own
 * command line gives `.text` 0x70 with `__fini_cpp_exceptions` at 0x3C and both functions still at 100 % - this
 * is the parked lib-level `-func_align 4` item (global_destructor_chain.o pads the same way, 0x20 vs 0x18), and
 * it cannot be worked around from the source: `#pragma func_align 4` is not honoured by this compiler.
 * Flags: Runtime.PPCEABI.H lib (`cflags_runtime`, Wii/1.3), unchanged.
 */

#include "types.h"

typedef struct __eti_init_info {
    void* eti_start;
    void* eti_end;
    void* code_start;
    unsigned long code_size;
} __eti_init_info;

extern __eti_init_info _eti_init_info[];

extern "C" {
int __register_fragment(__eti_init_info* info, char* r2);
void __unregister_fragment(int fragment);
void __destroy_global_chain(void);
}

static int fragmentID = -2;

static inline void __exception_info_constants(void** info, char** R2)
{
    register char* temp;
    asm { mr temp, r2; }
    *R2 = temp;
    *info = (void*)_eti_init_info;
}

extern "C" void __init_cpp_exceptions(void)
{
    char* R2;
    void* info;
    if (fragmentID == -2) {
        __exception_info_constants(&info, &R2);
        fragmentID = __register_fragment((__eti_init_info*)info, R2);
    }
}

extern "C" void __fini_cpp_exceptions(void)
{
    if (fragmentID != -2) {
        __unregister_fragment(fragmentID);
        fragmentID = -2;
    }
}

#pragma section const_type ".ctors$10"
__declspec(section ".ctors$10")
extern void* const __init_cpp_exceptions_reference = (void*)__init_cpp_exceptions;

#pragma section const_type ".dtors$10"
__declspec(section ".dtors$10")
extern void* const __destroy_global_chain_reference = (void*)__destroy_global_chain;

#pragma section const_type ".dtors$15"
__declspec(section ".dtors$15")
extern void* const __fini_cpp_exceptions_reference = (void*)__fini_cpp_exceptions;
