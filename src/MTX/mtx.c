/*
 * MTX/mtx.c - the MTX library 3x4 matrix routines: identity, copy, concat, inverse, rotation, translation, scale,
 *    quaternion conversion, look-at and the texture-projection ("light") matrices.
 * RANGE. .text 0x804C5C10-0x804C68A0 (19 functions); .sdata 0x80793F00-0x80793F08; .sdata2 0x8079D278-0x8079D298.
 *    Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the dump names `PSMTXIdentity`..`PSMTXQuat`; the
 *    range owns .sdata 0x80793F00 (read by `PSMTXConcat`/`PSMTXConcatArray`) and .sdata2 0x8079D278..0x8079D298
 *    (read by the rotation/translate/scale/quaternion bodies) and its vector helpers call into `PSVEC*` at
 *    0x804C6B60; `PSMTXMultVec` (0x804C68A0) is the registered `MTX/mtxvec.c`.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. the `PSMTX*`/`PSVEC*` rows are the map's names; GUESS: `PSMTXInvXpose`, `__PSMTXRotAxisRadInternal`,
 *    `C_MTXLookAt`, `C_MTXLightFrustum`, `C_MTXLightPerspective`, `C_MTXLightOrtho` (the SDK entry each body implements),
 *    `sMtxZeroOnePair`, `sMtxOne`, `sMtxZero`, `sMtxHalf`, `sMtxThree`, `sMtxTwo`, `sMtxMinusOne`, `sMtxDegToRad` (the constants
 *    by value); GUESS: `PSMTXConcatArray`, `PSMTXRotAxisRad`, `PSMTXQuat` (the map's rows: the dump has a placeholder at
 *    those addresses; the bodies are the SDK matrix-concat, axis-angle and quaternion-to-matrix routines).
 * RESIDUALS. `C_MTXLightFrustum` and `C_MTXLightOrtho` (99.5 %, 99.4 %): the operands of the two commutative scale
 *    multiplies (`m[0][0]`, `m[1][1]`) come out in the opposite order whichever way the source writes them; the unit's
 *    .text ends 8 bytes before the claimed end (the link pads to the next unit's 16-byte start).
 * SHAPES. the paired-single bodies (`psq_l`/`ps_*`) are asm functions with `nofralloc` (playbook 104); the constants are
 *    globals the asm and the C bodies both load, so the pool is the file's own definitions.
 */


#include "types.h"

#include "MTX/mtx.h"
#include "MTX/vec.h"
#include "MSL/s_cos.h"
#include "MSL/s_sin.h"
#include "MSL/s_tan.h"


#pragma fp_contract off

extern f32 sMtxZeroOnePair[2];
extern const f32 sMtxOne;
extern const f32 sMtxZero;
extern const f32 sMtxHalf;
extern const f32 sMtxThree;
extern const f32 sMtxTwo;
extern const f32 sMtxMinusOne;
extern const f32 sMtxDegToRad;

/* Sets `m` to the identity. */
asm void PSMTXIdentity(register Mtx m)
{
    nofralloc
    lfs       f0,sMtxZero(r0)
    lfs       f1,sMtxOne(r0)
    psq_st    f0,8(r3),0,0
    ps_merge10 f2,f1,f0
    ps_merge01 f1,f0,f1
    psq_st    f0,24(r3),0,0
    psq_st    f0,32(r3),0,0
    psq_st    f1,16(r3),0,0
    psq_st    f2,0(r3),0,0
    psq_st    f2,40(r3),0,0
    blr       
}

/* Copies a 3x4 matrix. */
asm void PSMTXCopy(register const Mtx src, register Mtx dst)
{
    nofralloc
    psq_l     f0,0(r3),0,0
    psq_st    f0,0(r4),0,0
    psq_l     f1,8(r3),0,0
    psq_st    f1,8(r4),0,0
    psq_l     f2,16(r3),0,0
    psq_st    f2,16(r4),0,0
    psq_l     f3,24(r3),0,0
    psq_st    f3,24(r4),0,0
    psq_l     f4,32(r3),0,0
    psq_st    f4,32(r4),0,0
    psq_l     f5,40(r3),0,0
    psq_st    f5,40(r4),0,0
    blr       
}

