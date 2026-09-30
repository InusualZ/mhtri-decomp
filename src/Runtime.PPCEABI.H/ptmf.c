/*
 * MSL C++ runtime: pointer-to-member-function support - `__ptmf_test` (0x30) and `__ptmf_scall` (0x28), with the null
 * record `__ptmf_null` in `.rodata` (0x80572428).
 *
 * .text 0x80456C88-0x80456CE0, `.rodata` 0x80572428-0x80572438.  The unit was found from the null record: seven Network
 * functions copy three words out of 0x80572428 to initialise a member-function-pointer field, and the `.rodata` around it
 * belongs to the runtime (`std::exception` typeinfo name at 0x80572418 before it, `__cvt_fp2unsigned`'s constants at
 * 0x80572438 after it), so it is the runtime's `__ptmf_null` rather than a Network descriptor.  Its text sits between
 * `__destroy_arr` and `__cvt_fp2unsigned`.  Attribution: MSL's `Runtime.PPCEABI.H/ptmf.c` (this build has no `__ptmf_cmpr`).
 *
 * RESIDUALS.  The map extent of `__ptmf_null` is 0x10 (alignment padding before the next rodata object); the record is 0xC.
 */

#include "Runtime.PPCEABI.H/ptmf.h"

const __ptmf __ptmf_null = {0, 0, 0};

asm long __ptmf_test(__ptmf* ptmf)
{
    nofralloc
    lwz r5, 0(r3)
    lwz r6, 4(r3)
    lwz r7, 8(r3)
    li r3, 1
    cmpwi r5, 0
    cmpwi cr6, r6, 0
    cmpwi cr7, r7, 0
    bnelr
    bnelr cr6
    bnelr cr7
    li r3, 0
    blr
}

/* untyped: opaque handle passed through - `this` of the member function, forwarded in r3 */
asm long __ptmf_scall(void* self)
{
    nofralloc
    lwz r0, 0(r12)
    lwz r11, 4(r12)
    lwz r12, 8(r12)
    add r3, r3, r0
    cmpwi r11, 0
    blt direct
    lwzx r12, r3, r12
    lwzx r12, r12, r11
direct:
    mtctr r12
    bctr
}
