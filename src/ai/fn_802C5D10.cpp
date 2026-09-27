/*
 * ai/fn_802C5D10.cpp - the AI-NPC sub-state machines' first block: the per-motion step dispatchers
 * that drive an `_AINPC_W`'s scripted motion.
 *
 * `.text` 0x802C5D10-0x802CC794 (76 functions, 27268 B).  Registered from
 * `proposal/802C5D10_fn_802C5D10.cpp`.
 *
 * Module `ai`, and the file keeps the map's stem (brief section 2, class 4): the record every
 * function takes in r3 is the `_AINPC_W` the neighbouring `ai/fn_802CC794.cpp` band reconstructs -
 * this band calls `ai_skill_ck__FP8_AINPC_WUc` and stores through the same `+0x170`/`+0x442`/`+0x444`
 * offsets that band's own writers use - but neither a `__FILE__` string nor a runtime-dump name covers
 * the range (`tools/symbols/dumpmap.py lookup` answers `zz_XXXXXXXX_` for all 76 symbols and
 * `tools/symbols/symedit.py range 0x802C5D10 0x802CC794` returns 76 bare `fn_XXXXXXXX` entries), so
 * every symbol in the inventory is still a map placeholder.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x802C5D10 0x802CC794` - 76 entries, all bare `.text`
 * `fn_XXXXXXXX` - and `python tools/symbols/dumpmap.py lookup` over that inventory, which answers only
 * `zz_XXXXXXXX_` placeholders).  Every definition below is `extern "C"` so objdiff pairs it by the
 * map's name (playbook 42); a C++ definition would mangle and measure 0 %.
 *
 * Seam - unproven.  The range is one maximal unclaimed run of `attribute.py`, and the discovery note's
 * `menu_item.cpp` seam is not this band's: that `__FILE__` string sits at `.data` 0x805CDFC8 and the
 * dump's own local symbol attributes it to the function at 0x802A22A4, in the registered `menu`
 * proposal - the same finding `light/light.cpp` recorded.  `.sdata2` is no help either way: the pool
 * run 0x8079A670-0x8079A778 is shared with the neighbouring bands (0.0, 50.0 and 30.0 are loaded by
 * both this range and 0x802C474C-0x802C5D10), so it constrains nothing.
 *
 * Sections: `.text` 0x802C5D10-0x802CC794, extab 0x80014484-0x8001467C (63 x 8-byte records, exactly
 * the run the extabindex entries for 0x802C5D10-0x802CC57C point at) and extabindex
 * 0x8003252C-0x80032820 (63 x 12 B).  No `.data` is claimed: the range's switch tables live at
 * 0x805D4964-0x805D4D38, but the run's ends are the neighbouring unclaimed bands' (the previous
 * proposal's data ends at 0x805D4964, `ai/fn_802CC794.cpp`'s begins at 0x805D4D38) and a data claim
 * has to be measured before and after; `.sdata2` is left to the pool (playbook 29: the constants are
 * `extern`, never defined).
 *
 * Flags: nothing per-unit - `cflags_main` (`Wii/1.3`, `-O3 -inline noauto -Cpp_exceptions on`), the
 * bracketing units' set - except that the file needs the peephole pass off (`#pragma peephole off`
 * below).  Evidence: retail keeps `clrlwi rN, rN, 24` + a separate compare in every narrowed-flag
 * test (fn_802C5D10's three, fn_802C6578's one) where `-O3`'s peephole emits one `clrlwi.` record
 * form; turning it off took fn_802C5D10 94.23 -> 95.99 and fn_802C6578 93.86 -> 97.71, the same
 * finding the sibling bands `light/light.cpp`, `camera/fn_802B5C58.cpp` and `stage/fn_802B2978.c`
 * record.  Under it the state increments must be written `field += 1` (not `field = field + 1`):
 * the latter leaves a `clrlwi r0, r0, 24` before the byte store that retail does not have (playbook
 * 38's narrow-store rule), worth exactly one instruction in five of the seven functions written.
 *
 * The flag parameters are spelled `u32` and tested `(flag & 0xFF)`: retail materialises the byte
 * with `clrlwi rN, rN, 24` at every test, which a `u8` parameter does not produce (`cmpwi r31, 0`).
 *
 * Residuals (12 of the range's 76 functions written, 6152 B of 27268; 3 byte-identical, 9 of the 12
 * at or above 97 %):
 *
 *  - fn_802C5ECC 91.41 %, fn_802C6110 96.92 %, fn_802C6318 93.55 %: retail's `setVec3` call
 *    reuses the callee's return pointer (`mr r4, r3`) where ours rematerialises the local's address
 *    (`addi r4, r1, N`) - the declaration `pl.h` reaches (`include/ef.h`, through which `_PLW`
 *    arrives) spells that helper `void`, while the target's own code proves it returns its `out`
 *    pointer (`include/enemy/fn_80165FC8.h` has the `VEC3*` spelling, but it clashes with `ef.h`'s
 *    `Vec*` one so the two cannot both be included).  fn_802C6318 also keeps a signed compare chain
 *    where retail tests three unsigned ranges, and its flag masks are CSE'd into each other.
 *  - fn_802C5D10 99.05 %: retail has one extra `clrlwi r0, r31, 24` at its third flag test, which
 *    MWCC CSEs into the second test's r0 (the branch reaches it directly, so the value is live);
 *    `(u8)flag`, a `u8` parameter and two separate `if`s were all measured and all keep the CSE.
 *  - fn_802C6BE0 98.15 %, fn_802C703C 98.71 %, fn_802C722C 99.09 %: each has one instruction the
 *    target carries and ours folds - the `clrlwi r5, r5, 16` retail masks a `u16` argument with from
 *    a constant ternary (measured: a `u16` parameter, a `u16` local and an explicit cast all fold
 *    it) - or one case-test opcode (`cmpwi` where ours picks `cmplwi`).  fn_802C6578 99.14 % is the
 *    same opcode residual.
 *  - fn_802C6E3C 97.30 %: the `clrlwi r5, r5, 16` residual plus a register-colouring swap (retail
 *    puts the flag parameter in r30 and `armed` in r31, ours the other way), which is its whole
 *    instruction diff - the operands and the frame are equal.
 *
 * Not yet written: the remaining 64 functions of the range, all still `fn_XXXXXXXX` in the map.  The
 * next one in address order is `fn_802C6908` (0x2D8, a 94-way jump-table dispatcher whose table is
 * `.data` 0x805D4964 and whose arms tail-call into the neighbouring bands - mechanical but long),
 * then `fn_802C75F4` (0x290) and `fn_802C7884` (0x240).
 *
 * The `_AINPC_W` layout is the union of both bands' offsets, traced: every offset this file reads was
 * checked against the target's own load/store displacement, and the `+0x1D8` gap that separated them
 * by 4 bytes was found that way (the record's `+0x1DC`/`+0x482`/`+0x492` fields sit at their
 * annotated offsets now, confirmed with a compile probe).  `ai/ainpc.h` is the one definition;
 * `src/ai/fn_802CC794.cpp` still carries its own copy of the first 0x492 bytes and should include
 * this header instead (see the outbox's shared-file request).
 *
 * Inventory and evidence: the unit's symbols are in `config/RMHE08/symbols.txt`, its target object at
 * `build/RMHE08/obj/ai/fn_802C5D10.o`, and its per-symbol scores in `build/RMHE08/report.json`.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"

#include "pl.h"
#include "fn_8004CAD8.h"
#include "Pl/pl_act.h"
#include "Pl/fn_8028F66C.h"
#include "g3d/g3d_calcworld.h"
#include "ef/fn_800CDB2C.h"

#include "ai/ainpc.h"
#include "unsplit/ai.h"

/* Retail keeps the unfused form of the byte-narrowing folds this band is full of (`clrlwi` + `cmpwi`
 * as two instructions where `-O3`'s peephole makes one `clrlwi.`), the same finding the sibling
 * bands `light/light.cpp`, `camera/fn_802B5C58.cpp` and `stage/fn_802B2978.c` record (playbook 39). */