/* Concatenates two matrices: ab = a * b. */
asm void PSMTXConcat(register const Mtx a, register const Mtx b, register Mtx ab)
{
    nofralloc
    stwu      r1,-64(r1)
    psq_l     f0,0(r3),0,0
    stfd      f14,8(r1)
    psq_l     f6,0(r4),0,0
    lis       r6, sMtxZeroOnePair@ha
    psq_l     f7,8(r4),0,0
    stfd      f15,16(r1)
    addi      r6,r6, sMtxZeroOnePair@l
    stfd      f31,40(r1)
    psq_l     f8,16(r4),0,0
    ps_muls0  f12,f6,f0
    psq_l     f2,16(r3),0,0
    ps_muls0  f13,f7,f0
    psq_l     f31,0(r6),0,0
    ps_muls0  f14,f6,f2
    psq_l     f9,24(r4),0,0
    ps_muls0  f15,f7,f2
    psq_l     f1,8(r3),0,0
    ps_madds1 f12,f8,f0,f12
    psq_l     f3,24(r3),0,0
    ps_madds1 f14,f8,f2,f14
    psq_l     f10,32(r4),0,0
    ps_madds1 f13,f9,f0,f13
    psq_l     f11,40(r4),0,0
    ps_madds1 f15,f9,f2,f15
    psq_l     f4,32(r3),0,0
    psq_l     f5,40(r3),0,0
    ps_madds0 f12,f10,f1,f12
    ps_madds0 f13,f11,f1,f13
    ps_madds0 f14,f10,f3,f14
    ps_madds0 f15,f11,f3,f15
    psq_st    f12,0(r5),0,0
    ps_muls0  f2,f6,f4
    ps_madds1 f13,f31,f1,f13
    ps_muls0  f0,f7,f4
    psq_st    f14,16(r5),0,0
    ps_madds1 f15,f31,f3,f15
    psq_st    f13,8(r5),0,0
    ps_madds1 f2,f8,f4,f2
    ps_madds1 f0,f9,f4,f0
    ps_madds0 f2,f10,f5,f2
    lfd       f14,8(r1)
    psq_st    f15,24(r5),0,0
    ps_madds0 f0,f11,f5,f0
    psq_st    f2,32(r5),0,0
    ps_madds1 f0,f31,f5,f0
    lfd       f15,16(r1)
    psq_st    f0,40(r5),0,0
    lfd       f31,40(r1)
    addi      r1,r1,64
    blr       
}

