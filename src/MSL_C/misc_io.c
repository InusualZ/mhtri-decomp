/*
 * MSL_C/misc_io.c - float classification (`__fpclassifyf`, `__signbitd`, `__fpclassifyd`) and `__stdio_atexit`.
 *
 * RANGE. .text 0x8045B9D8..0x8045BADC (4 functions in the map, 0x104 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS; the unit merges two small groups that may be two files.
 * EVIDENCE. family and adjacency only: the classifiers are called by the number-conversion unit, `s_ldexp` and
 *    `g3d/g3d_anmchr.cpp`, `__stdio_atexit` stores the stdio exit hook (`.sbss` 0x80794E1C, defined by the
 *    abort/exit unit).
 * RESIDUALS. COARSE: no data separates the classifiers from `__stdio_atexit`.
 * SHAPES. the classifiers read the exponent and mantissa words through a pointer to the argument.
 */
#include "MSL_C/misc_io.h"
#include "MSL_C/__fpclassifyd.h"
#include "MSL_C/abort_exit.h"
#include "MSL_C/ansi_files.h"

#define HIGH_WORD(x) (((u32*)&(x))[0])
#define LOW_WORD(x) (((u32*)&(x))[1])

int __fpclassifyf(f32 x)
{
    switch (*(u32*)&x & 0x7F800000) {
    case 0x7F800000:
        if ((*(u32*)&x & 0x7FFFFF) != 0) {
            return FP_NAN;
        }
        return FP_INFINITE;
    case 0:
        if ((*(u32*)&x & 0x7FFFFF) != 0) {
            return FP_SUBNORMAL;
        }
        return FP_ZERO;
    }
    return FP_NORMAL;
}

s32 __signbitd(f64 x)
{
    return HIGH_WORD(x) & 0x80000000;
}

int __fpclassifyd(f64 x)
{
    switch (HIGH_WORD(x) & 0x7FF00000) {
    case 0x7FF00000:
        if ((HIGH_WORD(x) & 0xFFFFF) != 0 || LOW_WORD(x) != 0) {
            return FP_NAN;
        }
        return FP_INFINITE;
    case 0:
        if ((HIGH_WORD(x) & 0xFFFFF) != 0 || LOW_WORD(x) != 0) {
            return FP_SUBNORMAL;
        }
        return FP_ZERO;
    }
    return FP_NORMAL;
}

void __stdio_atexit(void)
{
    __stdio_exit = __close_all;
}