#pragma peephole off

#ifdef __cplusplus
extern "C" {
#endif

/* The unowned helpers this band dispatches into: 0x802D0F34 onwards is one more unclaimed run, so
 * these are rule 2's counted gap (`tools/units/stylelint.py`, a named unsplit band).  The signatures
 * are the callees' own bodies: each returns in r3 and reads the registers the call sites set. */
void fn_802D3A2C(struct _AINPC_W* self, u32 a);
void fn_802D3CCC(struct _AINPC_W* self, u32 a);
void fn_802D3CD4(struct _AINPC_W* self, u32 a);
void fn_802D3CFC(struct _AINPC_W* self, u32 a);
void fn_802D29B8(struct _AINPC_W* self, u32 a);
void fn_802D29F8(struct _AINPC_W* self, u32 a);
void fn_802D2904(struct _AINPC_W* self, u32 a, u32 b, u32 c);
u32 fn_802D2984(struct _AINPC_W* self);
u32 fn_802D2990(struct _AINPC_W* self, u32 a, f32 b, f32 c);
void fn_802D2ABC(struct _AINPC_W* self, u32 a, u32 b, u32 c);
void fn_802D2B88(struct _AINPC_W* self, u32 a);
void fn_802D2B98(struct _AINPC_W* self, u32 a);
s32 fn_802D2B78(struct _AINPC_W* self, u32 a);
void fn_802D2AD4(struct _AINPC_W* self, u32 a);
void fn_802D40EC(struct _AINPC_W* self, u32 a, s32 b);
void fn_802D35B4(struct _AINPC_W* self, s32 a);
void fn_802D3684(struct _AINPC_W* self);
void fn_802D6690(struct _AINPC_W* self);
s16 fn_802D66B8(struct _AINPC_W* self);
void fn_802D84D0(struct _AINPC_W* self, u32 a);
void fn_802D9400(struct _AINPC_W* self);
void fn_802D9A44(struct _AINPC_W* self);
void fn_802D9D30(struct _AINPC_W* self, s32 a, u32 b, u32 c, u32 d);
void fn_802D2910(struct _AINPC_W* self, u32 a, u32 b, u32 c);
void fn_802D29CC(struct _AINPC_W* self, f32 a);
u32 fn_802D3B34(struct _AINPC_W* self, s32 a, s32 b);
u16 fn_802D30F8(u16 a, u16 b, u16 c);
void fn_802D31FC(struct _AINPC_W* self);
void fn_802D3210(struct _AINPC_W* self, u32* out);

/* `pl.h` pulls `ef.h` in, whose `setVec3`/`VEC3_ctor` spell the 3-float record `Vec` - the same
 * layout this file's locals use, so only the `setVec3` calls need the spelling. */

/* Callees whose owner is a registered unit but whose header does not declare them yet (the same gap
 * `camera/fn_802B5C58.cpp` recorded): each spelling is that owner's own declaration. */
void fn_8010072C(struct _PLW* owner, u8 type, nw4r::math::VEC3* pos, u32 param, f32 scale);
u32 fn_8027DCE0(struct _PLW* self, u8 arg1);
s32 fn_80291B08(struct _PLW* self, nw4r::math::VEC3* pos, LandData* land, f32* out, u32 kind);
void fn_80114CC8(void* actor, u8 key);

#ifdef __cplusplus
}
#endif