/* Concatenates `a` with each matrix of an array. */
asm void PSMTXConcatArray(register const Mtx a, register const Mtx* srcBase, register Mtx* dstBase, register u32 count)
{
    nofralloc
    stwu      r1,-80(r1)
    stfd      f31,64(r1)
    psq_st    f31,72(r1),0,0
    stfd      f30,48(r1)
    psq_st    f30,56(r1),0,0
    stfd      f29,32(r1)
    psq_st    f29,40(r1),0,0
    stfd      f28,16(r1)
    psq_st    f28,24(r1),0,0
    addi      r0,r6,-1
    psq_l     f0,0(r3),0,0
    psq_l     f1,8(r3),0,0
    la        r6, sMtxZeroOnePair(r0)
    psq_l     f2,16(r3),0,0
    psq_l     f3,24(r3),0,0
    psq_l     f4,32(r3),0,0
    psq_l     f5,40(r3),0,0
    mtctr     r0
    psq_l     f6,0(r4),0,0
    psq_l     f7,8(r4),0,0
    ps_muls0  f11,f6,f0
    psq_l     f8,16(r4),0,0
    ps_muls0  f13,f6,f2
    psq_l     f9,32(r4),0,0
    ps_muls0  f30,f6,f4
    psq_l     f6,24(r4),0,0
    ps_madds1 f11,f8,f0,f11
    psq_l     f28,0(r6),0,0
    ps_madds1 f13,f8,f2,f13
    psq_l     f10,40(r4),0,0
    ps_madds1 f30,f8,f4,f30
    ps_muls0  f12,f7,f0
    ps_muls0  f31,f7,f2
    ps_muls0  f29,f7,f4
    ps_madds0 f11,f9,f1,f11
    ps_madds0 f13,f9,f3,f13
    ps_madds0 f30,f9,f5,f30
    psq_st    f11,0(r5),0,0
    ps_madds1 f12,f6,f0,f12
    ps_madds1 f31,f6,f2,f31
    psq_st    f13,16(r5),0,0
    ps_madds1 f29,f6,f4,f29
L_1e8:
    ps_madds0 f12,f10,f1,f12
    psq_l     f6,48(r4),0,0
    ps_madds0 f31,f10,f3,f31
    psq_st    f30,32(r5),0,0
    ps_madds0 f29,f10,f5,f29
    psq_l     f8,64(r4),0,0
    ps_madd   f12,f28,f1,f12
    psq_l     f9,80(r4),0,0
    ps_muls0  f11,f6,f0
    psq_l     f7,56(r4),0,0
    psq_st    f12,8(r5),0,0
    ps_madd   f31,f28,f3,f31
    ps_muls0  f13,f6,f2
    psq_st    f31,24(r5),0,0
    ps_muls0  f30,f6,f4
    psq_l     f6,72(r4),0,0
    ps_madds1 f11,f8,f0,f11
    psq_l     f10,88(r4),0,0
    ps_madd   f29,f28,f5,f29
    addi      r4,r4,48
    ps_madds1 f13,f8,f2,f13
    psq_st    f29,40(r5),0,0
    ps_madds1 f30,f8,f4,f30
    ps_madds0 f11,f9,f1,f11
    ps_muls0  f12,f7,f0
    ps_muls0  f31,f7,f2
    psq_st    f11,48(r5),0,0
    ps_muls0  f29,f7,f4
    ps_madds0 f13,f9,f3,f13
    ps_madds0 f30,f9,f5,f30
    psq_st    f13,64(r5),0,0
    ps_madds1 f12,f6,f0,f12
    ps_madds1 f31,f6,f2,f31
    addi      r5,r5,48
    ps_madds1 f29,f6,f4,f29
    bdnz      L_1e8
    ps_madds0 f12,f10,f1,f12
    psq_st    f30,32(r5),0,0
    ps_madds0 f31,f10,f3,f31
    ps_madds0 f29,f10,f5,f29
    ps_madd   f12,f28,f1,f12
    ps_madd   f31,f28,f3,f31
    psq_st    f12,8(r5),0,0
    ps_madd   f29,f28,f5,f29
    psq_st    f31,24(r5),0,0
    psq_st    f29,40(r5),0,0
    psq_l     f31,72(r1),0,0
    lfd       f31,64(r1)
    psq_l     f30,56(r1),0,0
    lfd       f30,48(r1)
    psq_l     f29,40(r1),0,0
    lfd       f29,32(r1)
    psq_l     f28,24(r1),0,0
    lfd       f28,16(r1)
    addi      r1,r1,80
    blr       
}

