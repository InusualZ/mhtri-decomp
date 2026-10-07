/*
 * MTX/quat.c - the MTX library quaternion routines: matrix-to-quaternion conversion and spherical interpolation.
 * RANGE. .text 0x804C6D70-0x804C70E0 (2 functions); .rodata 0x80573A00-0x80573A10; .sdata2 0x8079D2C0-0x8079D2D0.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the 16-byte .rodata record holds the axis-successor table
 *    {1, 2, 0} that the conversion copies to its stack; the 16-byte .sdata2 pool (1.0, 0.0, 0.5, 0.99999) is read only by
 *    the two bodies, which call `sqrt`, `sin` and `acos`.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. GUESS: `QUATMtx` (convert a 3x4 matrix to a quaternion) and `QUATSlerp` (spherical interpolation), named from the
 *    bodies; `sQuatOne`, `sQuatZero`, `sQuatHalf`, `sQuatSlerpEpsilon`, `sQuatNextAxis` (the constants by value and role).
 * RESIDUALS. `QUATMtx` (97.7 %): the float temporaries of both branches take one register more than the target's (f7/f8, f4/f5)
 *    and the three index registers rotate; `QUATSlerp` (99.6 %): the product of the sign and the scale lands in a
 *    volatile register where the target keeps it in the sign's saved register.  The 12-byte {1, 2, 0} table is the compiler's
 *    own local-array copy source (`@N`), so the map row `sQuatNextAxis` differs by name in relocdiff.
 * SHAPES. both bodies are C; the pool constants are globals declared before the bodies and defined after them so the
 *    compiler loads them instead of folding.
 */

#include "types.h"

#include "MSL/s_sin.h"
#include "MSL/w_math.h"
#include "MTX/mtx.h"

#pragma fp_contract off

extern const f32 sQuatOne;
extern const f32 sQuatZero;
extern const f32 sQuatHalf;
extern const f32 sQuatSlerpEpsilon;

/* Converts the rotation of a 3x4 matrix to a quaternion. */
void QUATMtx(Quaternion* r, const Mtx m)
{
    f32 tr;
    f32 s;
    s32 i;
    s32 j;
    s32 k;
    s32 next[3] = { 1, 2, 0 };
    f32 q[3];

    tr = m[2][2] + (m[0][0] + m[1][1]);
    if (tr > sQuatZero) {
        s = (f32)sqrt(sQuatOne + tr);
        r->w = sQuatHalf * s;
        s = sQuatHalf / s;
        r->x = (m[2][1] - m[1][2]) * s;
        r->y = (m[0][2] - m[2][0]) * s;
        r->z = (m[1][0] - m[0][1]) * s;
    } else {
        i = 0;
        if (m[1][1] > m[0][0])
            i = 1;
        if (m[2][2] > m[i][i])
            i = 2;
        j = next[i];
        k = next[j];
        s = (f32)sqrt(sQuatOne + (m[i][i] - (m[j][j] + m[k][k])));
        q[i] = sQuatHalf * s;
        if (s != sQuatZero)
            s = sQuatHalf / s;
        q[j] = (m[i][j] + m[j][i]) * s;
        q[k] = (m[i][k] + m[k][i]) * s;
        r->w = (m[k][j] - m[j][k]) * s;
        r->x = q[0];
        r->y = q[1];
        r->z = q[2];
    }
}

/* Interpolates between two quaternions along the shorter arc. */
void QUATSlerp(const Quaternion* p, const Quaternion* q, Quaternion* r, f32 t)
{
    f32 cosTheta;
    f32 scale0;
    f32 sign;
    f32 theta;
    f32 sinTheta;
    f32 scale1;

    sign = sQuatOne;
    cosTheta = p->x * q->x + p->y * q->y + p->z * q->z + p->w * q->w;
    if (cosTheta < sQuatZero) {
        cosTheta = -cosTheta;
        sign = -sign;
    }
    if (cosTheta <= sQuatSlerpEpsilon) {
        theta = (f32)acos(cosTheta);
        sinTheta = (f32)sin(theta);
        scale0 = (f32)sin((sQuatOne - t) * theta) / sinTheta;
        scale1 = sign * ((f32)sin(t * theta) / sinTheta);
    } else {
        scale1 = sign * t;
        scale0 = sQuatOne - t;
    }
    r->x = scale0 * p->x + scale1 * q->x;
    r->y = scale0 * p->y + scale1 * q->y;
    r->z = scale0 * p->z + scale1 * q->z;
    r->w = scale0 * p->w + scale1 * q->w;
}

const f32 sQuatOne = 1.0f;                /* .sdata2 0x8079D2C0 */
const f32 sQuatZero = 0.0f;               /* .sdata2 0x8079D2C4 */
const f32 sQuatHalf = 0.5f;               /* .sdata2 0x8079D2C8 */
const f32 sQuatSlerpEpsilon = 0.99999f;   /* .sdata2 0x8079D2CC */
