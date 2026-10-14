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
 *    `sMtxZeroOnePair` (the .sdata pair by value); GUESS: `PSMTXConcatArray`, `PSMTXRotAxisRad`, `PSMTXQuat` (the map's
 *    rows: the dump has a placeholder at those addresses; the bodies are the SDK matrix-concat, axis-angle and
 *    quaternion-to-matrix routines).
 * RESIDUALS. none in .text; the unit's .text ends 8 bytes and its .sdata2 4 bytes before the claimed ends (link alignment fill).
 * SHAPES. the paired-single bodies that need no constant are asm functions with `nofralloc` (playbook 104); the ones
 *    that load one are C functions whose `register` locals are set from literals and feed an `asm` block, so the
 *    compiler schedules the pool loads into the block and pools the literals in first-use order (1, 0, 0.5, 3, 2, -1,
 *    deg-to-rad), the target's .sdata2 order; the variable declaration order picks the registers.
 */


#include "types.h"

#include "MTX/mtx.h"
#include "MTX/vec.h"
#include "MSL/s_cos.h"
#include "MSL/s_sin.h"
#include "MSL/s_tan.h"


#pragma fp_contract off

extern f32 sMtxZeroOnePair[2];

/* Sets `m` to the identity. */
void PSMTXIdentity(register Mtx m)
{
    register f32 zero;
    register f32 one;
    register f32 c10;

    one = 1.0f;
    zero = 0.0f;

    asm {
        psq_st    zero,8(m),0,0
        ps_merge10 c10,one,zero
        ps_merge01 one,zero,one
        psq_st    zero,24(m),0,0
        psq_st    zero,32(m),0,0
        psq_st    one,16(m),0,0
        psq_st    c10,0(m),0,0
        psq_st    c10,40(m),0,0
    }
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
void PSMTXRotTrig(register Mtx m, register char axis, register f32 sinA, register f32 cosA)
{
    register f32 fc0;
    register f32 fc1;
    register f32 nsinA;
    register f32 fw0;

    fc0 = 0.0f;
    fc1 = 1.0f;
    asm {
        frsp      sinA,sinA
        ori       axis,axis,32
        frsp      cosA,cosA
        cmplwi    axis,120
        ps_neg    nsinA,sinA
        beq       L_x
        cmplwi    axis,121
        beq       L_y
        cmplwi    axis,122
        beq       L_z
        b         L_end
    L_x:
        ps_merge00 fw0,sinA,cosA
        psq_st    fc1,0(m),1,0
        ps_merge00 fc1,cosA,nsinA
        psq_st    fc0,4(m),0,0
        psq_st    fc0,12(m),0,0
        psq_st    fc0,28(m),0,0
        psq_st    fc0,44(m),1,0
        psq_st    fw0,36(m),0,0
        psq_st    fc1,20(m),0,0
        b         L_end
    L_y:
        ps_merge00 fw0,cosA,fc0
        psq_st    fc0,24(m),0,0
        ps_merge00 fc1,fc0,fc1
        ps_merge00 nsinA,nsinA,fc0
        psq_st    fw0,0(m),0,0
        ps_merge00 fc0,sinA,fc0
        psq_st    fw0,40(m),0,0
        psq_st    fc1,16(m),0,0
        psq_st    fc0,8(m),0,0
        psq_st    nsinA,32(m),0,0
        b         L_end
    L_z:
        ps_merge00 fw0,sinA,cosA
        psq_st    fc0,8(m),0,0
        ps_merge00 nsinA,cosA,nsinA
        ps_merge00 fc1,fc1,fc0
        psq_st    fc0,24(m),0,0
        psq_st    fc0,32(m),0,0
        psq_st    fw0,16(m),0,0
        psq_st    nsinA,0(m),0,0
        psq_st    fc1,40(m),0,0
    L_end:
    }
}

/* Builds the rotation about an arbitrary axis from its sine and cosine. */
void __PSMTXRotAxisRadInternal(register Mtx m, register const Vec* axis, register f32 sT, register f32 cT)
{
    register f32 tT;
    register f32 fc0;
    register f32 tmp0;
    register f32 tmp1;
    register f32 tmp2;
    register f32 tmp3;
    register f32 tmp4;
    register f32 tmp5;
    register f32 tmp6;
    register f32 tmp7;
    register f32 tmp8;
    register f32 tmp9;

    tmp9 = 0.5f;
    tmp8 = 3.0f;
    asm {
        psq_l     tmp0,0(axis),0,0
        frsp      cT,cT
        frsp      sT,sT
        ps_mul    tmp2,tmp0,tmp0
        lfs       tmp1,8(axis)
        fadds     tmp7,tmp9,tmp9
        fsubs     fc0,tmp9,tmp9
        ps_madd   tmp3,tmp1,tmp1,tmp2
        fsubs     tT,tmp7,cT
        ps_merge00 cT,cT,cT
        ps_sum0   tmp4,tmp3,tmp1,tmp2
        frsqrte   tmp5,tmp4
        fmuls     tmp2,tmp5,tmp5
        fmuls     tmp3,tmp5,tmp9
        fnmsubs   tmp2,tmp2,tmp4,tmp8
        fmuls     tmp5,tmp2,tmp3
        ps_muls0  tmp0,tmp0,tmp5
        ps_muls0  tmp1,tmp1,tmp5
        ps_muls0  tmp4,tmp0,tT
        ps_muls0  tmp5,tmp1,tT
        ps_muls0  tmp9,tmp0,sT
        ps_muls1  tmp3,tmp4,tmp0
        ps_muls0  tmp2,tmp4,tmp0
        ps_muls0  tmp4,tmp4,tmp1
        fnmsubs   tmp6,tmp1,sT,tmp3
        ps_neg    tmp0,tmp9
        fmadds    tmp7,tmp1,sT,tmp3
        ps_sum0   tmp2,tmp2,tmp6,cT
        ps_sum0   tmp6,tmp0,fc0,tmp4
        ps_muls0  tmp5,tmp5,tmp1
        psq_st    tmp2,0(m),0,0
        ps_sum0   tmp8,tmp4,fc0,tmp9
        ps_sum0   tmp0,tmp4,tmp4,tmp0
        psq_st    tmp6,24(m),0,0
        ps_sum1   tmp3,cT,tmp7,tmp3
        ps_sum0   tmp5,tmp5,fc0,cT
        psq_st    tmp8,8(m),0,0
        ps_sum1   tmp4,tmp9,tmp0,tmp4
        psq_st    tmp3,16(m),0,0
        psq_st    tmp4,32(m),0,0
        psq_st    tmp5,40(m),0,0
    }
}

/* Builds the rotation about an arbitrary axis for an angle in radians. */
void PSMTXRotAxisRad(Mtx m, const Vec* axis, f32 rad)
{
    f32 sinA = (f32)sin(rad);
    f32 cosA = (f32)cos(rad);

    __PSMTXRotAxisRadInternal(m, axis, sinA, cosA);
}

/* Builds a translation matrix. */
void PSMTXTrans(register Mtx m, register f32 xT, register f32 yT, register f32 zT)
{
    register f32 c_zero;
    register f32 c_one;

    c_zero = 0.0f;
    c_one = 1.0f;
    asm {
        stfs      xT,12(m)
        stfs      yT,28(m)
        psq_st    c_zero,4(m),0,0
        psq_st    c_zero,32(m),0,0
        stfs      c_zero,16(m)
        stfs      c_one,20(m)
        stfs      c_zero,24(m)
        stfs      c_one,40(m)
        stfs      zT,44(m)
        stfs      c_one,0(m)
    }
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
void PSMTXScale(register Mtx m, register f32 xS, register f32 yS, register f32 zS)
{
    register f32 c_zero;

    c_zero = 0.0f;
    asm {
        stfs      xS,0(m)
        psq_st    c_zero,4(m),0,0
        psq_st    c_zero,12(m),0,0
        stfs      yS,20(m)
        psq_st    c_zero,24(m),0,0
        psq_st    c_zero,32(m),0,0
        stfs      zS,40(m)
        stfs      c_zero,44(m)
    }
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
void PSMTXQuat(register Mtx m, register const Quaternion* q)
{
    register f32 c_zero;
    register f32 c_one;
    register f32 c_two;
    register f32 tmp3;
    register f32 tmp4;
    register f32 tmp5;
    register f32 tmp6;
    register f32 tmp7;
    register f32 tmp8;
    register f32 tmp9;
    register f32 tmp10;
    register f32 tmp11;
    register f32 tmp12;
    register f32 tmp13;

    c_one = 1.0f;
    asm {
        psq_l     tmp4,0(q),0,0
        psq_l     tmp5,8(q),0,0
        ps_mul    tmp6,tmp4,tmp4
        ps_merge10 tmp9,tmp4,tmp4
        fsubs     c_zero,c_one,c_one
        ps_madd   tmp8,tmp5,tmp5,tmp6
        ps_muls1  tmp10,tmp5,tmp5
        psq_st    c_zero,12(m),1,0
        fadds     c_two,c_one,c_one
        ps_sum0   tmp3,tmp8,tmp8,tmp8
        psq_st    c_zero,44(m),1,0
        ps_mul    tmp7,tmp5,tmp5
        ps_madd   tmp12,tmp4,tmp9,tmp10
        fres      tmp13,tmp3
        ps_nmsub  tmp3,tmp3,tmp13,c_two
        ps_muls1  tmp11,tmp9,tmp5
        ps_msub   tmp10,tmp4,tmp9,tmp10
        ps_mul    tmp3,tmp13,tmp3
        ps_madds0 tmp9,tmp4,tmp5,tmp11
        ps_sum1   tmp8,tmp7,tmp8,tmp6
        fmuls     tmp3,tmp3,c_two
        ps_nmsub  tmp11,tmp11,c_two,tmp9
        ps_sum0   tmp6,tmp6,tmp6,tmp6
        ps_mul    tmp9,tmp9,tmp3
        ps_mul    tmp11,tmp11,tmp3
        ps_nmsub  tmp8,tmp8,tmp3,c_one
        psq_st    tmp9,8(m),1,0
        ps_mul    tmp12,tmp12,tmp3
        ps_mul    tmp10,tmp10,tmp3
        ps_merge10 tmp7,tmp11,c_zero
        ps_merge00 tmp5,tmp12,tmp8
        ps_merge10 tmp4,tmp8,tmp10
        psq_st    tmp7,24(m),0,0
        ps_merge01 tmp13,tmp11,tmp9
        ps_nmsub  tmp6,tmp6,tmp3,c_one
        psq_st    tmp5,16(m),0,0
        psq_st    tmp6,40(m),1,0
        psq_st    tmp4,0(m),0,0
        psq_st    tmp13,32(m),0,0
    }
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

    tmp = 1.0f / (r - l);
    m[0][0] = ((2.0f * n) * tmp) * scaleS;
    m[0][1] = 0.0f;
    m[0][2] = (((r + l) * tmp) * scaleS) - transS;
    m[0][3] = 0.0f;
    tmp = 1.0f / (t - b);
    m[1][0] = 0.0f;
    m[1][1] = ((2.0f * n) * tmp) * scaleT;
    m[1][2] = (((t + b) * tmp) * scaleT) - transT;
    m[1][3] = 0.0f;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = -1.0f;
    m[2][3] = 0.0f;
}

/* Builds the texture-projection matrix of a perspective view (field of view in degrees). */
void C_MTXLightPerspective(Mtx m, f32 fovY, f32 aspect, f32 scaleS, f32 scaleT, f32 transS, f32 transT)
{
    f32 angle;
    f32 cot;

    angle = 0.5f * fovY;
    angle = 0.017453292f * angle;
    cot = 1.0f / (f32)tan(angle);
    m[0][0] = (cot / aspect) * scaleS;
    m[0][1] = 0.0f;
    m[0][2] = -transS;
    m[0][3] = 0.0f;
    m[1][0] = 0.0f;
    m[1][1] = cot * scaleT;
    m[1][2] = -transT;
    m[1][3] = 0.0f;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = -1.0f;
    m[2][3] = 0.0f;
}

/* Builds the texture-projection matrix of an orthographic view. */
void C_MTXLightOrtho(Mtx m, f32 t, f32 b, f32 l, f32 r, f32 scaleS, f32 scaleT, f32 transS, f32 transT)
{
    f32 tmp;

    tmp = 1.0f / (r - l);
    m[0][0] = (2.0f * tmp) * scaleS;
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[0][3] = ((-(r + l) * tmp) * scaleS) + transS;
    tmp = 1.0f / (t - b);
    m[1][0] = 0.0f;
    m[1][1] = (2.0f * tmp) * scaleT;
    m[1][2] = 0.0f;
    m[1][3] = ((-(t + b) * tmp) * scaleT) + transT;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = 0.0f;
    m[2][3] = 1.0f;
}


f32 sMtxZeroOnePair[2] = { 0.0f, 1.0f }; /* .sdata 0x80793F00 */