/* Inverts an affine matrix; 0 when it is singular. */
asm u32 PSMTXInverse(register const Mtx src, register Mtx inv)
{
    nofralloc
    psq_l     f0,0(r3),1,0
    psq_l     f1,4(r3),0,0
    psq_l     f2,16(r3),1,0
    ps_merge10 f6,f1,f0
    psq_l     f3,20(r3),0,0
    psq_l     f4,32(r3),1,0
    ps_merge10 f7,f3,f2
    psq_l     f5,36(r3),0,0
    ps_mul    f11,f3,f6
    ps_merge10 f8,f5,f4
    ps_mul    f13,f5,f7
    ps_msub   f11,f1,f7,f11
    ps_mul    f12,f1,f8
    ps_msub   f13,f3,f8,f13
    ps_mul    f10,f3,f4
    ps_msub   f12,f5,f6,f12
    ps_mul    f7,f0,f13
    ps_mul    f9,f0,f5
    ps_mul    f8,f1,f2
    ps_madd   f7,f2,f12,f7
    ps_sub    f6,f6,f6
    ps_msub   f10,f2,f5,f10
    ps_madd   f7,f4,f11,f7
    ps_msub   f9,f1,f4,f9
    ps_msub   f8,f0,f3,f8
    ps_cmpo0  cr0,f7,f6
    bne       L_344
    li        r3,0
    blr       
L_344:
    fres      f0,f7
    ps_add    f6,f0,f0
    ps_mul    f5,f7,f0
    ps_nmsub  f0,f0,f5,f6
    lfs       f1,12(r3)
    ps_muls0  f13,f13,f0
    lfs       f2,28(r3)
    ps_muls0  f12,f12,f0
    lfs       f3,44(r3)
    ps_muls0  f11,f11,f0
    ps_merge00 f5,f13,f12
    ps_merge11 f4,f13,f12
    ps_mul    f6,f13,f1
    psq_st    f5,0(r4),0,0
    psq_st    f4,16(r4),0,0
    ps_muls0  f10,f10,f0
    ps_muls0  f9,f9,f0
    ps_madd   f6,f12,f2,f6
    psq_st    f10,32(r4),1,0
    ps_muls0  f8,f8,f0
    ps_nmadd  f6,f11,f3,f6
    psq_st    f9,36(r4),1,0
    ps_mul    f7,f10,f1
    ps_merge00 f5,f11,f6
    psq_st    f8,40(r4),1,0
    ps_madd   f7,f9,f2,f7
    ps_merge11 f4,f11,f6
    psq_st    f5,8(r4),0,0
    ps_nmadd  f7,f8,f3,f7
    psq_st    f4,24(r4),0,0
    psq_st    f7,44(r4),1,0
    li        r3,1
    blr       
}

/* Computes the inverse transpose of the upper 3x3; 0 when it is singular. */
asm u32 PSMTXInvXpose(register const Mtx src, register Mtx xPose)
{
    nofralloc
    psq_l     f0,0(r3),1,0
    psq_l     f1,4(r3),0,0
    psq_l     f2,16(r3),1,0
    ps_merge10 f6,f1,f0
    psq_l     f3,20(r3),0,0
    psq_l     f4,32(r3),1,0
    ps_merge10 f7,f3,f2
    psq_l     f5,36(r3),0,0
    ps_mul    f11,f3,f6
    ps_merge10 f8,f5,f4
    ps_mul    f13,f5,f7
    ps_msub   f11,f1,f7,f11
    ps_mul    f12,f1,f8
    ps_msub   f13,f3,f8,f13
    ps_mul    f10,f3,f4
    ps_msub   f12,f5,f6,f12
    ps_mul    f7,f0,f13
    ps_mul    f9,f0,f5
    ps_mul    f8,f1,f2
    ps_madd   f7,f2,f12,f7
    ps_sub    f6,f6,f6
    ps_msub   f10,f2,f5,f10
    ps_madd   f7,f4,f11,f7
    ps_msub   f9,f1,f4,f9
    ps_msub   f8,f0,f3,f8
    ps_cmpo0  cr0,f7,f6
    bne       L_444
    li        r3,0
    blr       
L_444:
    fres      f0,f7
    psq_st    f6,12(r4),1,0
    ps_add    f4,f0,f0
    ps_mul    f5,f7,f0
    psq_st    f6,28(r4),1,0
    ps_nmsub  f0,f0,f5,f4
    psq_st    f6,44(r4),1,0
    ps_muls0  f13,f13,f0
    ps_muls0  f12,f12,f0
    psq_st    f13,0(r4),0,0
    ps_muls0  f11,f11,f0
    psq_st    f12,16(r4),0,0
    ps_muls0  f10,f10,f0
    psq_st    f11,32(r4),0,0
    ps_muls0  f9,f9,f0
    psq_st    f10,8(r4),1,0
    ps_muls0  f8,f8,f0
    psq_st    f9,24(r4),1,0
    psq_st    f8,40(r4),1,0
    li        r3,1
    blr       
}

