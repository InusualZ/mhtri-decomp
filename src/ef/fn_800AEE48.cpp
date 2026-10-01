/* ef/fn_800AEE48.cpp - nw4r::ef post-field / resource layer, `.text` 0x800AEE48..0x800B4AC8.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * What it is.  The first two TUs of the NintendoWare-for-Revolution effect library (`nw4r::ef`) that the discovery's
 * `proposal/800AEE48_fn_800AEE48.cpp` range (0x800AEE48..0x800B99E8, capped at `--max-bytes`) held.  The split object's own `__FILE__`
 * strings name them, in `.data` order (which is `.text` order): `ef_postfield.cpp` (0x80593580) and `ef_resource.cpp` (0x80593690), plus the
 * header strings `res_drawparam_ac.h` and `res_emitterparam_ac.h`.  Provisional internal seam, from the panic-site file strings:
 *   0x800AEE48..0x800B2810   ef_postfield.cpp          (fn_800AF440 / fn_800B18F0 cite it)
 *   0x800B2810..0x800B4AC8   ef_resource.cpp           (fn_800B28FC .. fn_800B4898 cite it)
 * Phase 4 cut the third TU, the head of `ef_drawstripestrategy.cpp` (0x800B4AC8..0x800B99E8, `.data` 0x805939E8..), into
 * `ef/ef_drawstripestrategy.cpp` together with the 36 functions already written for it.
 *
 * Two names mean no single one names the unit, so the registration keeps the map's `fn_` stem (evidence class 4: the `__FILE__` evidence
 * decides the module `ef` and the language C++).  `langcheck.py` confirms C++ (the `Panic__Q24nw4r2dbFPCciPCce` callee and the `.cpp` names).
 *
 * Data.  The unit owns no pool section here: its `.data`/`.sdata`/`.sdata2` labels are `extern` by their map names and never defined
 * (playbook 29); the data pass claims the ranges once the source emits them (docs/plan.md 8.4).
 *
 * Status.  25 of the unit's 60 symbols are reconstructed.  The missing ones are the `ef_postfield.cpp` body (`fn_800AEE48`..
 * `fn_800B23A4`, 0x800AEE48..0x800B2810) and the `ef_resource.cpp` loader (`fn_800B28FC`.. `fn_800B4898`).  Residuals are in
 * `.pi/notes/800aee48-fn-800aee48-517d.md`.
 */

#include "types.h"
#include "gx.h"
#include "ef.h"
#include "ef/ef_drawstrategyimpl.h"
#include "ef/fn_800AEE48.h"
#include "sys_mem.h"
#include "unsplit/ef.h"
#include "g3d/fn_80063888.h" /* fn_80067E54, owned by g3d/fn_80063888.cpp (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

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
extern char lbl_80694598[];  /* the post-field singleton                                   .bss  0x80694598 */

/* Helpers owned by other units. */
extern void  MEMInitList(void* list, u16 offset);
extern void* fn_80501C9C(void* list, u16 index);
extern void  PSVECSubtract(Vec* dst, const Vec* a, const Vec* b);


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







/* The draw-time particle record the flag accessors read.  `field_0xB2` packs three 2-3 bit fields,
 * `field_0xB0` the layer/parameter index, `flags_0x00` the particle state bits. */



/* The post-field's second-list length. */
u16 fn_800B4A90(EfPostField* self) {
    return self->count_b;
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
    VEC3 field_0x54;   /* +0x54 */
    u8 pad_0x60[0x34]; /* +0x60 */
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
void fn_800B2840(void* self, const nw4r::math::VEC3* src) {
    copyVec3(&fn_800B26F8(self)->field_0x54, src);
}


/* Subtracts `b` from `self` in place and returns `self`. */
Vec* fn_800B0B90(Vec* self, Vec* b) {
    PSVECSubtract(self, self, b);
    return self;
}


/* The `RES_ACCESS` file header: its first word is the offset from the file base to the resource
 * header, whose +0x04 u16 the length accessors return. */
typedef struct EfResFile {
    u32 header_offset; /* +0x00  offset from the file base to the resource header */
} EfResFile; /* size: 0x04 */



extern u16 fn_800B2BB8(EfResFile* arg);
extern u16 fn_800B2CD8(EfResFile* arg);



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









/* --------------------------------------------------------------------------------------------- *
 * The list teardown and the remaining small draw helpers.
 * --------------------------------------------------------------------------------------------- */

extern void* fn_800A5250(void* list);
extern void  fn_80501BF4(void* list, void* node);

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



/* --------------------------------------------------------------------------------------------- *
 * The indexed list search and the ahead-context vector builders.
 * --------------------------------------------------------------------------------------------- */

extern void* fn_800B308C(void* block, u16 index);
extern void* fn_80501C60(void* list, void* node);

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

#ifdef __cplusplus
}
#endif