/* The per-motion step dispatcher: it arms the motion and its scale once, then steps the armed
 * sub-state until the motion ends. */
extern "C" void fn_802C5D10(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 vec;

    VEC3_ctor(&vec);
    fn_802D3A2C(self, 2);
    fn_802D29B8(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        fn_802D29F8(self, 0);
        if ((flag & 0xFF) == 0) {
            fn_802D2904(self, 14, 4, 146);
        } else {
            fn_802D2904(self, 14, 4, 186);
        }
        self->field_0x442 = 0;
        self->field_0x444 = 0;
        break;
    case 1:
        fn_802D3CD4(self, 1);
        if (fn_802D2984(self) == 1) {
            if ((flag & 0xFF) == 0) {
                fn_802D2ABC(self, 1, 83, 0);
            } else {
                fn_802D2ABC(self, 1, 85, 0);
            }
            break;
        }
        if ((flag & 0xFF) == 0) {
            if (fn_802D2990(self, 1, lbl_8079A6BC, lbl_8079A670) == 1) {
                fn_802D2B88(self, 2);
                f32 y = self->vec_0x178.y;
                f32 height = lbl_8079A6C0 + self->field_0x19C;

                if (y < height) {
                    self->vec_0x178.y = height;
                    fn_802D2ABC(self, 1, 83, 0);
                }
            }
        } else if ((flag & 0xFF) == 1) {
            if (fn_802D2990(self, 1, lbl_8079A6BC, lbl_8079A670) == 1) {
                fn_802D2B88(self, 2);
            }
        }
        break;
    }
}

