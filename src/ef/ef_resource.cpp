/*
 * ef/ef_resource.cpp - nw4r::ef::Resource: the resource singleton (two `nw4r::ut::List`s of loaded resource files
 *   and their counters), its constructor and static initialiser, the indexed list accessors and teardown, and the
 *   per-file header lookups by index and by name.
 * RANGE. .text 0x800B2878-0x800B4AC8 (31 functions); extab 0x8000A044-0x8000A10C, extabindex 0x8002340C-0x80023538,
 *   .ctors 0x8056F2E0-0x8056F2E4 (the static initialiser 0x800B4ABC), .data 0x80593690-0x805939E8 (the
 *   `__FILE__` string "ef_resource.cpp" first, cited from 0x800B28FC to 0x800B4898), .bss 0x80694598-0x806945B8
 *   (the singleton), .sdata 0x807912E8-0x80791300 (read by 0x800B3670 and 0x800B3E80).  Left edge: see
 *   `ef/ef_postfield.cpp`.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off`.
 * NAMES. The file name is the TU's own `__FILE__` string (0x80593690), not a guess.  `EfPostField` is the resource
 *   singleton's record (its name predates the seam); the helpers keep the map's `fn_` stems.
 * RESIDUALS. 16 rows unwritten (declared, never defined): 0x800B28FC-0x800B2BB8, 0x800B2DF8-0x800B38B8,
 *   0x800B38C0-0x800B3D4C, 0x800B3E80-0x800B483C, 0x800B4898-0x800B4A14.  The 15 written rows are byte-identical; the
 *   source order differs from retail's, so our `.text` and the extab and extabindex records run in another order.
 *   flipcheck: `.bss`, `.ctors`, `.data` and `.sdata` claimed, not emitted (declared by their map names, playbook 29).
 */

#include "types.h"
#include "gx.h"
#include "ef.h"
#include "ef/ef_drawstrategyimpl.h"
#include "ef/fn_800AEE48.h"
#include "ef/ef_resource.h"
#include "sys_mem.h"
#include "unsplit/ef.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "nw4r/fn_805012C4.h" /* nw4r::ut::List_* (rule 2) */

#ifdef __cplusplus
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r
extern "C" {
#endif

#pragma peephole off

/* This TU's `__FILE__`/assert strings (its claimed `.data`) and its singleton (`.bss`), declared, never defined. */
extern char lbl_80593690[];  /* "ef_resource.cpp"                                          .data 0x80593690 */
extern char lbl_805936A0[];  /* "NW4R:Pointer Error\nproject(=%p) is not valid..."          .data 0x805936A0 */
extern char lbl_80694598[];  /* the resource singleton                                     .bss  0x80694598 */

/* Helpers owned by other units. */
extern void* fn_800A5250(void* list);

/* Two `nw4r::ut::List`s plus their element counters; `fn_800B28B4` is the initialiser. */
typedef struct EfPostField {
    u8 pad_0x00[0x08]; /* +0x00  list A (List_Init, offset 4) */
    u16 count_a;       /* +0x08 */
    u8 pad_0x0A[0x02]; /* +0x0A */
    u32 field_0x0C;    /* +0x0C  zeroed by the initialiser */
    u8 list_b[0x08];   /* +0x10  list B (List_Init, offset 4) */
    u16 count_b;       /* +0x18 */
    u8 pad_0x1A[0x02]; /* +0x1A */
    u32 field_0x1C;    /* +0x1C  zeroed by the initialiser */
} EfPostField; /* size: 0x20 */

void* fn_800B2878(void) {
    return lbl_80694598;
}

void fn_800B28B4(EfPostField* self) {
    nw4r::ut::List_Init((nw4r::ut::List*)self, 4);
    nw4r::ut::List_Init((nw4r::ut::List*)&self->list_b, 4);
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
    return nw4r::ut::List_GetNth((nw4r::ut::List*)self, index);
}

void* fn_800B4A98(EfPostField* self, u16 index) {
    if ((u32)index >= self->count_b) {
        return 0;
    }
    return nw4r::ut::List_GetNth((nw4r::ut::List*)&self->list_b, index);
}

/* Returns the post-field length sentinel (the unit's constant). */
int fn_800B38B8(void) {
    return 11;
}

/* The post-field's second-list length. */
u16 fn_800B4A90(EfPostField* self) {
    return self->count_b;
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

/* Empties the primary list and clears its counter. */
int fn_800B483C(EfPostField* self) {
    while (fn_800A5250(self) != 0) {
        nw4r::ut::List_Remove((nw4r::ut::List*)self, fn_800A5250(self));
    }
    self->field_0x0C = 0;
    return 1;
}

/* Empties the second list and clears its counter. */
int fn_800B4A14(EfPostField* self) {
    while (fn_800A5250(&self->list_b) != 0) {
        nw4r::ut::List_Remove((nw4r::ut::List*)&self->list_b, fn_800A5250(&self->list_b));
    }
    self->field_0x1C = 0;
    return 1;
}

extern void* fn_800B308C(void* block, u16 index);

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
        node = nw4r::ut::List_GetNext((nw4r::ut::List*)self, node);
    }
    return 0;
}

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