/* Builds the rotation about `axis` ('x', 'y' or 'z') for an angle in radians. */
void PSMTXRotRad(Mtx m, char axis, f32 rad)
{
    f32 sinA = (f32)sin(rad);
    f32 cosA = (f32)cos(rad);

    PSMTXRotTrig(m, axis, sinA, cosA);
}

/* Builds the rotation about an axis from its sine and cosine. */
asm void PSMTXRotTrig(register Mtx m, register char axis, register f32 sinA, register f32 cosA)
{
    nofralloc
    frsp      f5,f1
    ori       r0,r4,32
    frsp      f4,f2
    cmplwi    r0,120
    lfs       f0,sMtxZero(r0)
    ps_neg    f2,f5
    lfs       f1,sMtxOne(r0)
    beq       L_554
    cmplwi    r0,121
    beq       L_57c
    cmplwi    r0,122
    beq       L_5a8
    blr       
L_554:
    ps_merge00 f3,f5,f4
    psq_st    f1,0(r3),1,0
    ps_merge00 f1,f4,f2
    psq_st    f0,4(r3),0,0
    psq_st    f0,12(r3),0,0
    psq_st    f0,28(r3),0,0
    psq_st    f0,44(r3),1,0
    psq_st    f3,36(r3),0,0
    psq_st    f1,20(r3),0,0
    blr       
L_57c:
    ps_merge00 f3,f4,f0
    psq_st    f0,24(r3),0,0
    ps_merge00 f1,f0,f1
    ps_merge00 f2,f2,f0
    psq_st    f3,0(r3),0,0
    ps_merge00 f0,f5,f0
    psq_st    f3,40(r3),0,0
    psq_st    f1,16(r3),0,0
    psq_st    f0,8(r3),0,0
    psq_st    f2,32(r3),0,0
    blr       
L_5a8:
    ps_merge00 f3,f5,f4
    psq_st    f0,8(r3),0,0
    ps_merge00 f2,f4,f2
    ps_merge00 f1,f1,f0
    psq_st    f0,24(r3),0,0
    psq_st    f0,32(r3),0,0
    psq_st    f3,16(r3),0,0
    psq_st    f2,0(r3),0,0
    psq_st    f1,40(r3),0,0
    blr       
}

