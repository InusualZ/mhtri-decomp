/* Runtime.PPCEABI.H/runtime - MSL's PowerPC EABI runtime helpers (runtime.c): the double-to-unsigned
 *   conversion, the out-of-line FPR/GPR save and restore entry points, and the 64-bit integer helpers.
 * RANGE. .text 0x80456CE0-0x80457420 (13 functions, 72 `entry` points); .rodata 0x80572438-0x80572450 (`__constants`).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (`cflags_runtime` + `-func_align 4`, Wii/1.3, configure.py);
 *   `-func_align 4` is load-bearing: the 13 functions sit back to back on 4-byte boundaries. Every body is `asm` +
 *   `nofralloc`, so the optimizer flags do not reach the instructions.
 * NAMES. every function, entry point and `__constants` is MSL runtime.c's own name; the branch labels are runtime.c's.
 * RESIDUALS. none.
 * SHAPES. the file compiles as C++: every definition sits in `extern "C"` so the names stay unmangled, and each
 *   `entry` label needs a prototype in that block (MWCC refuses an undeclared entry name). `__constants` is a file
 *   `static` (the target's symbol is local); `__cvt_dbl_ull`'s overflow test is `ble+` (the static prediction bit is
 *   in the target's word); `__mod2u` keeps its second `blr` behind its own label (the `bgt` target). The four
 *   save/restore functions are `__declspec(export)`, the force-active flag the target's `.comment` carries for them
 *   (playbook 36; the linked DOL hashes the same with or without it, their `entry` points keep them alive).
 */

#include "types.h"

