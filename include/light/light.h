/* The declarations owned by `light/light.cpp` (docs/plan.md 6.5 rule 2).
 *
 * `fn_802BECD0` (0x802BECD0, the first function of the unit's range) is the light-work accessor: it
 * returns `lbl_806BB7E0` or `lbl_806BB7E0 + 0x4F8`, the two 0x4F8-byte light-work records the module
 * constructs.  `include/unsplit/camera.h` declared it `struct CamWork*` while the address had no
 * registered owner; the owner defines it over its own view of that record (`LightWork`, size 0x4F8),
 * and the two spellings in one translation unit are the `(10505) illegal overloading` this move
 * clears.
 *
 * `struct LightWork` is only forward-declared here: the record's definition stays with the two units
 * that view it (the rule-1 residual `include/unsplit/camera.h` and `src/camera/fn_802B5C58.cpp`
 * record), so a consumer takes the accessor through its own view name.
 */
#ifndef MHTRI_LIGHT_LIGHT_H
#define MHTRI_LIGHT_LIGHT_H

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"

struct LightWork;

#ifdef __cplusplus
extern "C" {
#endif

struct LightWork* fn_802BECD0(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* The two setters this unit owns; the map names `set_amblight__FUc8_GXColor` and
 * `make_dir_light2__FlPQ34nw4r4math4VEC38_GXColorl` are their manglings, so the declarations sit at
 * C++ scope and the front-end reproduces them (docs/plan.md 6.5 rule 9).  `_GXColor` is passed by
 * value in the first and by pointer in the second - that is what the target's own call sites do
 * (`quest/arenatask.cpp`'s `arena_light_init` hands over a 4-byte colour word for both, and the
 * target's own callees read it through the pointer MWCC makes for a by-value struct).  Added with
 * that unit's body pass (rule 2: this range owns both addresses). */
void set_amblight(u8 id, _GXColor color);
void make_dir_light2(s32 index, nw4r::math::VEC3* vec, _GXColor color, s32 flag);
#endif

#endif /* MHTRI_LIGHT_LIGHT_H */