/* Builds the rotation about an arbitrary axis from its sine and cosine. */
asm void __PSMTXRotAxisRadInternal(register Mtx m, register const Vec* axis, register f32 sinA, register f32 cosA)
{
    nofralloc
    psq_l     f3,0(r4),0,0
    frsp      f11,f2
    lfs       f10,sMtxHalf(r0)
    frsp      f12,f1
    ps_mul    f4,f3,f3
    lfs       f2,8(r4)
    fadds     f8,f10,f10
    lfs       f9,sMtxThree(r0)
    fsubs     f1,f10,f10
    ps_madd   f5,f2,f2,f4
    fsubs     f0,f8,f11
    ps_merge00 f11,f11,f11
    ps_sum0   f6,f5,f2,f4
    frsqrte   f7,f6
    fmuls     f4,f7,f7
    fmuls     f5,f7,f10
    fnmsubs   f4,f4,f6,f9
    fmuls     f7,f4,f5
    ps_muls0  f3,f3,f7
    ps_muls0  f2,f2,f7
    ps_muls0  f6,f3,f0
    ps_muls0  f7,f2,f0
    ps_muls0  f10,f3,f12
    ps_muls1  f5,f6,f3
    ps_muls0  f4,f6,f3
    ps_muls0  f6,f6,f2
    fnmsubs   f0,f2,f12,f5
    ps_neg    f3,f10
    fmadds    f8,f2,f12,f5
    ps_sum0   f4,f4,f0,f11
    ps_sum0   f0,f3,f1,f6
    ps_muls0  f7,f7,f2
    psq_st    f4,0(r3),0,0
    ps_sum0   f9,f6,f1,f10
    ps_sum0   f3,f6,f6,f3
    psq_st    f0,24(r3),0,0
    ps_sum1   f5,f11,f8,f5
    ps_sum0   f7,f7,f1,f11
    psq_st    f9,8(r3),0,0
    ps_sum1   f6,f10,f3,f6
    psq_st    f5,16(r3),0,0
    psq_st    f6,32(r3),0,0
    psq_st    f7,40(r3),0,0
    blr       
}

/* Builds the rotation about an arbitrary axis for an angle in radians. */
void PSMTXRotAxisRad(Mtx m, const Vec* axis, f32 rad)
{
    f32 sinA = (f32)sin(rad);
    f32 cosA = (f32)cos(rad);

    __PSMTXRotAxisRadInternal(m, axis, sinA, cosA);
}

/* Builds a translation matrix. */
asm void PSMTXTrans(register Mtx m, register f32 xT, register f32 yT, register f32 zT)
{
    nofralloc
    lfs       f0,sMtxZero(r0)
    lfs       f4,sMtxOne(r0)
    stfs      f1,12(r3)
    stfs      f2,28(r3)
    psq_st    f0,4(r3),0,0
    psq_st    f0,32(r3),0,0
    stfs      f0,16(r3)
    stfs      f4,20(r3)
    stfs      f0,24(r3)
    stfs      f4,40(r3)
    stfs      f3,44(r3)
    stfs      f4,0(r3)
    blr       
}

/* Applies a translation to a matrix. */
asm void PSMTXTransApply(register const Mtx src, register Mtx dst, register f32 xT, register f32 yT, register f32 zT)
{
    nofralloc
    psq_l     f4,0(r3),0,0
    frsp      f1,f1
    psq_l     f5,8(r3),0,0
    frsp      f2,f2
    psq_l     f7,24(r3),0,0
    frsp      f3,f3
    psq_l     f8,40(r3),0,0
    psq_st    f4,0(r4),0,0
    ps_sum1   f5,f1,f5,f5
    psq_l     f6,16(r3),0,0
    psq_st    f5,8(r4),0,0
    ps_sum1   f7,f2,f7,f7
    psq_l     f9,32(r3),0,0
    psq_st    f6,16(r4),0,0
    ps_sum1   f8,f3,f8,f8
    psq_st    f7,24(r4),0,0
    psq_st    f9,32(r4),0,0
    psq_st    f8,40(r4),0,0
    blr       
}

/* Builds a scale matrix. */
asm void PSMTXScale(register Mtx m, register f32 xS, register f32 yS, register f32 zS)
{
    nofralloc
    lfs       f0,sMtxZero(r0)
    stfs      f1,0(r3)
    psq_st    f0,4(r3),0,0
    psq_st    f0,12(r3),0,0
    stfs      f2,20(r3)
    psq_st    f0,24(r3),0,0
    psq_st    f0,32(r3),0,0
    stfs      f3,40(r3)
    stfs      f0,44(r3)
    blr       
}

