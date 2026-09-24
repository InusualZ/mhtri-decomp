/*
 * nw4r g3d: g3d_resanmcamera.cpp - the camera animation channel setter, `.text` 0x8008A220-0x8008A28C
 * (2 functions).
 *
 * Re-cut from `auto/800898B0_fn_800898B0.c` (docs/plan.md 12 item 5).  `fn_8008A220` cites the
 * `g3d_resuser_ac.h` header string (lbl_8058FFFC), and the `g3d_resanmcamera.cpp` data fragment is
 * 0x8058FF20-0x80590010 (tudiscover's right boundary at 0x8008A28C), so the range is
 * `g3d_resanmcamera.cpp`.
 *
 * rule 7 deferred: the map carries only `fn_XXXXXXXX` names here (docs/plan.md 6.5 rule 7); renaming a
 * symbol needs the map and the source in one edit (playbook 31).
 *
 * Language: langcheck says the retail TU is C++, but the source is C-idiom (`fn_8008A220` passes a
 * `void*` to a `u32*` parameter) and this re-cut must not rewrite bodies, so the extension stays `.c`.
 * `#pragma peephole off` / `#pragma fp_contract off` are file-scoped.  Registered `Object(NonMatching,
 * ...)` in lib g3d.
 */


#include "types.h"

#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic(const char*, int, const char*, ...) */
extern void Panic__Q24nw4r2dbFPCciPCce(const char *file, int line, const char *msg, ...);


/* Forward declarations for the unit's own functions. */
void *fn_8008A220(void *self, u32 value);
void fn_8008A284(u32 *self, u32 value);

/* Stores a pointer and asserts its 4-byte alignment. */
void *fn_8008A220(void *self, u32 value)
{
    fn_8008A284(self, value);
    if (value & 3) {
        Panic__Q24nw4r2dbFPCciPCce("g3d_resuser_ac.h", 87,
            "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* Stores a word. */
void fn_8008A284(u32 *self, u32 value)
{
    *self = value;
}
