/*
 * MTX/mtxvec.c - the MTX library matrix-vector product.
 * RANGE. .text 0x804C68A0-0x804C6900 (1 function).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the dump
 *    names the single body `PSMTXMultVec`; no data.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. `PSMTXMultVec` is the map's name.
 * RESIDUALS. none: the body matches; the object's .text ends 12 bytes before the claimed end (alignment padding).
 * SHAPES. a paired-single asm function with `nofralloc` (playbook 104).
 */


#include "types.h"

#include "MTX/mtxvec.h"



/* Transforms a point by a 3x4 matrix. */
asm void PSMTXMultVec(register const Mtx m, register const Vec* src, register Vec* dst)
{
    nofralloc
    psq_l     f0,0(r4),0,0
    psq_l     f2,0(r3),0,0
    psq_l     f1,8(r4),1,0
    ps_mul    f4,f2,f0
    psq_l     f3,8(r3),0,0
    ps_madd   f5,f3,f1,f4
    psq_l     f8,16(r3),0,0
    ps_sum0   f6,f5,f6,f5
    psq_l     f9,24(r3),0,0
    ps_mul    f10,f8,f0
    psq_st    f6,0(r5),1,0
    ps_madd   f11,f9,f1,f10
    psq_l     f2,32(r3),0,0
    ps_sum0   f12,f11,f12,f11
    psq_l     f3,40(r3),0,0
    ps_mul    f4,f2,f0
    psq_st    f12,4(r5),1,0
    ps_madd   f5,f3,f1,f4
    ps_sum0   f6,f5,f6,f5
    psq_st    f6,8(r5),1,0
    blr       
}

