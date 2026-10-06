/*
 * ef/ef_drawstripestrategy.cpp - nw4r::ef DrawStripeStrategy and its particle/list helpers: the
 *   `Particle`/`ParticleManager` walkers, the draw-time particle copies and ahead-vector builders the billboard
 *   dispatch hands out, and the class's deleting destructor `fn_800B99E8`, which closes the range.
 * RANGE. .text 0x800B4AC8-0x800B9A44 (60 functions); extab 0x8000A10C-0x8000A23C, extabindex 0x80023538-0x80023700,
 *   .data 0x805939E8-0x80593E88 (the `__FILE__` string "ef_drawstripestrategy.cpp" first, copies at 0x80593D0C,
 *   0x80593D5C and 0x80593DAC), .sdata2 0x80796120-0x80796150.  Left edge: `ef/fn_800AEE48.cpp` (`ef_resource.cpp`)
 *   ends there; right edge: `ef/ef_drawbillboardstrategy.cpp` starts there.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off`.
 * NAMES. The map has only `fn_` stems for the range.
 * RESIDUALS. 23 rows unwritten (declared, never defined): 0x800B4BA4-0x800B4FE0, 0x800B4FE4-0x800B51F8,
 *   0x800B52D0-0x800B59E4, 0x800B5B54-0x800B612C, 0x800B6144-0x800B6534, 0x800B6548-0x800B68F8,
 *   0x800B69A0-0x800B7628, 0x800B76C4-0x800B7F58, 0x800B7FCC-0x800B83C0, 0x800B83CC-0x800B8788,
 *   0x800B8888-0x800B8D48, 0x800B8DC0-0x800B95C0, 0x800B9630-0x800B99E8.
 *   3 partial rows:
 *  - `fn_800B51F8`: the prologue saves r5 (`mr r31, r5`) two slots early;
 *  - `fn_800B6900`: the record copy interleaves its word loads and stores differently;
 *  - `fn_800B7F58`: two saved values swap r30/r31.
 *   flipcheck: `.data` and `.sdata2` claimed, not emitted (declared by their map names, playbook 29); `.text` 0x75C
 *   of 0x4F7C; extab 0x80 of 0x130; extabindex 0xC0 of 0x1C8.
 * SHAPES. `EfStripeParam` (nine words and a scalar, 0x28) stays apart from the smooth-stripe unit's `EfVec3x3`:
 *   retail copies its first nine words as words and only +0x24 as a float.
 */

#include "types.h"
#include "gx.h"
#include "ef.h"
#include "ef/ef_drawstrategyimpl.h"
#include "ef/fn_800AEE48.h"
#include "ef/ef_drawstripestrategy.h" /* the ahead-vector builders and EfAheadItem (this unit's own header) */
#include "sys_mem.h"
#include "unsplit/ef.h"
#include "g3d/fn_80063888.h" /* fn_80067E54, owned by g3d/fn_80063888.cpp (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

