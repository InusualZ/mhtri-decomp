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

struct LightWork;

#ifdef __cplusplus
extern "C" {
#endif

struct LightWork* fn_802BECD0(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LIGHT_LIGHT_H */
