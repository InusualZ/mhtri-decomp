/*
 * MTX/mtx44.c - the MTX library 4x4 projection matrices (frustum, perspective, orthographic) and `PSVECAdd`.
 * RANGE. .text 0x804C6900-0x804C6B60 (4 functions); .sdata2 0x8079D298-0x8079D2B0.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the 24-byte pool at 0x8079D298 (1.0, 2.0, 0.0, -1.0, 0.5, degrees-to-radians) is
 *    read only by the three projection bodies; `tan` is called by the perspective body.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. `C_MTXOrtho` and `PSVECAdd` are the map's names; GUESS: `C_MTXFrustum`, `C_MTXPerspective` (the SDK entries
 *    the bodies implement).
 * RESIDUALS. none.
 * SHAPES. `PSVECAdd` is a paired-single asm function with `nofralloc` (playbook 104); every constant is a literal, so
 *    the compiler pools them in first-use order (1, 2, 0, -1, 0.5, deg-to-rad), the target's .sdata2 order.
 */


#include "types.h"

#include "MSL/s_tan.h"
#include "MTX/mtx44.h"


#pragma fp_contract off


/* Builds the perspective projection of a view frustum. */
void C_MTXFrustum(Mtx44 m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f)
{
    f32 tmp;

    tmp = 1.0f / (r - l);
    m[0][0] = (2.0f * n) * tmp;
    m[0][1] = 0.0f;
    m[0][2] = (r + l) * tmp;
    m[0][3] = 0.0f;
    tmp = 1.0f / (t - b);
    m[1][0] = 0.0f;
    m[1][1] = (2.0f * n) * tmp;
    m[1][2] = (t + b) * tmp;
    m[1][3] = 0.0f;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    tmp = 1.0f / (f - n);
    m[2][2] = -n * tmp;
    m[2][3] = tmp * -(f * n);
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = -1.0f;
    m[3][3] = 0.0f;
}

/* Builds the perspective projection of a vertical field of view (degrees) and an aspect ratio. */
void C_MTXPerspective(Mtx44 m, f32 fovY, f32 aspect, f32 n, f32 f)
{
    f32 angle;
    f32 cot;
    f32 tmp;

    angle = 0.5f * fovY;
    angle = 0.017453292f * angle;
    cot = 1.0f / (f32)tan(angle);
    m[0][0] = cot / aspect;
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[0][3] = 0.0f;
    m[1][0] = 0.0f;
    m[1][1] = cot;
    m[1][2] = 0.0f;
    m[1][3] = 0.0f;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    tmp = 1.0f / (f - n);
    m[2][2] = -n * tmp;
    m[2][3] = tmp * -(f * n);
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = -1.0f;
    m[3][3] = 0.0f;
}

/* Builds the orthographic projection of a view box. */
void C_MTXOrtho(Mtx44 m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f)
{
    f32 tmp;

    tmp = 1.0f / (r - l);
    m[0][0] = 2.0f * tmp;
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[0][3] = tmp * -(r + l);
    tmp = 1.0f / (t - b);
    m[1][0] = 0.0f;
    m[1][1] = 2.0f * tmp;
    m[1][2] = 0.0f;
    m[1][3] = tmp * -(t + b);
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    tmp = 1.0f / (f - n);
    m[2][2] = -1.0f * tmp;
    m[2][3] = -f * tmp;
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

/* Adds two vectors. */
asm void PSVECAdd(register const Vec* a, register const Vec* b, register Vec* ab)
{
    nofralloc
    psq_l     f2,0(r3),0,0
    psq_l     f4,0(r4),0,0
    ps_add    f6,f2,f4
    psq_st    f6,0(r5),0,0
    psq_l     f3,8(r3),1,0
    psq_l     f5,8(r4),1,0
    ps_add    f7,f3,f5
    psq_st    f7,8(r5),1,0
    blr       
}


