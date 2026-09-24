/* ef/fn_800AEE48.cpp - nw4r::ef post-field / resource / stripe-strategy layer, `.text` 0x800AEE48..0x800B99E8.
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * What it is.  119 functions / 0xABA0 bytes of the NintendoWare-for-Revolution effect library
 * (`nw4r::ef`).  The split object's own `__FILE__` strings name the layer's original source files, in
 * `.data` order (which is `.text` order): `ef_postfield.cpp` (0x80593580), `ef_resource.cpp`
 * (0x80593690) and `ef_drawstripestrategy.cpp` (0x805939E8), plus the header strings
 * `res_drawparam_ac.h`, `res_emitterparam_ac.h`, `particle.h`, `particlemanager.h` and `drawinfo.h`.
 * The discovery's `proposal/800AEE48_fn_800AEE48.cpp` range was capped at `--max-bytes`, so its seam
 * is a guess and the one proposal is really three TUs (docs/plan.md 12, "the seam is re-checked the
 * moment its functions match").  Provisional internal seams, from the panic-site file strings:
 *   0x800AEE48..0x800B2810   ef_postfield.cpp          (fn_800AF440 / fn_800B18F0 cite it)
 *   0x800B2810..0x800B4BA4   ef_resource.cpp           (fn_800B28FC .. fn_800B4898 cite it)
 *   0x800B4BA4..0x800B99E8   the head of ef_drawstripestrategy.cpp (fn_800B4BA4 .. fn_800B9630 cite
 *                            it); the TU continues past this range - the landed
 *                            `ef/ef_drawstripestrategy.cpp` (0x800B99E8..0x800BE154) is its tail.
 *
 * Pending the re-cut, the whole range is registered once, under the map's `fn_` stem (evidence class 4:
 * the `__FILE__` evidence decides the module `ef` and the language C++, but three names mean no single
 * one names the unit - the brief forbids inventing one, and the seam spans the next proposal, whose
 * `ef/ef_drawstripestrategy.cpp` already owns that name).  The module comes from the `__FILE__`
 * strings and the `.cpp` extension from their suffix; `langcheck.py` confirms C++ (the
 * `Panic__Q24nw4r2dbFPCciPCce` callee and the `.cpp` names).
 *
 * Types.  Every record is reconstructed from the load/store offsets the matched bodies use and carries
 * its size annotation.  The layer's own `EfStripeParam` (nine words + a scalar, 0x28) and
 * `EfStripeSample` (three `Vec`s) are distinct even though the registered
 * `ef_drawsmoothstripestrategy.cpp` spells a 0x28 sampler record `EfVec3x3`, because the target copies
 * the first nine words as words and only +0x24 as a float.
 *
 * Data.  The unit owns no pool section here: its `.data`/`.sdata`/`.sdata2` labels are `extern` by
 * their map names and never defined (playbook 29); the data pass claims the ranges once the source
 * emits them (docs/plan.md 8.4).
 *
 * Status.  61 of the 119 symbols are reconstructed, every one at the 80 % bar or above (58 byte-
 * identical, `fn_800B6900` 80.5 %, `fn_800B51F8` 94.4 %, `fn_800B7F58` 99.0 %).  The unit as a whole is
 * 8.0 % (3196/43936 code bytes) because the 58 missing symbols are the layer's large players: the
 * `ef_postfield.cpp` body (`fn_800AEE48`.. `fn_800B23A4`, 0x800AEE48..0x800B2810), the `ef_resource.cpp`
 * loader (`fn_800B28FC`.. `fn_800B4898`) and the ef_drawstripestrategy `Particle`/`ParticleManager`
 * walkers (`fn_800B4BA4`.. `fn_800B9630`), none of which are reconstructed yet.  Residuals of the
 * three below 100 % are in `.pi/notes/800aee48-fn-800aee48-517d.md`.
 */

#include "types.h"
#include "gx.h"
#include "ef.h"
#include "ef/ef_drawstrategyimpl.h"
#include "ef/fn_800AEE48.h"
#include "sys_mem.h"
#include "unsplit/ef.h"