/* The four-step variant: it arms a 248-frame motion and a 20-frame ramp, then re-aims the motion
 * through the `+0x005` frame table until the ramp is exhausted. */
extern "C" void fn_802C5ECC(struct _AINPC_W* self)
{
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 offset;

    VEC3_ctor(&vec);
    fn_802D3A2C(self, 2);
    fn_802D29B8(self, 2);
    fn_802D3CD4(self, 1);
    switch (self->state) {
    case 0:
        self->state += 1;
        fn_802D2B88(self, 2);
        fn_802D29F8(self, 0);
        fn_802D2904(self, 14, 0, 248);
        fn_802D3CD4(self, 1);
        self->vec_0x024.x = lbl_8079A678;
        self->vec_0x024.y = lbl_8079A678;
        self->vec_0x024.z = lbl_8079A678;
        self->field_0x1AC = 20;
        break;
    case 1: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames > 0) {
            break;
        }
        self->state += 1;
        fn_802D2B98(self, 2);
        self->field_0x002 = 0;
        setVec3((Vec*)&offset, lbl_8079A670, lbl_8079A670, lbl_8079A6C4);
        copyVec3(&vec, &offset);
        rotVecY(&vec, self->field_0x194);
        fn_80073F68(&vec, &self->vec_0x178);
        vec.y = self->field_0x19C;
        fn_8010072C((struct _PLW*)self, 42, &vec, 0, lbl_8079A678);
        self->field_0x1AC = lbl_80792598[fn_802D66B8(self)];
        self->sub_step = 0;
        break;
    }
    case 2: {
        s16 step = self->field_0x442 + 1;

        self->field_0x442 = step;
        if (step > lbl_80792598[self->sub_step]) {
            u32 slot = self->field_0x444 + 1;

            self->field_0x444 = slot;
            if (slot == 3) {
                self->field_0x444 = 4;
            } else if (slot > 4) {
                self->field_0x444 = 4;
            }
            self->sub_step += 1;
        }
        if (self->field_0x442 > (s16)self->field_0x1AC) {
            self->field_0x1AC = 20;
            self->state += 1;
        }
        break;
    }
    case 3: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames > 0) {
            break;
        }
        fn_802D2ABC(self, 1, 87, 0);
        break;
    }
    }
}

/* The three-step variant: it copies the player's position into `vec_0x178` and arms a 14-frame
 * ramp, then re-aims the motion once the ramp hits its second-to-last frame. */
extern "C" void fn_802C6110(struct _AINPC_W* self)
{
    nw4r::math::VEC3 vec;
    LandData land;
    nw4r::math::VEC3 offset;
    f32 height;

    VEC3_ctor(&vec);
    fn_8012A624(&land);
    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        fn_802D29F8(self, 0);
        fn_802D2904(self, 54, 0, 0);
        copyVec3(&self->vec_0x178, &self->plw_0x16C->vec_0x03C);
        self->field_0x194 = self->plw_0x16C->field_0x058;
        self->field_0x1AC = 14;
        fn_802D2B88(self, 2);
        self->vec_0x024.x = lbl_8079A678;
        self->vec_0x024.y = lbl_8079A678;
        self->vec_0x024.z = lbl_8079A678;
        fn_802D3CCC(self, 1);
        break;
    case 1: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames <= 0) {
            self->state += 1;
            if (fn_8027DCE0(self->plw_0x16C, 0) == 1) {
                fn_802D2B98(self, 2);
            }
            self->field_0x002 = 0;
        } else if (frames == 2) {
            setVec3((Vec*)&offset, lbl_8079A670, lbl_8079A670, lbl_8079A6CC);
            copyVec3(&vec, &offset);
            rotVecY(&vec, self->field_0x194);
            fn_80073F68(&vec, &self->vec_0x178);
            if (fn_8027DCE0(self->plw_0x16C, 0) == 1) {
                fn_8010072C((struct _PLW*)self, 42, &vec, 0, lbl_8079A678);
            }
        }
        break;
    }
    case 2:
        copyVec3(&self->vec_0x178, &self->plw_0x16C->vec_0x03C);
        self->field_0x194 = self->plw_0x16C->field_0x058;
        if (fn_8027DCE0(self->plw_0x16C, 0) == 0) {
            if (fn_80291B08((struct _PLW*)self, &self->vec_0x178, &land, &height, -5) == 1) {
                self->vec_0x178.y = height;
            }
            fn_802D2ABC(self, 1, 86, self->field_0x1EC);
        }
        break;
    }
}

