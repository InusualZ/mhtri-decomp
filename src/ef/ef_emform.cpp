/*
 * ef/ef_emform.cpp - the emitter-form registry: `EmitterFormBuilder::Create` maps a shape id to one of the seven
 *   registered shape instances, which this unit defines; their static initializer, their implicit constructors
 *   (each runs the abstract base's, which installs `EmitterForm`'s table, then installs its own shape's table) and
 *   the two tables this unit owns (`EmitterFormBuilder`'s and the abstract `EmitterForm`'s) come out of the
 *   compiler.
 * RANGE. .text 0x800CCCF8-0x800CCFB0 (10 functions); extab 0x8000A584-0x8000A5CC, extabindex 0x80023BEC-0x80023C58,
 *   .ctors 0x8056F2E8-0x8056F2EC, .data 0x80594E98-0x80594EE0 (the `__FILE__` string "ef_emform.cpp" first),
 *   .sbss 0x80794938-0x80794958 (the seven instances).
 * FLAGS. `cflags_main` plus `-pool off` (configure.py: retail addresses each string with its own `lis`/`addi`);
 *   file-wide `#pragma peephole off` (retail computes each table address into r0, `addi r0, r4, sym@l`, where the
 *   peephole pass reuses the `lis` register).
 * NAMES. The class names are nw4r's (`nw4r::ef::EmitterForm*`); the instance names are GUESSES.
 *   GUESS: `sEmitterFormDisc` .. `sEmitterFormPoint` (the seven `.sbss` instances, named for their class).
 *   GUESS: `ef_emform_file_name`, `ef_emform_err_false` (the assert's file and message strings).
 * RESIDUALS. none known.
 * SHAPES. The lookup's id is signed, and its cases are in retail's order (0, 1, 7, 8, 10, 5, 9): that is the
 *   `cmpwi` chain MWCC emits.  The instances are defined in retail's construction order (disc, line, cylinder,
 *   sphere, torus, cube, point); the implicit constructors come out after the static initializer in the reverse
 *   of that order, each followed by the first constructor it calls.
 */

#include "ef/ef_emform.h"

#pragma peephole off
#pragma dont_inline on

/* The unit's `.data` strings: the file name and the failed-assertion message (the two tables follow them). */
char ef_emform_file_name[] = "ef_emform.cpp";
char ef_emform_err_false[] = "NW4R:Failed assertion false";

namespace nw4r {
namespace ef {

/* The seven registered instances, in construction order. */
static EmitterFormDisc sEmitterFormDisc;
static EmitterFormLine sEmitterFormLine;
static EmitterFormCylinder sEmitterFormCylinder;
static EmitterFormSphere sEmitterFormSphere;
static EmitterFormTorus sEmitterFormTorus;
static EmitterFormCube sEmitterFormCube;
static EmitterFormPoint sEmitterFormPoint;

/* 0x800CCCF8 (0xB0): returns the registered instance of shape `id`, or asserts and returns null. */
EmitterForm* EmitterFormBuilder::Create(int id) {
    switch (id) {
    case 0:  return &sEmitterFormDisc;
    case 1:  return &sEmitterFormLine;
    case 7:  return &sEmitterFormCylinder;
    case 8:  return &sEmitterFormSphere;
    case 10: return &sEmitterFormTorus;
    case 5:  return &sEmitterFormCube;
    case 9:  return &sEmitterFormPoint;
    default:
        nw4r::db::Panic(ef_emform_file_name, 65, ef_emform_err_false);
        return 0;
    }
}

} // namespace ef
} // namespace nw4r