#ifdef __cplusplus
namespace nw4r {
namespace math {
f32 FrSqrt(f32 value);
}  // namespace math
}  // namespace nw4r
extern "C" {
#endif

#pragma peephole off

extern char lbl_80593CB4[];  /* the ef_resource vtable                                     .data 0x80593CB4 */
extern char lbl_80694C08[];  /* a draw-strategy singleton                                  .bss  0x80694C08 */
extern char lbl_80694C20[];  /* a draw-strategy singleton                                  .bss  0x80694C20 */
extern f32 lbl_80796140;    /* a stripe-strategy constant                                 .sdata2 0x80796140 */
extern f32 lbl_80796124; /* a scale factor  .sdata2 0x80796124 */
extern f32 lbl_80796130; /* a scale factor  .sdata2 0x80796130 */

/* Helpers owned by other units. */
extern void  PSVECSubtract(Vec* dst, const Vec* a, const Vec* b);
extern f32 fn_80052214(void* self, void* other);
extern void fn_800513F0(void* self);
extern int fn_800A5248(void* node);
extern void  GXSetCullMode(u32 mode);
extern void  GXBegin(u32 primitive, u32 vtxfmt, u32 count);
extern void* fn_800508A8(void* arg);
extern void* fn_800508AC(void* arg);
extern void* fn_80051570(void* arg);
extern void  PSMTXMultVec(const f32* mtx, const void* src, void* dst);
extern void  fn_800A7F00(void* arg, Vec* v);

/* Nine words plus a trailing scalar: the sampler's per-step record.  (The target copies the nine words
 * as words and only +0x24 as a float, so they are not three `Vec`s.) */
typedef struct EfStripeParam {
    u32 words_0x00[9]; /* +0x00 */
    f32 field_0x24;    /* +0x24 */
} EfStripeParam; /* size: 0x28 */

/* Three positions the sample zeroer walks; `fn_800B6954` zeroes them one `Vec` at a time. */
typedef struct EfStripeSample {
    Vec a; /* +0x00 */
    Vec b; /* +0x0C */
    Vec c; /* +0x18 */
} EfStripeSample; /* size: 0x24 */

/* The per-instance byte offset a node manager carries. */
typedef struct EfNodeManager {
    u8 pad_0x00[0x42]; /* +0x00 */
    u16 field_0x42;    /* +0x42  byte offset from a node base to its link field */
} EfNodeManager; /* size: 0x44 */

/* The unit's own symbols that are used before their definition. */
void* fn_800B4B60(void* self, s16 flag);
void fn_800B4FE0(void);
void fn_800B5288(f32 x, f32 y);
int  fn_800B5298(u32 value);
void fn_800B52AC(Vec* v);
void fn_800B52BC(f32 x, f32 y, f32 z);
u32 fn_800B5B40(EfParticleState* self);
void* fn_800B5B34(void* self, void* node);
void* fn_800B8DB0(void* self, void* node);
u32 fn_800B9628(EfParticleState* self);

/* Constructs the resource record and installs its vtable. */
void* fn_800B4AC8(void* self) {
    fn_800C5F74((EfParticleLayers*)self);
    *(void**)self = lbl_80593CB4;
    return self;
}

/* The two-arg destructor: always detaches, then frees when the signed flag is positive. */
void* fn_800B4B04(void* self, s16 flag) {
    if (self != 0) {
        fn_800B4B60(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Frees `self` when the signed flag is positive. */
void* fn_800B4B60(void* self, s16 flag) {
    if (self != 0 && flag > 0) {
        operator delete(self);
    }
    return self;
}

/* Nothing to do. */
void fn_800B4FE0(void) {}

/* Emits two stripe endpoints, each preceded by its scale when the flag's low bit is set. */
void fn_800B51F8(Vec* a, Vec* b, u32 flags, f32 scale) {
    fn_800B52AC(a);
    if (fn_800B5298(flags)) {
        fn_800B5288(lbl_80796124, scale);
    }
    fn_800B52AC(b);
    if (fn_800B5298(flags)) {
        fn_800B5288(lbl_80796130, scale);
    }
}

/* Writes a pair of f32 to the pipe. */
void fn_800B5288(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Tests the low bit of a status word (the booleanised `(value & 1) != 0`). */
int fn_800B5298(u32 value) {
    return (value & 1) != 0;
}

void fn_800B52AC(Vec* v) {
    fn_800B52BC(v->x, v->y, v->z);
}

/* Writes three f32 to the pipe. */
void fn_800B52BC(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* Resolves the stripe length, returning 0 when it is below the unit's threshold. */
int fn_800B59E4(void* self) {
    f32 length = fn_80052214(self, self);
    if (length < fn_800B5A48()) {
        return 0;
    }
    nw4r::math::FrSqrt(length);
    fn_800513F0(self);
    return 1;
}

/* A stripe-strategy constant. */
f32 fn_800B5A48(void) {
    return lbl_80796140;
}

/* Tests one bit of a status word. */
int fn_800B5A50(u32 value) {
    return (value & 0x8) != 0;
}

/* Walks the particle list at +0x3C until the callback reports 1 (or the end). */
void* fn_800B5A64(EfDrawList* self) {
    void* node = (void*)fn_800B5B40((EfParticleState*)self);
    while (node != 0 && fn_800A5248(node) != 1) {
        node = fn_800B5ACC(self, node);
    }
    return node;
}

/* Walks the auxiliary list through the per-node offset table until the callback reports 1. */
void* fn_800B5ACC(void* self, void* node) {
    void* next = fn_800B5B34(self, node);
    while (next != 0 && fn_800A5248(next) != 1) {
        next = fn_800B5B34(self, next);
    }
    return next;
}

/* The next list node: the manager's per-instance byte offset into the node.  The offset is a runtime
 * field, so there is no compile-time field name to reach; the byte add is the only spelling. */
void* fn_800B5B34(void* self, void* node) {
    return *(void**)((u8*)node + ((EfNodeManager*)self)->field_0x42);
}

/* The particle's stored word at +0x3C. */
u32 fn_800B5B40(EfParticleState* self) {
    return self->field_0x3C;
}

/* The particle's packed flags, bits 6-7. */
u32 fn_800B5B48(void* ctx, EfParticleState* particle) {
    return particle->field_0xB2 & 0xC0;
}

/* A draw-strategy singleton. */
void* fn_800B612C(void) {
    return lbl_80694C20;
}

/* A draw-strategy singleton. */
void* fn_800B6138(void) {
    return lbl_80694C08;
}

/* Tests one bit of a status word. */
int fn_800B6534(u32 value) {
    return (value & 0x10) != 0;
}

/* The particle's layer/parameter index byte. */
u8 fn_800B68F8(void* ctx, EfParticleState* particle) {
    return particle->field_0xB0;
}

/* Copies a stripe sample. */
void fn_800B6900(EfStripeParam* dst, EfStripeParam* src) {
    *dst = *src;
}

/* Zeroes the three positions of a stripe sample and returns it. */
EfStripeSample* fn_800B6954(EfStripeSample* self) {
    VEC3_ctor((VEC3*)&self->a);
    VEC3_ctor((VEC3*)&self->b);
    VEC3_ctor((VEC3*)&self->c);
    return self;
}

/* The particle's state bit 0x800. */
u32 fn_800B6994(void* ctx, EfParticleState* particle) {
    return particle->flags_0x00 & 0x800;
}

/* Draws eight stripe steps at one scale when the flag is set. */
void fn_800B7628(u32 cull_mode, u32 enabled, u32 flags) {
    GXSetCullMode(cull_mode);
    if (enabled != 0) {
        GXBegin(0x98, 0, 8);
        for (int i = 0; i < 8; i++) {
            fn_800B52BC(lbl_80796130, lbl_80796130, lbl_80796130);
            if (fn_800B5298(flags)) {
                fn_800B5288(lbl_80796130, lbl_80796130);
            }
        }
        fn_800B4FE0();
    }
}

/* The particle's packed flags, bits 0-2. */
u32 fn_800B76B8(void* ctx, EfParticleState* particle) {
    return particle->field_0xB2 & 0x7;
}

/* Transforms a vector by the matrix the helper pair builds, and returns `dst`. */
void* fn_800B7F58(void* dst, void* a, void* b) {
    void* x = fn_800508A8(dst);
    void* y = fn_800508AC(b);
    PSMTXMultVec((const f32*)fn_80051570(a), y, x);
    return dst;
}

/* The particle's packed flags, bits 3-5. */
u32 fn_800B83C0(void* ctx, EfParticleState* particle) {
    return particle->field_0xB2 & 0x38;
}

/* Builds the particle's +0xB0 transform into a local and copies it into `dst`. */
void fn_800B8788(nw4r::math::VEC3* dst, EfParticleState* particle) {
    u8 tmp[0x10];
    copyVec3(dst, (const nw4r::math::VEC3*)fn_80067E54(tmp, &particle->field_0xB0));
}

/* Copies the particle's +0x98 block into `dst`. */
void fn_800B87C8(nw4r::math::VEC3* dst, EfParticleState* particle) {
    copyVec3(dst, (const nw4r::math::VEC3*)&particle->field_0x98);
}

/* Subtracts the two ahead vectors and copies the reference block when the result is degenerate. */
void fn_800B87D0(Vec* a, EfParticleState* particle, EfAheadItem* item) {
    PSVECSubtract(a, &item->field_0xAC, &particle->field_0xA4);
    if (fn_800B59E4(a) == 0) {
        copyVec3((nw4r::math::VEC3*)a, (const nw4r::math::VEC3*)&particle->field_0x98);
    }
}

/* Builds the ahead vector and copies the reference block when the result is degenerate. */
/* untyped: opaque handle passed through to fn_800A7F00 */
void fn_800B882C(Vec* a, EfParticleState* particle, void* arg) {
    fn_800A7F00(arg, a);
    if (fn_800B59E4(a) == 0) {
        copyVec3((nw4r::math::VEC3*)a, (const nw4r::math::VEC3*)&particle->field_0x98);
    }
}

/* Walks a node chain through fn_800B8DB0 until the callback reports 1 (or the end). */
void* fn_800B8D48(void* self, void* node) {
    void* next = fn_800B8DB0(self, node);
    while (next != 0 && fn_800A5248(next) != 1) {
        next = fn_800B8DB0(self, next);
    }
    return next;
}

void* fn_800B8DB0(void* self, void* node) {
    return *(void**)((u8*)node + ((EfNodeManager*)self)->field_0x42 + 4);
}

/* Walks the +0x38 head through fn_800B8DB0 until the callback reports 1 (or the end). */
void* fn_800B95C0(EfParticleState* self) {
    void* next = (void*)fn_800B9628(self);
    while (next != 0 && fn_800A5248(next) != 1) {
        next = fn_800B8DB0(self, next);
    }
    return next;
}

/* The particle's stored word at +0x38. */
u32 fn_800B9628(EfParticleState* self) {
    return self->field_0x38;
}

/* Deletes the DrawStripeStrategy and, when the flag is positive, frees the storage. */
void* fn_800B99E8(void* self, s16 flag) {
    if (self != 0) {
        fn_800B4B04(self, 0);
        if ((s16)flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

#ifdef __cplusplus
}
#endif