/* Applies a scale to a matrix. */
asm void PSMTXScaleApply(register const Mtx src, register Mtx dst, register f32 xS, register f32 yS, register f32 zS)
{
    nofralloc
    frsp      f1,f1
    psq_l     f4,0(r3),0,0
    frsp      f2,f2
    psq_l     f5,8(r3),0,0
    frsp      f3,f3
    ps_muls0  f4,f4,f1
    psq_l     f6,16(r3),0,0
    ps_muls0  f5,f5,f1
    psq_l     f7,24(r3),0,0
    ps_muls0  f6,f6,f2
    psq_l     f8,32(r3),0,0
    psq_st    f4,0(r4),0,0
    ps_muls0  f7,f7,f2
    psq_l     f2,40(r3),0,0
    psq_st    f5,8(r4),0,0
    ps_muls0  f8,f8,f3
    psq_st    f6,16(r4),0,0
    ps_muls0  f2,f2,f3
    psq_st    f7,24(r4),0,0
    psq_st    f8,32(r4),0,0
    psq_st    f2,40(r4),0,0
    blr       
}

/* Builds the rotation matrix of a quaternion. */
asm void PSMTXQuat(register Mtx m, register const Quaternion* q)
{
    nofralloc
    psq_l     f4,0(r4),0,0
    psq_l     f5,8(r4),0,0
    ps_mul    f6,f4,f4
    lfs       f1,sMtxOne(r0)
    ps_merge10 f9,f4,f4
    fsubs     f0,f1,f1
    ps_madd   f8,f5,f5,f6
    ps_muls1  f10,f5,f5
    psq_st    f0,12(r3),1,0
    fadds     f2,f1,f1
    ps_sum0   f3,f8,f8,f8
    psq_st    f0,44(r3),1,0
    ps_mul    f7,f5,f5
    ps_madd   f12,f4,f9,f10
    fres      f13,f3
    ps_nmsub  f3,f3,f13,f2
    ps_muls1  f11,f9,f5
    ps_msub   f10,f4,f9,f10
    ps_mul    f3,f13,f3
    ps_madds0 f9,f4,f5,f11
    ps_sum1   f8,f7,f8,f6
    fmuls     f3,f3,f2
    ps_nmsub  f11,f11,f2,f9
    ps_sum0   f6,f6,f6,f6
    ps_mul    f9,f9,f3
    ps_mul    f11,f11,f3
    ps_nmsub  f8,f8,f3,f1
    psq_st    f9,8(r3),1,0
    ps_mul    f12,f12,f3
    ps_mul    f10,f10,f3
    ps_merge10 f7,f11,f0
    ps_merge00 f5,f12,f8
    ps_merge10 f4,f8,f10
    psq_st    f7,24(r3),0,0
    ps_merge01 f13,f11,f9
    ps_nmsub  f6,f6,f3,f1
    psq_st    f5,16(r3),0,0
    psq_st    f6,40(r3),1,0
    psq_st    f4,0(r3),0,0
    psq_st    f13,32(r3),0,0
    blr       
}

/* Builds the view matrix looking from `camPos` toward `target` with `camUp` as the up hint. */
void C_MTXLookAt(Mtx m, const Vec* camPos, const Vec* camUp, const Vec* target)
{
    Vec vLook;
    Vec vRight;
    Vec vUp;

    vLook.x = camPos->x - target->x;
    vLook.y = camPos->y - target->y;
    vLook.z = camPos->z - target->z;
    PSVECNormalize(&vLook, &vLook);
    PSVECCrossProduct(camUp, &vLook, &vRight);
    PSVECNormalize(&vRight, &vRight);
    PSVECCrossProduct(&vLook, &vRight, &vUp);

    m[0][0] = vRight.x;
    m[0][1] = vRight.y;
    m[0][2] = vRight.z;
    m[0][3] = -(camPos->x * vRight.x + camPos->y * vRight.y + camPos->z * vRight.z);
    m[1][0] = vUp.x;
    m[1][1] = vUp.y;
    m[1][2] = vUp.z;
    m[1][3] = -(camPos->x * vUp.x + camPos->y * vUp.y + camPos->z * vUp.z);
    m[2][0] = vLook.x;
    m[2][1] = vLook.y;
    m[2][2] = vLook.z;
    m[2][3] = -(camPos->x * vLook.x + camPos->y * vLook.y + camPos->z * vLook.z);
}