/* The two-step variant: it arms a 30-frame wait and a 0.75 scale, then fires the motion in the
 * direction the player's position implies. */
extern "C" void fn_802C6318(struct _AINPC_W* self, u8 flag)
{
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 front;
    nw4r::math::VEC3 back;

    VEC3_ctor(&vec);
    fn_802D3A2C(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 30;
        self->field_0x002 = 1;
        self->vec_0x024.x = lbl_8079A674;
        self->vec_0x024.y = lbl_8079A674;
        self->vec_0x024.z = lbl_8079A674;
        if ((flag & 0xFF) == 0) {
            setVec3((Vec*)&front, lbl_8079A670, lbl_8079A670, lbl_8079A6CC);
            copyVec3(&vec, &front);
        } else {
            self->vec_0x178.y = self->field_0x19C;
            setVec3((Vec*)&back, lbl_8079A670, lbl_8079A670, lbl_8079A6C4);
            copyVec3(&vec, &back);
        }
        if (fn_802D2B78(self, 2) != 0) {
            break;
        }
        fn_802D2B88(self, 2);
        rotVecY(&vec, self->field_0x194);
        fn_80073F68(&vec, &self->vec_0x178);
        fn_8010072C((struct _PLW*)self, 42, &vec, 0, lbl_8079A678);
        break;
    case 1: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames > 0) {
            break;
        }
        self->state += 1;
        fn_802D2904(self, 16, 0, 0);
        fn_802D2B98(self, 2);
        break;
    }
    case 2:
        if (fn_802D2984(self) != 1) {
            break;
        }
        if ((flag & 0xFF) == 0) {
            fn_802D2AD4(self, 0);
        } else {
            fn_802D6690(self);
            fn_802D40EC(self, 29, -1);
            switch ((u32)self->field_0x444) {
            case 0:
            case 1:
                fn_802D2ABC(self, 1, 23, 0);
                fn_802D40EC(self, 27, 1);
                break;
            case 2:
            case 3:
                fn_802D40EC(self, 26, 1);
                fn_802D2ABC(self, 1, 23, 0);
                break;
            case 4:
                fn_802D40EC(self, 28, 1);
                fn_802D2ABC(self, 1, 24, 0);
                break;
            }
        }
        self->field_0x440 = 0;
        break;
    }
}

/* The two-state projectile-launch dispatcher: it arms a 6-frame shot or a 57-frame hold depending on
 * the tier the record carries. */
extern "C" void fn_802C6578(struct _AINPC_W* self, u8 flag)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        if ((flag & 0xFF) == 0) {
            fn_802D29F8(self, 0);
            fn_802D2904(self, 6, 4, 0);
        } else {
            fn_802D29F8(self, 2);
            fn_802D2904(self, 57, 4, 0);
        }
        self->field_0x1AC = 600;
        if (self->field_0x420 - 3 <= 1) {
            break;
        }
        fn_802D9D30(self, (s8)self->field_0x482, 20, self->field_0x462, 2);
        break;
    case 1: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames > 0 && self->field_0x461 != 0) {
            break;
        }
        if ((flag & 0xFF) == 0) {
            fn_802D2AD4(self, 0);
        } else {
            fn_802D2AD4(self, 2);
        }
        self->field_0x461 = 0;
        break;
    }
    }
}

/* The four-state charge dispatcher: it arms a 300-frame charge, releases it into the bite/stomp
 * motion the `+0x484` tier selects, then waits for the motion to end. */