extern "C" {
void _savefpr_14(void);
void _savefpr_15(void);
void _savefpr_16(void);
void _savefpr_17(void);
void _savefpr_18(void);
void _savefpr_19(void);
void _savefpr_20(void);
void _savefpr_21(void);
void _savefpr_22(void);
void _savefpr_23(void);
void _savefpr_24(void);
void _savefpr_25(void);
void _savefpr_26(void);
void _savefpr_27(void);
void _savefpr_28(void);
void _savefpr_29(void);
void _savefpr_30(void);
void _savefpr_31(void);
void _restfpr_14(void);
void _restfpr_15(void);
void _restfpr_16(void);
void _restfpr_17(void);
void _restfpr_18(void);
void _restfpr_19(void);
void _restfpr_20(void);
void _restfpr_21(void);
void _restfpr_22(void);
void _restfpr_23(void);
void _restfpr_24(void);
void _restfpr_25(void);
void _restfpr_26(void);
void _restfpr_27(void);
void _restfpr_28(void);
void _restfpr_29(void);
void _restfpr_30(void);
void _restfpr_31(void);
void _savegpr_14(void);
void _savegpr_15(void);
void _savegpr_16(void);
void _savegpr_17(void);
void _savegpr_18(void);
void _savegpr_19(void);
void _savegpr_20(void);
void _savegpr_21(void);
void _savegpr_22(void);
void _savegpr_23(void);
void _savegpr_24(void);
void _savegpr_25(void);
void _savegpr_26(void);
void _savegpr_27(void);
void _savegpr_28(void);
void _savegpr_29(void);
void _savegpr_30(void);
void _savegpr_31(void);
void _restgpr_14(void);
void _restgpr_15(void);
void _restgpr_16(void);
void _restgpr_17(void);
void _restgpr_18(void);
void _restgpr_19(void);
void _restgpr_20(void);
void _restgpr_21(void);
void _restgpr_22(void);
void _restgpr_23(void);
void _restgpr_24(void);
void _restgpr_25(void);
void _restgpr_26(void);
void _restgpr_27(void);
void _restgpr_28(void);
void _restgpr_29(void);
void _restgpr_30(void);
void _restgpr_31(void);

/* 0.0, 2^32 and 2^31, the bounds of `__cvt_fp2unsigned`. */
static const u32 __constants[] = {0x00000000, 0x00000000, 0x41F00000, 0x00000000, 0x41E00000, 0x00000000};

/* Converts a double to an unsigned 32-bit integer, saturating to 0 and 0xFFFFFFFF. */
asm unsigned long __cvt_fp2unsigned(register double d)
{
    nofralloc
    stwu r1,-16(r1)
    lis r4,__constants@ha
    addi r4,r4,__constants@l
    li r3,0
    lfd f0,0(r4)
    lfd f3,8(r4)
    lfd f4,16(r4)
    fcmpu cr0,f1,f0
    fcmpu cr6,f1,f3
    blt @exit
    addi r3,r3,-1
    bge cr6,@exit
    fcmpu cr7,f1,f4
    fmr f2,f1
    blt cr7,@1
    fsub f2,f1,f4
@1:
    fctiwz f2,f2
    stfd f2,8(r1)
    lwz r3,12(r1)
    blt cr7,@exit
    addis r3,r3,-32768
@exit:
    addi r1,r1,16
    blr
}

/* Saves f14..f31 below the frame pointer in r11; `_savefpr_N` enters at register N. */
__declspec(export) asm void __save_fpr(void)
{
    nofralloc
    entry _savefpr_14
    stfd f14,-144(r11)
    entry _savefpr_15
    stfd f15,-136(r11)
    entry _savefpr_16
    stfd f16,-128(r11)
    entry _savefpr_17
    stfd f17,-120(r11)
    entry _savefpr_18
    stfd f18,-112(r11)
    entry _savefpr_19
    stfd f19,-104(r11)
    entry _savefpr_20
    stfd f20,-96(r11)
    entry _savefpr_21
    stfd f21,-88(r11)
    entry _savefpr_22
    stfd f22,-80(r11)
    entry _savefpr_23
    stfd f23,-72(r11)
    entry _savefpr_24
    stfd f24,-64(r11)
    entry _savefpr_25
    stfd f25,-56(r11)
    entry _savefpr_26
    stfd f26,-48(r11)
    entry _savefpr_27
    stfd f27,-40(r11)
    entry _savefpr_28
    stfd f28,-32(r11)
    entry _savefpr_29
    stfd f29,-24(r11)
    entry _savefpr_30
    stfd f30,-16(r11)
    entry _savefpr_31
    stfd f31,-8(r11)
    blr
}

/* Restores f14..f31 from below the frame pointer in r11; `_restfpr_N` enters at register N. */
__declspec(export) asm void __restore_fpr(void)
{
    nofralloc
    entry _restfpr_14
    lfd f14,-144(r11)
    entry _restfpr_15
    lfd f15,-136(r11)
    entry _restfpr_16
    lfd f16,-128(r11)
    entry _restfpr_17
    lfd f17,-120(r11)
    entry _restfpr_18
    lfd f18,-112(r11)
    entry _restfpr_19
    lfd f19,-104(r11)
    entry _restfpr_20
    lfd f20,-96(r11)
    entry _restfpr_21
    lfd f21,-88(r11)
    entry _restfpr_22
    lfd f22,-80(r11)
    entry _restfpr_23
    lfd f23,-72(r11)
    entry _restfpr_24
    lfd f24,-64(r11)
    entry _restfpr_25
    lfd f25,-56(r11)
    entry _restfpr_26
    lfd f26,-48(r11)
    entry _restfpr_27
    lfd f27,-40(r11)
    entry _restfpr_28
    lfd f28,-32(r11)
    entry _restfpr_29
    lfd f29,-24(r11)
    entry _restfpr_30
    lfd f30,-16(r11)
    entry _restfpr_31
    lfd f31,-8(r11)
    blr
}

/* Saves r14..r31 below the frame pointer in r11; `_savegpr_N` enters at register N. */
__declspec(export) asm void __save_gpr(void)
{
    nofralloc
    entry _savegpr_14
    stw r14,-72(r11)
    entry _savegpr_15
    stw r15,-68(r11)
    entry _savegpr_16
    stw r16,-64(r11)
    entry _savegpr_17
    stw r17,-60(r11)
    entry _savegpr_18
    stw r18,-56(r11)
    entry _savegpr_19
    stw r19,-52(r11)
    entry _savegpr_20
    stw r20,-48(r11)
    entry _savegpr_21
    stw r21,-44(r11)
    entry _savegpr_22
    stw r22,-40(r11)
    entry _savegpr_23
    stw r23,-36(r11)
    entry _savegpr_24
    stw r24,-32(r11)
    entry _savegpr_25
    stw r25,-28(r11)
    entry _savegpr_26
    stw r26,-24(r11)
    entry _savegpr_27
    stw r27,-20(r11)
    entry _savegpr_28
    stw r28,-16(r11)
    entry _savegpr_29
    stw r29,-12(r11)
    entry _savegpr_30
    stw r30,-8(r11)
    entry _savegpr_31
    stw r31,-4(r11)
    blr
}

/* Restores r14..r31 from below the frame pointer in r11; `_restgpr_N` enters at register N. */
__declspec(export) asm void __restore_gpr(void)
{
    nofralloc
    entry _restgpr_14
    lwz r14,-72(r11)
    entry _restgpr_15
    lwz r15,-68(r11)
    entry _restgpr_16
    lwz r16,-64(r11)
    entry _restgpr_17
    lwz r17,-60(r11)
    entry _restgpr_18
    lwz r18,-56(r11)
    entry _restgpr_19
    lwz r19,-52(r11)
    entry _restgpr_20
    lwz r20,-48(r11)
    entry _restgpr_21
    lwz r21,-44(r11)
    entry _restgpr_22
    lwz r22,-40(r11)
    entry _restgpr_23
    lwz r23,-36(r11)
    entry _restgpr_24
    lwz r24,-32(r11)
    entry _restgpr_25
    lwz r25,-28(r11)
    entry _restgpr_26
    lwz r26,-24(r11)
    entry _restgpr_27
    lwz r27,-20(r11)
    entry _restgpr_28
    lwz r28,-16(r11)
    entry _restgpr_29
    lwz r29,-12(r11)
    entry _restgpr_30
    lwz r30,-8(r11)
    entry _restgpr_31
    lwz r31,-4(r11)
    blr
}

/* Divides the unsigned 64-bit r3:r4 by r5:r6 and returns the quotient in r3:r4. */
asm void __div2u(void)
{
    nofralloc
    cmpwi r3,0
    cntlzw r0,r3
    cntlzw r9,r4
    bne lab1
    addi r0,r9,32
lab1:
    cmpwi r5,0
    cntlzw r9,r5
    cntlzw r10,r6
    bne lab2
    addi r9,r10,32
lab2:
    cmpw r0,r9
    subfic r10,r0,64
    bgt lab9
    addi r9,r9,1
    subfic r9,r9,64
    add r0,r0,r9
    subf r9,r9,r10
    mtctr r9
    cmpwi r9,32
    addi r7,r9,-32
    blt lab3
    srw r8,r3,r7
    li r7,0
    b lab4
lab3:
    srw r8,r4,r9
    subfic r7,r9,32
    slw r7,r3,r7
    or r8,r8,r7
    srw r7,r3,r9
lab4:
    cmpwi r0,32
    addic r9,r0,-32
    blt lab5
    slw r3,r4,r9
    li r4,0
    b lab6
lab5:
    slw r3,r3,r0
    subfic r9,r0,32
    srw r9,r4,r9
    or r3,r3,r9
    slw r4,r4,r0
lab6:
    li r10,-1
    addic r7,r7,0
lab7:
    adde r4,r4,r4
    adde r3,r3,r3
    adde r8,r8,r8
    adde r7,r7,r7
    subfc r0,r6,r8
    subfe. r9,r5,r7
    blt lab8
    mr r8,r0
    mr r7,r9
    addic r0,r10,1
lab8:
    bdnz lab7
    adde r4,r4,r4
    adde r3,r3,r3
    blr
lab9:
    li r4,0
    li r3,0
    blr
}

/* Divides the signed 64-bit r3:r4 by r5:r6 and returns the quotient in r3:r4. */
asm void __div2i(void)
{
    nofralloc
    stwu r1,-16(r1)
    clrrwi. r9,r3,31
    beq positive1
    subfic r4,r4,0
    subfze r3,r3
positive1:
    stw r9,8(r1)
    clrrwi. r10,r5,31
    beq positive2
    subfic r6,r6,0
    subfze r5,r5
positive2:
    stw r10,12(r1)
    cmpwi r3,0
    cntlzw r0,r3
    cntlzw r9,r4
    bne lab1
    addi r0,r9,32
lab1:
    cmpwi r5,0
    cntlzw r9,r5
    cntlzw r10,r6
    bne lab2
    addi r9,r10,32
lab2:
    cmpw r0,r9
    subfic r10,r0,64
    bgt lab9
    addi r9,r9,1
    subfic r9,r9,64
    add r0,r0,r9
    subf r9,r9,r10
    mtctr r9
    cmpwi r9,32
    addi r7,r9,-32
    blt lab3
    srw r8,r3,r7
    li r7,0
    b lab4
lab3:
    srw r8,r4,r9
    subfic r7,r9,32
    slw r7,r3,r7
    or r8,r8,r7
    srw r7,r3,r9
lab4:
    cmpwi r0,32
    addic r9,r0,-32
    blt lab5
    slw r3,r4,r9
    li r4,0
    b lab6
lab5:
    slw r3,r3,r0
    subfic r9,r0,32
    srw r9,r4,r9
    or r3,r3,r9
    slw r4,r4,r0
lab6:
    li r10,-1
    addic r7,r7,0
lab7:
    adde r4,r4,r4
    adde r3,r3,r3
    adde r8,r8,r8
    adde r7,r7,r7
    subfc r0,r6,r8
    subfe. r9,r5,r7
    blt lab8
    mr r8,r0
    mr r7,r9
    addic r0,r10,1
lab8:
    bdnz lab7
    adde r4,r4,r4
    adde r3,r3,r3
    lwz r9,8(r1)
    lwz r10,12(r1)
    xor. r7,r9,r10
    beq no_adjust
    cmpwi r9,0
    subfic r4,r4,0
    subfze r3,r3
no_adjust:
    b func_end
lab9:
    li r4,0
    li r3,0
func_end:
    addi r1,r1,16
    blr
}

/* Returns the unsigned 64-bit remainder of r3:r4 by r5:r6 in r3:r4. */
asm void __mod2u(void)
{
    nofralloc
    cmpwi r3,0
    cntlzw r0,r3
    cntlzw r9,r4
    bne lab1
    addi r0,r9,32
lab1:
    cmpwi r5,0
    cntlzw r9,r5
    cntlzw r10,r6
    bne lab2
    addi r9,r10,32
lab2:
    cmpw r0,r9
    subfic r10,r0,64
    bgt lab9
    addi r9,r9,1
    subfic r9,r9,64
    add r0,r0,r9
    subf r9,r9,r10
    mtctr r9
    cmpwi r9,32
    addi r7,r9,-32
    blt lab3
    srw r8,r3,r7
    li r7,0
    b lab4
lab3:
    srw r8,r4,r9
    subfic r7,r9,32
    slw r7,r3,r7
    or r8,r8,r7
    srw r7,r3,r9
lab4:
    cmpwi r0,32
    addic r9,r0,-32
    blt lab5
    slw r3,r4,r9
    li r4,0
    b lab6
lab5:
    slw r3,r3,r0
    subfic r9,r0,32
    srw r9,r4,r9
    or r3,r3,r9
    slw r4,r4,r0
lab6:
    li r10,-1
    addic r7,r7,0
lab7:
    adde r4,r4,r4
    adde r3,r3,r3
    adde r8,r8,r8
    adde r7,r7,r7
    subfc r0,r6,r8
    subfe. r9,r5,r7
    blt lab8
    mr r8,r0
    mr r7,r9
    addic r0,r10,1
lab8:
    bdnz lab7
    mr r4,r8
    mr r3,r7
    blr
lab9:
    blr
}

/* Returns the signed 64-bit remainder of r3:r4 by r5:r6 in r3:r4, with the dividend's sign. */
asm void __mod2i(void)
{
    nofralloc
    cmpwi cr7,r3,0
    bge cr7,positive1
    subfic r4,r4,0
    subfze r3,r3
positive1:
    cmpwi r5,0
    bge positive2
    subfic r6,r6,0
    subfze r5,r5
positive2:
    cmpwi r3,0
    cntlzw r0,r3
    cntlzw r9,r4
    bne lab1
    addi r0,r9,32
lab1:
    cmpwi r5,0
    cntlzw r9,r5
    cntlzw r10,r6
    bne lab2
    addi r9,r10,32
lab2:
    cmpw r0,r9
    subfic r10,r0,64
    bgt lab9
    addi r9,r9,1
    subfic r9,r9,64
    add r0,r0,r9
    subf r9,r9,r10
    mtctr r9
    cmpwi r9,32
    addi r7,r9,-32
    blt lab3
    srw r8,r3,r7
    li r7,0
    b lab4
lab3:
    srw r8,r4,r9
    subfic r7,r9,32
    slw r7,r3,r7
    or r8,r8,r7
    srw r7,r3,r9
lab4:
    cmpwi r0,32
    addic r9,r0,-32
    blt lab5
    slw r3,r4,r9
    li r4,0
    b lab6
lab5:
    slw r3,r3,r0
    subfic r9,r0,32
    srw r9,r4,r9
    or r3,r3,r9
    slw r4,r4,r0
lab6:
    li r10,-1
    addic r7,r7,0
lab7:
    adde r4,r4,r4
    adde r3,r3,r3
    adde r8,r8,r8
    adde r7,r7,r7
    subfc r0,r6,r8
    subfe. r9,r5,r7
    blt lab8
    mr r8,r0
    mr r7,r9
    addic r0,r10,1
lab8:
    bdnz lab7
    mr r4,r8
    mr r3,r7
lab9:
    bge cr7,no_adjust
    subfic r4,r4,0
    subfze r3,r3
no_adjust:
    blr
}

/* Shifts the 64-bit r3:r4 left by r5. */
asm void __shl2i(void)
{
    nofralloc
    subfic r8,r5,32
    addic r9,r5,-32
    slw r3,r3,r5
    srw r10,r4,r8
    or r3,r3,r10
    slw r10,r4,r9
    or r3,r3,r10
    slw r4,r4,r5
    blr
}

/* Shifts the unsigned 64-bit r3:r4 right by r5. */
asm void __shr2u(void)
{
    nofralloc
    subfic r8,r5,32
    addic r9,r5,-32
    srw r4,r4,r5
    slw r10,r3,r8
    or r4,r4,r10
    srw r10,r3,r9
    or r4,r4,r10
    srw r3,r3,r5
    blr
}

/* Converts the signed 64-bit r3:r4 to a double in f1, rounding to nearest even. */
asm void __cvt_sll_dbl(void)
{
    nofralloc
    stwu r1,-16(r1)
    clrrwi. r5,r3,31
    beq positive
    subfic r4,r4,0
    subfze r3,r3
positive:
    or. r7,r3,r4
    li r6,0
    beq zero
    cntlzw r7,r3
    cntlzw r8,r4
    rlwinm r9,r7,26,0,4
    srawi r9,r9,31
    and r9,r9,r8
    add r7,r7,r9
    subfic r8,r7,32
    addic r9,r7,-32
    slw r3,r3,r7
    srw r10,r4,r8
    or r3,r3,r10
    slw r10,r4,r9
    or r3,r3,r10
    slw r4,r4,r7
    subf r6,r7,r6
    clrlwi r7,r4,21
    cmpwi r7,1024
    addi r6,r6,1086
    blt noround
    bgt round
    rlwinm. r7,r4,0,20,20
    beq noround
round:
    addic r4,r4,2048
    addze r3,r3
    addze r6,r6
noround:
    rotlwi r4,r4,21
    rlwimi r4,r3,21,0,10
    rlwinm r3,r3,21,12,31
    slwi r6,r6,20
    or r3,r6,r3
    or r3,r5,r3
zero:
    stw r3,8(r1)
    stw r4,12(r1)
    lfd f1,8(r1)
    addi r1,r1,16
    blr
}

/* Converts the double in f1 to an unsigned 64-bit r3:r4, saturating to 0 and all ones. */
asm void __cvt_dbl_ull(void)
{
    nofralloc
    stwu r1,-16(r1)
    stfd f1,8(r1)
    lwz r3,8(r1)
    lwz r4,12(r1)
    rlwinm r5,r3,12,21,31
    cmplwi r5,1023
    bge not_fraction
return_zero:
    li r3,0
    li r4,0
    b func_end
not_fraction:
    clrrwi. r6,r3,31
    bne return_zero
    clrlwi r3,r3,12
    oris r3,r3,16
    addi r5,r5,-1075
    cmpwi r5,0
    bge left
    neg r5,r5
    subfic r8,r5,32
    addic r9,r5,-32
    srw r4,r4,r5
    slw r10,r3,r8
    or r4,r4,r10
    srw r10,r3,r9
    or r4,r4,r10
    srw r3,r3,r5
    b func_end
left:
    cmpwi r5,11
    ble+ no_overflow
    li r3,-1
    li r4,-1
    b func_end
no_overflow:
    subfic r8,r5,32
    addic r9,r5,-32
    slw r3,r3,r5
    srw r10,r4,r8
    or r3,r3,r10
    slw r10,r4,r9
    or r3,r3,r10
    slw r4,r4,r5
func_end:
    addi r1,r1,16
    blr
}
} /* extern "C" */
