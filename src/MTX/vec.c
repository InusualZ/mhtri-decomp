/*
 * MTX/vec.c - the MTX library vector routines: normalise, magnitude, dot and cross product, half-angle vector and
 *    squared distance.
 * RANGE. .text 0x804C6B60-0x804C6D70 (6 functions); .sdata2 0x8079D2B0-0x8079D2C0.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the dump names `PSVECNormalize`..`PSVECSquareDistance`; the 16-byte pool at
 *    0x8079D2B0 (0.5, 3.0 for the reciprocal-square-root step, and a zero) is read only by this range; `PSVECAdd`
 *    (0x804C6B30) is the registered `MTX/mtx44.c`.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. `PSVEC*` rows are the map's names; GUESS: `C_VECHalfAngle` (the SDK entry the body implements), `sVecHalf`, `sVecThree`,
 *    `sVecZero` (the constants by value); GUESS: `PSVECMag` (the map's row; the dump
 *    names a different function at 0x804C6BB0, the body is the vector length).
 * RESIDUALS. the object's .sdata2 and .text end short of the claimed ranges by their alignment padding (12 of 16 B, 520 of
 *    528 B); the link pads them.
 * SHAPES. the paired-single bodies are asm functions with `nofralloc` (playbook 104); the pool constants are globals
 *    declared before the bodies and defined after them so the compiler loads them instead of folding.
 */


#include "types.h"

#include "MTX/mtx44.h"
#include "MTX/vec.h"


extern const f32 sVecHalf;
extern const f32 sVecThree;
extern const f32 sVecZero;

/* Scales a vector to unit length. */
asm void PSVECNormalize(register const Vec* src, register Vec* unit)
{
    nofralloc
    psq_l     f2,0(r3),0,0
    psq_l     f3,8(r3),1,0
    ps_mul    f5,f2,f2
    lfs       f0,sVecHalf(r0)
    lfs       f1,sVecThree(r0)
    ps_madd   f4,f3,f3,f5
    ps_sum0   f4,f4,f3,f5
    frsqrte   f5,f4
    fmuls     f6,f5,f5
    fmuls     f0,f5,f0
    fnmsubs   f6,f6,f4,f1
    fmuls     f5,f6,f0
    ps_muls0  f2,f2,f5
    ps_muls0  f3,f3,f5
    psq_st    f2,0(r4),0,0
    psq_st    f3,8(r4),1,0
    blr       
}

/* Returns the length of a vector. */
asm f32 PSVECMag(register const Vec* v)
{
    nofralloc
    psq_l     f0,0(r3),0,0
    lfs       f4,sVecHalf(r0)
    ps_mul    f0,f0,f0
    lfs       f1,8(r3)
    fsubs     f2,f4,f4
    ps_madd   f1,f1,f1,f0
    ps_sum0   f1,f1,f0,f0
    fcmpu     cr0,f1,f2
    beqlr     
    frsqrte   f0,f1
    lfs       f3,sVecThree(r0)
    fmuls     f2,f0,f0
    fmuls     f0,f0,f4
    fnmsubs   f2,f2,f1,f3
    fmuls     f0,f2,f0
    fmuls     f1,f1,f0
    blr       
}

/* Returns the dot product of two vectors. */
asm f32 PSVECDotProduct(register const Vec* a, register const Vec* b)
{
    nofralloc
    psq_l     f2,4(r3),0,0
    psq_l     f3,4(r4),0,0
    ps_mul    f2,f2,f3
    psq_l     f5,0(r3),0,0
    psq_l     f4,0(r4),0,0
    ps_madd   f3,f5,f4,f2
    ps_sum0   f1,f3,f2,f2
    blr       
}

/* Computes the cross product of two vectors. */
asm void PSVECCrossProduct(register const Vec* a, register const Vec* b, register Vec* axb)
{
    nofralloc
    psq_l     f1,0(r4),0,0
    lfs       f2,8(r3)
    psq_l     f0,0(r3),0,0
    ps_merge10 f6,f1,f1
    lfs       f3,8(r4)
    ps_mul    f4,f1,f2
    ps_muls0  f7,f1,f0
    ps_msub   f5,f0,f3,f4
    ps_msub   f8,f0,f6,f7
    ps_merge11 f9,f5,f5
    ps_merge01 f10,f5,f8
    psq_st    f9,0(r5),1,0
    ps_neg    f10,f10
    psq_st    f10,4(r5),0,0
    blr       
}

/* Computes the unit vector halfway between the reversed vectors `a` and `b`. */
void C_VECHalfAngle(const Vec* a, const Vec* b, Vec* half)
{
    Vec aTmp;
    Vec bTmp;
    Vec hTmp;

    aTmp.x = -a->x;
    aTmp.y = -a->y;
    aTmp.z = -a->z;
    bTmp.x = -b->x;
    bTmp.y = -b->y;
    bTmp.z = -b->z;
    PSVECNormalize(&aTmp, &aTmp);
    PSVECNormalize(&bTmp, &bTmp);
    PSVECAdd(&aTmp, &bTmp, &hTmp);
    if (PSVECDotProduct(&hTmp, &hTmp) > sVecZero)
        PSVECNormalize(&hTmp, half);
    else
        *half = hTmp;
}

/* Returns the squared distance between two points. */
asm f32 PSVECSquareDistance(register const Vec* a, register const Vec* b)
{
    nofralloc
    psq_l     f0,4(r3),0,0
    psq_l     f1,4(r4),0,0
    psq_l     f2,0(r3),0,0
    ps_sub    f3,f0,f1
    psq_l     f0,0(r4),0,0
    ps_sub    f0,f2,f0
    ps_mul    f3,f3,f3
    ps_madd   f1,f0,f0,f3
    ps_sum0   f1,f1,f3,f3
    blr       
}


const f32 sVecHalf = 0.5f;  /* .sdata2 0x8079D2B0 */
const f32 sVecThree = 3.0f; /* .sdata2 0x8079D2B4 */
const f32 sVecZero = 0.0f;  /* .sdata2 0x8079D2B8 */
