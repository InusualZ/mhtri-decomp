/*
 * MSL C++ runtime: the global destructor chain - `__register_global_object` (0x18, 6 instructions) and
 * `__destroy_global_chain` (0x48), with the chain head itself in `.sbss` below.
 *
 * .text 0x804566A4-0x80456704, exactly the two named functions (the map has them: `__register_global_object`
 * at 0x804566A4, `__destroy_global_chain` at 0x804566BC, and the next symbol `fn_80456704` starts where the
 * second one ends). Attribution: the SDK's `global_destructor_chain.c` (the same file exists in pikmin2's tree
 * with precisely these three functions - `__register_atexit` is marked UNUSED there and this build has no
 * symbol for it, so it is not claimed here - and its listing is the source shape below), plus the map's own
 * `__global_destructor_chain` at `.sbss:0x80794DF8`.
 *
 * Load-bearing shapes, from the target's own instructions:
 *   - the destructor takes two arguments: the object in r3 and a `-1` in r4 (`li r4,-1` before the `bctrl`),
 *     which is the C++ "not deleting" flag of the D0/D2 destructor ABI - the field is `void (*)(void*, int)`;
 *   - `__destroy_global_chain` re-reads the head *after* stepping it (the `b` to the loop test), so the walk
 *     is a `while` on the global and not a `do/while` on a local;
 *   - `__register_global_object`'s parameter order is (object, destructor, chain): r3 goes to the chain's +0x8,
 *     r4 to +0x4 and r5 is the chain node itself.
 *
 * The chain head (`__global_destructor_chain`, `.sbss:0x80794DF8`, 4 B) is this unit's own data since phase 4
 * (window e): the unit's `.sbss` claim is 0x80794DF8..0x80794E00 and the source defines the word.  The claim is 8 B
 * (the word plus 4 B of padding up to the next unit's 8-aligned start) and the object emits 4 B, so
 * `flipcheck.py` answers `.sbss: object is 0x4, splits.txt claims 0x8`: the unit is `NonMatching` (demoted at phase 4;
 * the linked DOL bytes would be unchanged, the object is not the target's).
 *
 * Flags: `Runtime.PPCEABI.H` lib (`cflags_runtime`, Wii/1.3); both functions are byte-identical under it.
 * Residual: `.sbss` 4 B against the target's 8 B (above); `__register_global_object` 24/24 B and
 * `__destroy_global_chain` 72/72 B at 100 %.
 */

#include "types.h"

typedef struct DestructorChain {
    struct DestructorChain* next;   /* +0x00 */
    void (*destructor)(void*, int); /* +0x04; the int is the ABI's "deleting" flag, -1 here */
    void* object;                   /* +0x08 */
} DestructorChain;

DestructorChain* __global_destructor_chain;

void __register_global_object(void* object, void (*destructor)(void*, int), DestructorChain* chain)
{
    chain->next = __global_destructor_chain;
    chain->destructor = destructor;
    chain->object = object;
    __global_destructor_chain = chain;
}

void __destroy_global_chain(void)
{
    DestructorChain* chain;
    /* The assignment in the condition is load-bearing: retail reuses the pointer its loop test loaded
     * (`lwz r0,0x0(r3)`), and reading the global again in the body costs one extra load (76 B vs 72 B). */
    while ((chain = __global_destructor_chain) != NULL) {
        __global_destructor_chain = chain->next;
        chain->destructor(chain->object, -1);
    }
}