/* Builds the texture-projection matrix of a frustum. */
void C_MTXLightFrustum(Mtx m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 scaleS, f32 scaleT, f32 transS, f32 transT)
{
    f32 tmp;

    tmp = sMtxOne / (r - l);
    m[0][0] = (tmp * (sMtxTwo * n)) * scaleS;
    m[0][1] = sMtxZero;
    m[0][2] = (((r + l) * tmp) * scaleS) - transS;
    m[0][3] = sMtxZero;
    tmp = sMtxOne / (t - b);
    m[1][0] = sMtxZero;
    m[1][1] = (tmp * (sMtxTwo * n)) * scaleT;
    m[1][2] = (((t + b) * tmp) * scaleT) - transT;
    m[1][3] = sMtxZero;
    m[2][0] = sMtxZero;
    m[2][1] = sMtxZero;
    m[2][2] = sMtxMinusOne;
    m[2][3] = sMtxZero;
}

/* Builds the texture-projection matrix of a perspective view (field of view in degrees). */
void C_MTXLightPerspective(Mtx m, f32 fovY, f32 aspect, f32 scaleS, f32 scaleT, f32 transS, f32 transT)
{
    f32 angle;
    f32 cot;

    angle = sMtxHalf * fovY;
    angle = sMtxDegToRad * angle;
    cot = sMtxOne / (f32)tan(angle);
    m[0][0] = (cot / aspect) * scaleS;
    m[0][1] = sMtxZero;
    m[0][2] = -transS;
    m[0][3] = sMtxZero;
    m[1][0] = sMtxZero;
    m[1][1] = cot * scaleT;
    m[1][2] = -transT;
    m[1][3] = sMtxZero;
    m[2][0] = sMtxZero;
    m[2][1] = sMtxZero;
    m[2][2] = sMtxMinusOne;
    m[2][3] = sMtxZero;
}

/* Builds the texture-projection matrix of an orthographic view. */
void C_MTXLightOrtho(Mtx m, f32 t, f32 b, f32 l, f32 r, f32 scaleS, f32 scaleT, f32 transS, f32 transT)
{
    f32 tmp;

    tmp = sMtxOne / (r - l);
    m[0][0] = scaleS * (sMtxTwo * tmp);
    m[0][1] = sMtxZero;
    m[0][2] = sMtxZero;
    m[0][3] = ((-(r + l) * tmp) * scaleS) + transS;
    tmp = sMtxOne / (t - b);
    m[1][0] = sMtxZero;
    m[1][1] = scaleT * (sMtxTwo * tmp);
    m[1][2] = sMtxZero;
    m[1][3] = ((-(t + b) * tmp) * scaleT) + transT;
    m[2][0] = sMtxZero;
    m[2][1] = sMtxZero;
    m[2][2] = sMtxZero;
    m[2][3] = sMtxOne;
}


f32 sMtxZeroOnePair[2] = { 0.0f, 1.0f }; /* .sdata 0x80793F00 */

const f32 sMtxOne = 1.0f;            /* .sdata2 0x8079D278 */
const f32 sMtxZero = 0.0f;           /* .sdata2 0x8079D27C */
const f32 sMtxHalf = 0.5f;           /* .sdata2 0x8079D280 */
const f32 sMtxThree = 3.0f;          /* .sdata2 0x8079D284 */
const f32 sMtxTwo = 2.0f;            /* .sdata2 0x8079D288 */
const f32 sMtxMinusOne = -1.0f;      /* .sdata2 0x8079D28C */
const f32 sMtxDegToRad = 0.017453292f; /* .sdata2 0x8079D290 */