extern "C" void fn_802C6690(struct _AINPC_W* self)
{
    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        fn_802D29F8(self, 0);
        fn_802D2904(self, 14, 4, 0);
        break;
    case 1:
        if (fn_802D2984(self) == 1) {
            self->field_0x1AC = 300;
            fn_802D2B88(self, 2);
            self->state += 1;
        }
        break;
    case 2: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames >= 0) {
            break;
        }
        fn_802D2B98(self, 2);
        fn_802D2904(self, 16, 0, 0);
        fn_802D9400(self);
        self->field_0x485 = 0;
        fn_802D3684(self);
        if (self->field_0x484 < 3) {
            fn_802D9D30(self, 9, 1, 0, 2);
            self->field_0x486 = 900;
        } else {
            fn_802D9D30(self, 9, 2, 0, 2);
        }
        fn_802D9A44(self);
        self->state += 1;
        break;
    }
    case 3:
        if (fn_802D2984(self) == 1) {
            fn_802D2AD4(self, 0);
        }
        break;
    }
}

/* The two-state tail-swipe dispatcher: it arms a 6-frame wind-up, then fires the swipe through the
 * `+0x1F4` motion as the countdown crosses frame 4. */
extern "C" void fn_802C681C(struct _AINPC_W* self)
{
    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 6;
        fn_802D29F8(self, 0);
        fn_802D2904(self, 29, 6, 0);
        break;
    case 1: {
        s32 frames;

        if (fn_802D2984(self) != 1) {
            break;
        }
        frames = self->field_0x1AC - 1;
        self->field_0x1AC = frames;
        if (frames == 4) {
            fn_802D35B4(self, (s16)self->field_0x1F4);
            fn_802D3CFC(self, 1);
            fn_80114CC8(self, 0);
            break;
        }
        if (frames <= 0) {
            fn_802D2AD4(self, 0);
        }
        break;
    }
    }
}

/* The two-state approach dispatcher: it picks the near/far ring the flag names, arms a 450-frame
 * approach, and gives up when the target leaves the ring or the timer runs out. */
extern "C" void fn_802C6BE0(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;
    f32 near_limit;
    f32 far_limit;
    s32 armed;

    VEC3_ctor(&start);
    near_limit = lbl_8079A670;
    far_limit = near_limit;
    armed = 0;
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 450;
        switch (flag & 0xFF) {
        case 0:
        case 1:
            fn_802D2910(self, 8, 4, 0);
            break;
        case 2:
            fn_802D2910(self, 10, 2, 0);
            fn_802D29CC(self, lbl_8079A680);
            break;
        }
        fn_802D29F8(self, 0);
        break;
    case 1: {
        s32 limit;
        f32 distance;

        switch (flag & 0xFF) {
        case 0:
            near_limit = lbl_8079A6CC;
            far_limit = lbl_8079A6D0;
            break;
        case 1:
            near_limit = lbl_8079A6D4;
            far_limit = lbl_8079A6D8;
            break;
        case 2:
            near_limit = lbl_8079A68C;
            far_limit = lbl_8079A694;
            armed = 1;
            break;
        }
        limit = (flag & 0xFF) == 2 ? 10 : 90;
        distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);
        if (fn_802D3B34(self, (s32)limit, armed) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (distance <= near_limit * near_limit) {
            fn_802D2AD4(self, 0);
        } else if (distance >= far_limit * far_limit) {
            fn_802D2AD4(self, 0);
        } else if (--self->field_0x1AC < 0) {
            fn_802D2AD4(self, 0);
            break;
        }
        fn_80050CA0(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194, 2048);
        break;
    }
    }
}

/* The two-state guard dispatcher: it waits for the target to close inside the `+0x1BC` ring, then
 * aims the motion at it and reports the aim window through the `+0x194` angle selector. */
extern "C" void fn_802C6E3C(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;
    f32 limit;
    s32 armed;
    s32 limit2;

    VEC3_ctor(&start);
    limit = lbl_8079A670;
    armed = 0;
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 450;
        fn_802D29F8(self, 0);
        fn_802D2910(self, 11, 2, 0);
        break;
    case 1: {
        f32 distance;

        if ((flag & 0xFF) == 0) {
            limit = lbl_8079A6DC;
        }
        distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);
        if ((flag & 0xFF) == 2) {
            limit2 = 20;
            armed = 1;
        } else {
            limit2 = 90;
        }
        if (fn_802D3B34(self, limit2, armed) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (distance <= self->field_0x1BC * self->field_0x1BC) {
            fn_802D2AD4(self, 0);
            break;
        } else if (--self->field_0x1AC < 0) {
            fn_802D2AD4(self, 0);
            break;
        } else if ((flag & 0xFF) == 0) {
            if (distance >= limit * limit) {
                fn_802D2AD4(self, 0);
                break;
            }
        }
        fn_80050CA0(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194,
                                        distance > lbl_8079A6E0 ? 1536 : 2560);
        break;
    }
    }
}

