/*
 * MTX/mtx44.c - the MTX library 4x4 projection matrices (frustum, perspective, orthographic) and `PSVECAdd`.
 * RANGE. .text 0x804C6900-0x804C6B60 (4 functions); .sdata2 0x8079D298-0x8079D2B0.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the 24-byte pool at 0x8079D298 (1.0, 2.0, 0.0, -1.0, 0.5, degrees-to-radians) is
 *    read only by the three projection bodies; `tan` is called by the perspective body.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. `C_MTXOrtho` and `PSVECAdd` are the map's names; GUESS: `C_MTXFrustum`, `C_MTXPerspective` (the SDK entries
 *    the bodies implement), `sMtx44One`, `sMtx44Two`, `sMtx44Zero`, `sMtx44MinusOne`, `sMtx44Half`, `sMtx44DegToRad` (the
 *    constants by value).
 * RESIDUALS. `C_MTXFrustum` (99.5 %): the two `(2 * n) * tmp` scale multiplies take their operands in the opposite order
 *    whichever way the source writes them.
 * SHAPES. `PSVECAdd` is a paired-single asm function with `nofralloc` (playbook 104); the pool constants are globals
 *    declared before the bodies and defined after them so the compiler loads them instead of folding.
 */


#include "types.h"

#include "MSL/s_tan.h"
#include "MTX/mtx44.h"


#pragma fp_contract off

extern const f32 sMtx44One;
extern const f32 sMtx44Two;
extern const f32 sMtx44Zero;
extern const f32 sMtx44MinusOne;
extern const f32 sMtx44Half;
extern const f32 sMtx44DegToRad;

/* Builds the perspective projection of a view frustum. */
void C_MTXFrustum(Mtx44 m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f)
{
    f32 tmp;

    tmp = sMtx44One / (r - l);
    m[0][0] = (sMtx44Two * n) * tmp;
    m[0][1] = sMtx44Zero;
    m[0][2] = (r + l) * tmp;
    m[0][3] = sMtx44Zero;
    tmp = sMtx44One / (t - b);
    m[1][0] = sMtx44Zero;
    m[1][1] = (sMtx44Two * n) * tmp;
    m[1][2] = (t + b) * tmp;
    m[1][3] = sMtx44Zero;
    m[2][0] = sMtx44Zero;
    m[2][1] = sMtx44Zero;
    tmp = sMtx44One / (f - n);
    m[2][2] = -n * tmp;
    m[2][3] = tmp * -(f * n);
    m[3][0] = sMtx44Zero;
    m[3][1] = sMtx44Zero;
    m[3][2] = sMtx44MinusOne;
    m[3][3] = sMtx44Zero;
}

/* Builds the perspective projection of a vertical field of view (degrees) and an aspect ratio. */
void C_MTXPerspective(Mtx44 m, f32 fovY, f32 aspect, f32 n, f32 f)
{
    f32 angle;
    f32 cot;
    f32 tmp;

    angle = sMtx44Half * fovY;
    angle = sMtx44DegToRad * angle;
    cot = sMtx44One / (f32)tan(angle);
    m[0][0] = cot / aspect;
    m[0][1] = sMtx44Zero;
    m[0][2] = sMtx44Zero;
    m[0][3] = sMtx44Zero;
    m[1][0] = sMtx44Zero;
    m[1][1] = cot;
    m[1][2] = sMtx44Zero;
    m[1][3] = sMtx44Zero;
    m[2][0] = sMtx44Zero;
    m[2][1] = sMtx44Zero;
    tmp = sMtx44One / (f - n);
    m[2][2] = -n * tmp;
    m[2][3] = tmp * -(f * n);
    m[3][0] = sMtx44Zero;
    m[3][1] = sMtx44Zero;
    m[3][2] = sMtx44MinusOne;
    m[3][3] = sMtx44Zero;
}

/* Builds the orthographic projection of a view box. */
void C_MTXOrtho(Mtx44 m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f)
{
    f32 tmp;

    tmp = sMtx44One / (r - l);
    m[0][0] = sMtx44Two * tmp;
    m[0][1] = sMtx44Zero;
    m[0][2] = sMtx44Zero;
    m[0][3] = tmp * -(r + l);
    tmp = sMtx44One / (t - b);
    m[1][0] = sMtx44Zero;
    m[1][1] = sMtx44Two * tmp;
    m[1][2] = sMtx44Zero;
    m[1][3] = tmp * -(t + b);
    m[2][0] = sMtx44Zero;
    m[2][1] = sMtx44Zero;
    tmp = sMtx44One / (f - n);
    m[2][2] = sMtx44MinusOne * tmp;
    m[2][3] = -f * tmp;
    m[3][0] = sMtx44Zero;
    m[3][1] = sMtx44Zero;
    m[3][2] = sMtx44Zero;
    m[3][3] = sMtx44One;
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


const f32 sMtx44One = 1.0f;              /* .sdata2 0x8079D298 */
const f32 sMtx44Two = 2.0f;              /* .sdata2 0x8079D29C */
const f32 sMtx44Zero = 0.0f;             /* .sdata2 0x8079D2A0 */
const f32 sMtx44MinusOne = -1.0f;        /* .sdata2 0x8079D2A4 */
const f32 sMtx44Half = 0.5f;             /* .sdata2 0x8079D2A8 */
const f32 sMtx44DegToRad = 0.017453292f; /* .sdata2 0x8079D2AC */
