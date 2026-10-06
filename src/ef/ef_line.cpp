/*
 * ef/ef_line.cpp - the line emitter form (`EmitterFormLine::Emission`): validates `em`/`pm`/`params`, walks
 *   `count` samples along the direction three `SinCos` Euler angles build, projects each through `CalcVelocity`
 *   and submits it through the particle manager's `CreateParticle`.
 * RANGE. .text 0x800CCFB0-0x800CD584 (1 function); extab 0x8000A5CC-0x8000A5D4, extabindex 0x80023C58-0x80023C64,
 *   .data 0x80594EE0-0x80594F98 (the `__FILE__` string "ef_line.cpp", the three assert messages, the class's
 *   table at 0x80594F8C), .sdata2 0x807962E8-0x80796300.
 * FLAGS. `cflags_main` plus `-pool off`; file-wide `#pragma fp_contract off` (retail's FP is unfused `fmuls` +
 *   `fadds`) and `#pragma peephole off` (retail keeps the paired-single epilogue as `li r0,off; psq_lx`).
 * NAMES. The class and method are nw4r's (the asserts' `em`/`pm`/`params` are its parameter names).
 *   GUESS: `ef_line_file_name`, `ef_line_err_em`, `ef_line_err_pm`, `ef_line_err_params` (their text).
 * RESIDUALS. none known: every section is byte-identical to the target object.
 * SHAPES. The function is `nw4r::ef::EmitterFormLine::Emission` (the class is declared in `ef/ef_emform.h`; this unit
 *   defines its key function, so the compiler emits the class's table here, after the strings).  The
 *   particle manager's spawn is its virtual `CreateParticle`, taking the position and velocity by value: the call
 *   copies them (velocity first), evaluates the momentum before the life and dispatches through r3.  The float
 *   argument precedes the space matrix (nw4r's `Emission(..., u16 life, f32 lifeRnd, const MTX34* space)`).
 * SHAPES. The strings are global definitions in retail's order; `-pool off` (configure.py) gives each its own
 *   `lis`/`addi`, where the default pools them off one base register.
 * SHAPES. The `.sdata2` constants are literals: MWCC pools them in retail's order.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/ef_emitter.h" /* ef_random_float (rule 2) */
#include "ef/ef_emform.h" /* nw4r::ef::EmitterFormLine, EfWork, ParticleManager (rule 1) */

#pragma fp_contract off
#pragma peephole off

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }
/* The unit's `.data` strings, in retail order: the file name and one message per checked pointer (the
 * class's table follows them). */
char ef_line_file_name[] = "ef_line.cpp";
char ef_line_err_em[] = "NW4R:Pointer Error\nem(=%p) is not valid pointer.";
char ef_line_err_pm[] = "NW4R:Pointer Error\npm(=%p) is not valid pointer.";
char ef_line_err_params[] = "NW4R:Pointer Error\nparams(=%p) is not valid pointer.";

/* NW4R_POINTER_ASSERT's RVL address-range check (MEM1/MEM2, cached and uncached, plus locked cache). */
#define NW4R_VALID_PTR(p)                                                                          \
    (((u32)(p) & 0xFF000000) == 0x80000000 || ((u32)(p) & 0xFF800000) == 0x81000000 ||             \
     ((u32)(p) & 0xF8000000) == 0x90000000 || ((u32)(p) & 0xFF000000) == 0xC0000000 ||             \
     ((u32)(p) & 0xFF800000) == 0xC1000000 || ((u32)(p) & 0xF8000000) == 0xD0000000 ||             \
     ((u32)(p) & 0xFFFFC000) == 0xE0000000)

#define NW4R_POINTER_ASSERT(p, line, msg)                                                          \
    (NW4R_VALID_PTR(p) ? (void)0 : nw4r::db::Panic(ef_line_file_name, line, msg, (p)))

/* 0x800CCFB0 (0x5D4): emits `count` particles along the line the three rotation angles turn, at random or evenly
 * spaced (flag 0x20000), centred on the origin with flag 0x04000000. */
void nw4r::ef::EmitterFormLine::Emission(EfWork* em, ParticleManager* pm, int count, u32 optionFlag, f32* params,
                                         u16 aPtclLife, f32 aPtclLifeRnd, const MTX34* space) {
    NW4R_POINTER_ASSERT(em, 42, ef_line_err_em);
    NW4R_POINTER_ASSERT(pm, 43, ef_line_err_pm);
    NW4R_POINTER_ASSERT(params, 44, ef_line_err_params);

    if (count >= 1) {
        int i;
        for (i = 0; i < count; i++) {
            f32 pos;
            f32 sx, cx, sy, cy, sz, cz;
            VEC3 p, normal, fromYAxis, v;

            if ((optionFlag & 0x00020000) == 0) {
                pos = ef_random_float(&em->progress);
            } else if (count > 1) {
                pos = (f32)i / (f32)(count - 1);
            } else {
                pos = 0.0f;
            }
            if ((optionFlag & 0x04000000) != 0)
                pos -= 0.5f;
            pos *= params[0];

            ef_sin_cos(&sx, &cx, params[1]);
            ef_sin_cos(&sy, &cy, params[2]);
            ef_sin_cos(&sz, &cz, params[3]);

            VEC3_ctor(&p);
            p.x = (cx * cz * sy + sx * sz) * pos;
            p.y = (-cz * sx + cx * sy * sz) * pos;
            p.z = (cx * cy) * pos;

            VEC3_ctor(&normal);
            normal.x = 0.0f;
            normal.y = 1.0f;
            normal.z = 0.0f;

            setVec3(&fromYAxis, p.x, 0.0f, p.z);

            VEC3_ctor(&v);
            CalcVelocity((Vec*)&v, em, (Vec*)&p, (Vec*)&normal, (Vec*)&p, (Vec*)&fromYAxis);

            pm->CreateParticle(CalcLife(aPtclLife, aPtclLifeRnd, em), p, v, space,
                               1.0f + 0.01f * (f32)(s32)em->scale_rate *
                                                  ef_random_float(&em->progress),
                               &em->spawn_data, em->spawn_extra, em->spawn_flag);
        }
    }
}