/* The two-state guard dispatcher: it arms one of two motions on the flag, then steers the motion
 * while the target stays inside the 2048-unit aim window and the 90-frame guard lasts. */
extern "C" void fn_802C703C(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;

    VEC3_ctor(&start);
    fn_802D29B8(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        switch (flag & 0xFF) {
        case 0:
            fn_802D29F8(self, 0);
            fn_802D2910(self, 8, 4, 0);
            break;
        case 1:
            fn_802D29F8(self, 2);
            fn_802D2910(self, 12, 4, 0);
            fn_802D31FC(self);
            self->field_0x1DC = lbl_8079A6E4;
            break;
        }
        self->field_0x1AC = 90;
        break;
    case 1:
        fn_80050CA0(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        if ((flag & 0xFF) == 1) {
            f32 distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);

            self->field_0x194 =
                fn_802D30F8((u16)ang_b, (u16)self->field_0x194, distance > lbl_8079A6E0 ? 2048 : 6656);
            fn_802D3210(self, &self->field_0x190);
            self->field_0x190 = fn_802D30F8((u16)ang_a, (u16)self->field_0x190, 1536);
        } else {
            self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194, 2048);
        }
        if (self->field_0x194 - ang_b < 2048) {
            fn_802D2AD4(self, self->variant);
            break;
        }
        if (--self->field_0x1AC <= 0) {
            fn_802D2AD4(self, self->variant);
        }
        break;
    }
}

/* The two-state guard dispatcher: it arms a 1.3-scaled motion and a 450-frame guard, then gives up
 * as soon as the target closes inside the `+0x1BC` ring. */
extern "C" void fn_802C722C(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;
    s32 armed;
    s32 limit;

    VEC3_ctor(&start);
    armed = 0;
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 450;
        fn_802D29F8(self, 0);
        fn_802D2910(self, 11, 2, 0);
        fn_802D29CC(self, lbl_8079A6E8);
        break;
    case 1: {
        f32 distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);

        if ((flag & 0xFF) == 1) {
            limit = 20;
            armed = 1;
        } else {
            limit = 90;
        }
        if (fn_802D3B34(self, limit, armed) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (distance <= self->field_0x1BC * self->field_0x1BC) {
            fn_802D2AD4(self, 0);
            break;
        } else if (--self->field_0x1AC < 0) {
            fn_802D2AD4(self, 0);
            break;
        }
        fn_80050CA0(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194,
                                        distance > lbl_8079A6E0 ? 2048 : 4096);
        break;
    }
    }
}

/* The two-state dying/stagger dispatcher: it rolls one of two motions, then folds the aim every
 * frame until the target closes in or the 300-frame guard runs out. */
extern "C" void fn_802C73E4(struct _AINPC_W* self)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;

    VEC3_ctor(&start);
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 300;
        fn_802D29F8(self, 0);
        self->field_0x194 -= 16384;
        fn_802D2910(self, 200, 6, 0);
        fn_802D31FC(self);
        self->field_0x1D4 = lbl_8079A6EC;
        if (ran_suu(1) & 1) {
            fn_802D9D30(self, (s8)self->field_0x482, 6, 0, 2);
        } else {
            fn_802D9D30(self, (s8)self->field_0x482, 7, 0, 2);
        }
        break;
    case 1: {
        f32 distance;
        u16 selector;

        fn_802D3210(self, &self->field_0x190);
        distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);
        if (fn_802D3B34(self, 30, 1) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (distance <= self->field_0x1BC * self->field_0x1BC) {
            fn_802D2AD4(self, 0);
            break;
        } else if (--self->field_0x1AC < 0) {
            fn_802D2AD4(self, 0);
            break;
        }
        fn_80050CA0(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        ang_b -= 16384;
        if (distance > lbl_8079A6E0) {
            selector = 1024;
        } else if (distance > lbl_8079A6F0) {
            selector = 2048;
        } else {
            selector = 4096;
        }
        self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194, selector);
        break;
    }
    }
}