#ifdef __cplusplus
namespace nw4r {
namespace math {
f32 FrSqrt(f32 value);
}  // namespace math
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r
extern "C" {
#endif

#pragma peephole off

/* `nw4r::db::Panic` is declared by `ef.h` outside its `extern "C"` block, so it keeps the real C++
 * mangled spelling (`Panic__Q24nw4r2dbFPCciPCce`) without a hand-written mangling (rule 9). */

/* This unit's pooled `__FILE__`/assert strings and constants (the `.data` range
 * 0x80593580..0x80593E7C and the `.sdata2` range 0x807960E8..0x80796148).  Declared, never defined. */
extern char lbl_80593580[];  /* "ef_postfield.cpp"                                        .data 0x80593580 */
extern char lbl_80593598[];  /* "NW4R:Failed assertion 0"                                  .data 0x80593598 */
extern char lbl_805935E4[];  /* "res_drawparam_ac.h"                                       .data 0x805935E4 */
extern char lbl_8059362C[];  /* "res_emitterparam_ac.h"                                    .data 0x8059362C */
extern char lbl_80593678[];  /* "res_emitterparam_ac.h"                                    .data 0x80593678 */
extern char lbl_80593690[];  /* "ef_resource.cpp"                                          .data 0x80593690 */
extern char lbl_805936A0[];  /* "NW4R:Pointer Error\nproject(=%p) is not valid..."          .data 0x805936A0 */
extern char lbl_805939E8[];  /* "ef_drawstripestrategy.cpp"                                .data 0x805939E8 */
extern char lbl_80593D0C[];  /* "ef_drawstripestrategy.cpp"                                .data 0x80593D0C */
extern char lbl_80593D5C[];  /* "ef_drawstripestrategy.cpp"                                .data 0x80593D5C */
extern char lbl_80593DAC[];  /* "ef_drawstripestrategy.cpp"                                .data 0x80593DAC */
extern char lbl_80593E00[];  /* "particle.h"                                               .data 0x80593E00 */
extern char lbl_80593E40[];  /* "particlemanager.h"                                        .data 0x80593E40 */
extern char lbl_80593CB4[];  /* the ef_resource vtable                                     .data 0x80593CB4 */
extern char lbl_80694598[];  /* the post-field singleton                                   .bss  0x80694598 */
extern char lbl_80694C08[];  /* a draw-strategy singleton                                  .bss  0x80694C08 */
extern char lbl_80694C20[];  /* a draw-strategy singleton                                  .bss  0x80694C20 */
extern f32 lbl_80796140;    /* a stripe-strategy constant                                 .sdata2 0x80796140 */

/* Helpers owned by other units. */
extern void  MEMInitList(void* list, u16 offset);
extern void* fn_80501C9C(void* list, u16 index);
extern void  PSVECSubtract(Vec* dst, const Vec* a, const Vec* b);
extern void  fn_80041E40(void* dst, void* src);
extern void  fn_80043EA8(VEC3* out);

/* `fn_800C5F74` has no registered owner, so its declaration lives in `unsplit/ef.h` (rule 2);
 * `__dl__FPv` is `sys_mem.cpp`'s `operator delete`, reached through its owner (rules 2 and 9). */

/* The unit's own GX writers (defined below). */
void fn_800B5288(f32 x, f32 y);
int  fn_800B5298(u32 value);
void fn_800B52AC(Vec* v);
void fn_800B52BC(f32 x, f32 y, f32 z);

/* --------------------------------------------------------------------------------------------- *
 * The post-field list pair (fn_800B2878..fn_800B28B4) and its indexed accessors.
 * --------------------------------------------------------------------------------------------- */

/* Two `nw4r::ut::List`s plus their element counters; `fn_800B28B4` is the initialiser. */
typedef struct EfPostField {
    u8 pad_0x00[0x08]; /* +0x00  list A (MEMInitList, offset 4) */
    u16 count_a;       /* +0x08 */
    u8 pad_0x0A[0x02]; /* +0x0A */
    u32 field_0x0C;    /* +0x0C  zeroed by the initialiser */
    u8 list_b[0x08];   /* +0x10  list B (MEMInitList, offset 4) */
    u16 count_b;       /* +0x18 */
    u8 pad_0x1A[0x02]; /* +0x1A */
    u32 field_0x1C;    /* +0x1C  zeroed by the initialiser */
} EfPostField; /* size: 0x20 */

void* fn_800B2878(void) {
    return lbl_80694598;
}

void fn_800B28B4(EfPostField* self) {
    MEMInitList(self, 4);
    MEMInitList(&self->list_b, 4);
    self->field_0x0C = 0;
    self->field_0x1C = 0;
}

EfPostField* fn_800B2884(EfPostField* self) {
    fn_800B28B4(self);
    return self;
}

void fn_800B4ABC(void) {
    fn_800B2884((EfPostField*)lbl_80694598);
}

void* fn_800B4A70(EfPostField* self, u16 index) {
    if ((u32)index >= self->count_a) {
        return 0;
    }
    return fn_80501C9C(self, index);
}

void* fn_800B4A98(EfPostField* self, u16 index) {
    if ((u32)index >= self->count_b) {
        return 0;
    }
    return fn_80501C9C(&self->list_b, index);
}

/* --------------------------------------------------------------------------------------------- *
 * Small accessors and predicates.
 * --------------------------------------------------------------------------------------------- */

/* Returns the post-field length sentinel (the unit's constant). */
int fn_800B38B8(void) {
    return 11;
}

/* Nothing to do. */
void fn_800B4FE0(void) {}

/* A draw-strategy singleton. */
void* fn_800B612C(void) {
    return lbl_80694C20;
}

/* A draw-strategy singleton. */
void* fn_800B6138(void) {
    return lbl_80694C08;
}

/* A stripe-strategy constant. */
f32 fn_800B5A48(void) {
    return lbl_80796140;
}

/* Tests one bit of a status word. */
int fn_800B5A50(u32 value) {
    return (value & 0x8) != 0;
}

/* Tests one bit of a status word. */
int fn_800B6534(u32 value) {
    return (value & 0x10) != 0;
}

/* The draw-time particle record the flag accessors read.  `field_0xB2` packs three 2-3 bit fields,
 * `field_0xB0` the layer/parameter index, `flags_0x00` the particle state bits. */

/* The particle's stored word at +0x38. */
u32 fn_800B9628(EfParticleState* self) {
    return self->field_0x38;
}

/* The particle's stored word at +0x3C. */
u32 fn_800B5B40(EfParticleState* self) {
    return self->field_0x3C;
}

/* The post-field's second-list length. */
u16 fn_800B4A90(EfPostField* self) {
    return self->count_b;
}

/* The particle's layer/parameter index byte. */
u8 fn_800B68F8(void* ctx, EfParticleState* particle) {
    return particle->field_0xB0;
}

/* The particle's state bit 0x800. */
u32 fn_800B6994(void* ctx, EfParticleState* particle) {
    return particle->flags_0x00 & 0x800;
}

/* The particle's packed flags, bits 6-7. */
u32 fn_800B5B48(void* ctx, EfParticleState* particle) {
    return particle->field_0xB2 & 0xC0;
}

/* The particle's packed flags, bits 0-2. */
u32 fn_800B76B8(void* ctx, EfParticleState* particle) {
    return particle->field_0xB2 & 0x7;
}

/* The particle's packed flags, bits 3-5. */
u32 fn_800B83C0(void* ctx, EfParticleState* particle) {
    return particle->field_0xB2 & 0x38;
}

/* --------------------------------------------------------------------------------------------- *
 * The resource-parameter accessor family (ef_resource.cpp, 0x800B23A4..0x800B28B4).
 * --------------------------------------------------------------------------------------------- */

/* The `RES_ACCESS` record: its first word is the checked `mData` pointer, and `fn_800B23A4` asserts
 * and returns it (the out-of-line `IsValidPointer` instantiation - `ef.h`). */
typedef struct EfResDrawParam {
    u8 pad_0x00[0x94];   /* +0x00 */
    u8 field_0x94[0x4C]; /* +0x94  the parameter payload the accessors return */
} EfResDrawParam; /* size: 0xE0 */

/* The state word `fn_800B23A4` returns (a `Res*` block whose flags sit at +0). */
typedef struct EfResState {
    u16 flags_0x00; /* +0x00 */
} EfResState; /* size: 0x02 */

/* A record whose first field holds a pointer (the two `sts`-style setters store one). */
typedef struct EfResSlot {
    void* field_0x00; /* +0x00 */
} EfResSlot; /* size: 0x04 */

extern EfResState* fn_800B23A4(void* self);
extern EfResDrawParam* fn_800A5484(void* arg);
extern EfResDrawParam* fn_800A4864(EfResDrawParam* arg);

/* Sets or clears the 0x400 flag on the accessor's state word. */
void fn_800B24BC(void* self, int enable) {
    if (enable != 0) {
        fn_800B23A4(self)->flags_0x00 |= 0x400;
    } else {
        fn_800B23A4(self)->flags_0x00 &= ~0x400;
    }
}

/* Stores a pointer into the record's first field. */
void fn_800B2544(EfResSlot* self, void* value) {
    self->field_0x00 = value;
}

/* Stores a pointer into the record's first field. */
void fn_800B2590(EfResSlot* self, void* value) {
    self->field_0x00 = value;
}

/* Resolves the parameter through the two accessors and stores it. */
void fn_800B2504(EfResSlot* self, void* arg) {
    fn_800B2544(self, fn_800A4864(fn_800A5484(arg)));
}

/* Resolves the parameter and stores the second block's +0x94 field. */
void fn_800B254C(EfResSlot* self, void* arg) {
    fn_800B2590(self, &fn_800A4864(fn_800A5484(arg))->field_0x94);
}

/* The resolved parameter block the +0x4C/+0x54 accessors read. */
typedef struct EfResParams {
    u8 pad_0x00[0x4C]; /* +0x00 */
    f32 field_0x4C;    /* +0x4C */
    u8 pad_0x50[0x04]; /* +0x50 */
    u8 field_0x54[0x40]; /* +0x54 */
} EfResParams; /* size: 0x94 */

extern EfResParams* fn_800B2598(void* self);
extern EfResParams* fn_800B26F8(void* self);

/* The resolved parameter's scale. */
f32 fn_800B26B0(void* self) {
    return fn_800B2598(self)->field_0x4C;
}

/* The resolved parameter's second block. */
void* fn_800B26D4(void* self) {
    return &fn_800B2598(self)->field_0x54;
}

/* Sets the resolved parameter's scale. */
void fn_800B2810(void* self, f32 value) {
    fn_800B26F8(self)->field_0x4C = value;
}

/* Copies a source block over the resolved parameter's second block. */
void fn_800B2840(void* self, void* src) {
    fn_80041E40(&fn_800B26F8(self)->field_0x54, src);
}

/* Constructs the resource record and installs its vtable. */
void* fn_800B4AC8(void* self) {
    fn_800C5F74((EfParticleLayers*)self);
    *(void**)self = lbl_80593CB4;
    return self;
}

/* Subtracts `b` from `self` in place and returns `self`. */
Vec* fn_800B0B90(Vec* self, Vec* b) {
    PSVECSubtract(self, self, b);
    return self;
}

/* --------------------------------------------------------------------------------------------- *
 * The draw-time particle copies (ef_drawstripestrategy.cpp, 0x800B8788..0x800B882C).
 * --------------------------------------------------------------------------------------------- */

extern void* fn_80067E54(void* out, void* in);

/* Copies the particle's +0x98 block into `dst`. */
void fn_800B87C8(void* dst, EfParticleState* particle) {
    fn_80041E40(dst, &particle->field_0x98);
}

/* Builds the particle's +0xB0 transform into a local and copies it into `dst`. */
void fn_800B8788(void* dst, EfParticleState* particle) {
    u8 tmp[0x10];
    fn_80041E40(dst, fn_80067E54(tmp, &particle->field_0xB0));
}

/* --------------------------------------------------------------------------------------------- *
 * The stripe sample, the list heads and the resolution helpers (ef_drawstripestrategy.cpp).
 * --------------------------------------------------------------------------------------------- */

/* Nine words plus a trailing scalar: the sampler's per-step record.  (The target copies the nine words
 * as words and only +0x24 as a float, so they are not three `Vec`s.) */
typedef struct EfStripeParam {
    u32 words_0x00[9]; /* +0x00 */
    f32 field_0x24;    /* +0x24 */
} EfStripeParam; /* size: 0x28 */

/* The `RES_ACCESS` file header: its first word is the offset from the file base to the resource
 * header, whose +0x04 u16 the length accessors return. */
typedef struct EfResFile {
    u32 header_offset; /* +0x00  offset from the file base to the resource header */
} EfResFile; /* size: 0x04 */

/* Three positions the sample zeroer walks; `fn_800B6954` zeroes them one `Vec` at a time. */
typedef struct EfStripeSample {
    Vec a; /* +0x00 */
    Vec b; /* +0x0C */
    Vec c; /* +0x18 */
} EfStripeSample; /* size: 0x24 */


extern f32 lbl_80796124; /* a scale factor  .sdata2 0x80796124 */
extern f32 lbl_80796130; /* a scale factor  .sdata2 0x80796130 */

extern f32 fn_80052214(void* self, void* other);
extern void fn_800513F0(void* self);
extern int fn_800A5248(void* node);
extern u16 fn_800B2BB8(EfResFile* arg);
extern u16 fn_800B2CD8(EfResFile* arg);
void* fn_800B5B34(void* self, void* node);
void* fn_800B5ACC(void* self, void* node);

/* Zeroes the three positions of a stripe sample and returns it. */
EfStripeSample* fn_800B6954(EfStripeSample* self) {
    fn_80043EA8((VEC3*)&self->a);
    fn_80043EA8((VEC3*)&self->b);
    fn_80043EA8((VEC3*)&self->c);
    return self;
}

/* Copies a stripe sample. */
void fn_800B6900(EfStripeParam* dst, EfStripeParam* src) {
    *dst = *src;
}

/* The second-list length stored in the post-field when the argument is null else resolved. */
u32 fn_800B3D4C(EfPostField* self, EfResFile* arg) {
    if (arg != 0) {
        return (u16)fn_800B2BB8(arg);
    }
    return self->field_0x0C;
}

/* The stored length at +0x1C when the argument is null else resolved. */
u32 fn_800B3D84(EfPostField* self, EfResFile* arg) {
    if (arg != 0) {
        return (u16)fn_800B2CD8(arg);
    }
    return self->field_0x1C;
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

/* The per-instance byte offset a node manager carries. */
typedef struct EfNodeManager {
    u8 pad_0x00[0x42]; /* +0x00 */
    u16 field_0x42;    /* +0x42  byte offset from a node base to its link field */
} EfNodeManager; /* size: 0x44 */

/* The next list node: the manager's per-instance byte offset into the node.  The offset is a runtime
 * field, so there is no compile-time field name to reach; the byte add is the only spelling. */
void* fn_800B5B34(void* self, void* node) {
    return *(void**)((u8*)node + ((EfNodeManager*)self)->field_0x42);
}

/* The next list node, reaching four bytes past the manager's per-instance offset. */
void* fn_800B8DB0(void* self, void* node);
void* fn_800B8DB0(void* self, void* node) {
    return *(void**)((u8*)node + ((EfNodeManager*)self)->field_0x42 + 4);
}

/* Walks a node chain through fn_800B8DB0 until the callback reports 1 (or the end). */
void* fn_800B8D48(void* self, void* node) {
    void* next = fn_800B8DB0(self, node);
    while (next != 0 && fn_800A5248(next) != 1) {
        next = fn_800B8DB0(self, next);
    }
    return next;
}

/* Walks the +0x38 head through fn_800B8DB0 until the callback reports 1 (or the end). */
void* fn_800B95C0(EfParticleState* self) {
    void* next = (void*)fn_800B9628(self);
    while (next != 0 && fn_800A5248(next) != 1) {
        next = fn_800B8DB0(self, next);
    }
    return next;
}

/* Frees `self` when the signed flag is positive. */
void* fn_800B4B60(void* self, s16 flag) {
    if (self != 0 && flag > 0) {
        operator delete(self);
    }
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

/* --------------------------------------------------------------------------------------------- *
 * The list teardown and the remaining small draw helpers.
 * --------------------------------------------------------------------------------------------- */

extern void* fn_800A5250(void* list);
extern void  fn_80501BF4(void* list, void* node);
extern void  GXSetCullMode(u32 mode);
extern void  GXBegin(u32 primitive, u32 vtxfmt, u32 count);
extern void* fn_800508A8(void* arg);
extern void* fn_800508AC(void* arg);
extern void* fn_80051570(void* arg);
extern void  PSMTXMultVec(const f32* mtx, const void* src, void* dst);

/* Empties the primary list and clears its counter. */
int fn_800B483C(EfPostField* self) {
    while (fn_800A5250(self) != 0) {
        fn_80501BF4(self, fn_800A5250(self));
    }
    self->field_0x0C = 0;
    return 1;
}

/* Empties the second list and clears its counter. */
int fn_800B4A14(EfPostField* self) {
    while (fn_800A5250(&self->list_b) != 0) {
        fn_80501BF4(&self->list_b, fn_800A5250(&self->list_b));
    }
    self->field_0x1C = 0;
    return 1;
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

/* Transforms a vector by the matrix the helper pair builds, and returns `dst`. */
void* fn_800B7F58(void* dst, void* a, void* b) {
    void* x = fn_800508A8(dst);
    void* y = fn_800508AC(b);
    PSMTXMultVec((const f32*)fn_80051570(a), y, x);
    return dst;
}

/* --------------------------------------------------------------------------------------------- *
 * The indexed list search and the ahead-context vector builders.
 * --------------------------------------------------------------------------------------------- */

extern void* fn_800B308C(void* block, u16 index);
extern void* fn_80501C60(void* list, void* node);
extern void  fn_800A7F00(void* arg, Vec* v);

/* A node whose +0xAC vector the ahead builder reads. */
typedef struct EfAheadItem {
    u8 pad_0x00[0xAC]; /* +0x00 */
    Vec field_0xAC;    /* +0xAC */
} EfAheadItem; /* size: 0xB8 */

/* Finds the block holding `index`, walking the chain when no block is given. */
void* fn_800B3DBC(void* self, u32 index, EfResFile* arg) {
    if (arg != 0) {
        u32 n = (u16)fn_800B2BB8(arg);
        if (index < n) {
            return fn_800B308C(arg, (u16)index);
        }
        return 0;
    }
    void* node = fn_800A5250(self);
    while (node != 0) {
        u32 n = (u16)fn_800B2BB8((EfResFile*)node);
        if (index < n) {
            return fn_800B308C(node, (u16)index);
        }
        index -= n;
        node = fn_80501C60(self, node);
    }
    return 0;
}

/* Subtracts the two ahead vectors and copies the reference block when the result is degenerate. */
void fn_800B87D0(Vec* a, EfParticleState* particle, EfAheadItem* item) {
    PSVECSubtract(a, &item->field_0xAC, &particle->field_0xA4);
    if (fn_800B59E4(a) == 0) {
        fn_80041E40(a, &particle->field_0x98);
    }
}

/* Builds the ahead vector and copies the reference block when the result is degenerate. */
void fn_800B882C(Vec* a, EfParticleState* particle, void* arg) {
    fn_800A7F00(arg, a);
    if (fn_800B59E4(a) == 0) {
        fn_80041E40(a, &particle->field_0x98);
    }
}

/* The `RES_ACCESS` header accessors (ef_resource.cpp): `EfResFile`'s first word is the offset to the
 * resource header; the accessor validates the file pointer and returns the header's +0x04 u16. */

/* The `project` header length, validated at the caller's line. */
u16 fn_800B2BB8(EfResFile* self) {
    if (!IsValidPointer((u32)self)) {
        nw4r::db::Panic(lbl_80593690, 151, lbl_805936A0, self);
    }
    return *(u16*)((u8*)self + self->header_offset + 4);
}

/* The `searchName` header length, validated at the caller's line. */
u16 fn_800B2CD8(EfResFile* self) {
    if (!IsValidPointer((u32)self)) {
        nw4r::db::Panic(lbl_80593690, 165, lbl_805936A0, self);
    }
    return *(u16*)((u8*)self + self->header_offset + 4);
}

/* --------------------------------------------------------------------------------------------- *
 * The out-of-line GX FIFO writers, 0x800B5288..0x800B54B4 (ef_drawstripestrategy.cpp's own copies;
 * the registered `ef_drawsmoothstripestrategy.cpp` carries the parallel `0x800C6F*` family).
 * --------------------------------------------------------------------------------------------- */

/* Writes a pair of f32 to the pipe. */
void fn_800B5288(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Tests the low bit of a status word (the booleanised `(value & 1) != 0`). */
int fn_800B5298(u32 value) {
    return (value & 1) != 0;
}

/* Writes one vector to the pipe. */
void fn_800B52BC(f32 x, f32 y, f32 z);
void fn_800B52AC(Vec* v) {
    fn_800B52BC(v->x, v->y, v->z);
}
/* Writes three f32 to the pipe. */
void fn_800B52BC(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

#ifdef __cplusplus
}
#endif
