/* ef/fn_800FE978.cpp - the effect-family spawn/init handler at `.text` 0x800FE978-0x800FF8D4
 * (3 functions, 0xF5C bytes).  Registered once, at its final home (docs/plan.md 12).
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every
 * fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt; dumpmap returns only
 * the placeholder `zz_00fe978_`, and the object carries no `__FILE__` string).
 *
 * Naming evidence (brief 2): class 4.  No `__FILE__` string is referenced by the object (checked:
 * every relocation in `.text` is a callee or a pool literal, no `.data` filename pool), the runtime
 * dump names all three symbols `zz_*` (not evidence), and the map carries only `fn_XXXXXXXX`.  The
 * module is `ef` (every neighbour in the band is `ef`; the code drives `nw4r::ef::Effect` through
 * `res_eft_create` and the `_EFT` records of `ef.h`).  The map stem is kept as the file name.
 *
 * Language: C++ (langcheck: 10 mangled callees, e.g. `res_eft_create__FUsUsUl`,
 * `SetRootMtx__Q34nw4r2ef6EffectFRCQ34nw4r4math5MTX34`); the object carries `extab`/`extabindex`
 * (confounded by the `ef` lib's `-Cpp_exceptions on`, but the mangled callees decide it).
 *
 * What it is: the family's per-spawn state machine.  It builds the resource path from the
 * per-map/per-area table (`lbl_8059BA18`..`lbl_8059BF84`), loads the family's effect file into the
 * work block (`_EFT::work_0x38`), and either seeds the work from the file (type 0/1) or drives the
 * display-list of pre-placed slots (`res_eft_create` / `fn_800F91C4`, `SetRootMtx`), then advances
 * the state (`fn_800FF8D4`) or bails through the library's error path (`fn_800FF886C`/`fn_800FFCA8`).
 *
 * Status: the two small helpers (`fn_800FF8A0`, `fn_800FF840`) are reconstructed byte-identical
 * (100 %).  `fn_800FE978` (0xEC8 = 3784 B) is the spawn state machine; it reaches **73.47 %** and is
 * the recorded residual.
 *
 * Residual (`fn_800FE978`, 73.47 %, target 3784 B / ours 3352 B): the control flow, all 11 per-map
 * name tables, the file load and the two/three-case dispatch blocks are reconstructed; what still
 * differs is codegen, not shape:
 *   * prologue: the original frame is 0x840 and MWCC emits the dynamic `stwux` prologue
 *     (`clrlwi`/`subfic`/`mr r12`/`stwux`) saving r22-r31; ours is a static 0x8F0 frame saving
 *     r21-r31 (one extra live web), so `self` lands in r31 where retail has r30 and every
 *     r30/r31 operand row differs.  Neither reversing the local declarations nor forcing `size`
 *     onto the stack (an array, a `volatile`) moved MWCC's slot assignment - the layout is the
 *     allocator's, given the same local types.
 *   * stack slots: retail's locals are info(0x20) mtx(0x60) header(0x90) size(0x128) name(0x12C)
 *     set(0x240); ours are mtx(0x08) info(0x38) header(0x70) name(0x108) set(0x308), so every
 *     `r1`-relative immediate differs.
 *   * 108 target-only instructions (432 B): the two `mapno`-22/21 area switches and the
 *     `fn_802FB8EC` sub-switches compile to compare chains in retail but to local jump tables
 *     (`.data` 0x50 B) here; the report metric ignores the relocation name, not the shape.
 * The shapes that reproduce the machine are all present and measured; what is left is the
 * register/stack colouring the retail build chose, which is not reachable by a source rewrite of
 * this function alone.
 */

#include "types.h"
#include "ef/fn_800CDB2C.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft001.h"
#include "ef/effect.h"
#include "ef/eft004.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* ------------------------------------------------------------------------------------------------ */
/* pooled data (owned by the map, referenced by name - playbook 29)                                   */
/* ------------------------------------------------------------------------------------------------ */

/* The per-map, per-area resource-name tables the first switch selects from. */
extern char* lbl_8059BA18[];
extern char* lbl_8059BAE0[];
extern char* lbl_8059BBF0[];
extern char* lbl_8059BC4C[];
extern char* lbl_8059BC7C[];
extern char* lbl_8059BCAC[];
extern char* lbl_8059BD24[];
extern char* lbl_8059BDE8[];
extern char* lbl_8059BEF8[];
extern char* lbl_8059BF54[];
extern char* lbl_8059BF84[];

/* The four colour/scale constants the type-5/16 area cases write into `_EFT::pos_0x18`. */
extern f32 lbl_80796690;
extern f32 lbl_80796694;
extern f32 lbl_80796698;
extern f32 lbl_8079669C;
extern f32 lbl_807966A0;
extern f32 lbl_807966A4;

/* ------------------------------------------------------------------------------------------------ */
/* externs                                                                                            */
/* ------------------------------------------------------------------------------------------------ */

/* MTX34 identity (`fn_8005050C`), VEC3 clear (`fn_80043EA8`) - both stubs the map still spells. */
extern "C" void fn_8005050C(nw4r::math::MTX34* mtx);
extern "C" void fn_80043EA8(nw4r::math::VEC3* out);

/* The nw4r::ef / engine helpers, reached through their real signatures (rule 9). */
nw4r::ef::Effect* res_eft_create(u16 id, u16 kind, u32 arg);
extern "C" nw4r::ef::Effect* fn_800F91C4(u16 id, u16 kind, s32 a, s32 b);
extern "C" void fn_800532DC(nw4r::math::MTX34* dst, nw4r::math::MTX34* src);
/* C++ callees: the target object references their manglings (cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34,
 * work_mem_alloc__FUl, work_mem_free__FPv, load_file__FPcUll, ran_suu__Fl), so no extern "C"
 * (relocaudit).  fn_802FB8EC is the reverse: its target spelling is plain, so it keeps C linkage. */
void cpSetRotMatrix(_CP_VECTOR* rot, nw4r::math::MTX34* mtx);
void setVector3(nw4r::math::VEC3* out, f32 x, f32 y, f32 z);
extern "C" void fn_800F886C(void* self);
/* File load (`fn_800CEE2C`/`fn_804A6120`) and the work heap.  The eft004 pool helpers and `memcpy`
 * come from their owners' headers (rule 2). */
extern "C" s32 fn_804A6120(void* info);
void* work_mem_alloc(u32 size);
void work_mem_free(void* ptr);
void load_file(char* path, u32 dst, s32 size);
u16 ran_suu(s32 index);
u8 get_now_mapno();
extern "C" u8 fn_802FB8EC(u8 index);
u32 LbCheckKujiraEvent();

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                              */
/* ------------------------------------------------------------------------------------------------ */

/* The 0x3C-byte file stat `fn_800CEE2C` fills; only the byte count at +0x34 is read here.  Its own
 * name because `src/fn_80040598.cpp` carries the SDK spelling of the same record (rule 1: a name
 * moves to `include/` the second *unit* needs it; this unit only reads one field). */
struct EftFileStat {
    /* +0x00 */ u8 pad_0x00[0x34];
    /* +0x34 */ u32 length;
}; /* size: 0x38 */

/* The 0x98-byte header at the head of the family's effect file. */
struct EftFileHeader {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 pad_0x01[0x0B];
    /* +0x0C */ u8 slot_count;
    /* +0x0D */ u8 pad_0x0D[0x07];
    /* +0x14 */ u32 slot_offset;
    /* +0x18 */ u8 pad_0x18[0x80];
}; /* size: 0x98 */

/* One 0x2C-byte slot of the spawn set: a type byte, a flag byte, two 16-bit weights, the slot's
 * vector and its rotation triple. */
struct EftSpawnSlot {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 pad_0x01[0x02];
    /* +0x03 */ u8 flags_0x03;
    /* +0x04 */ s16 value_0x04;
    /* +0x06 */ s16 value_0x06;
    /* +0x08 */ u8 pad_0x08[0x0C];
    /* +0x14 */ nw4r::math::VEC3 vec_0x14;
    /* +0x20 */ _CP_VECTOR rot_0x20;
}; /* size: 0x2C */

/* The spawn set the resource loader fills and `fn_800FF840` releases: a small header, a rotation
 * vector at +0x84 and 30 slots at +0x90. */
struct EftSpawnSet {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 pad_0x03[0x81];
    /* +0x84 */ nw4r::math::VEC3 vec_0x84;
    /* +0x90 */ EftSpawnSlot slots_0x90[30];
}; /* size: 0x5B8 */

/* One 0x14-byte slot-bookkeeping record of the work block (`work + 0xD8 + i*0x14`). */
struct EftSpawnWorkSlot {
    /* +0x00 */ u8 slot_index;
    /* +0x01 */ u8 pad_0x01[0x03];
    /* +0x04 */ s32 value_a;
    /* +0x08 */ s32 value_b;
    /* +0x0C */ s32 value_c;
    /* +0x10 */ u8 pad_0x10[0x04];
}; /* size: 0x14 */

/* The family's work block (`_EFT::work_0x38`).  The three sections are sized from the strides the
 * functions use: effects[30] (0x04..0x7C), effects2[20] (0x80..0xD0), slots[30] (0xD8..0x330),
 * matrices[30] (0x330..0x8D0), then the slot run and the per-slot type bytes. */
struct EftSpawnWork {
    /* +0x000 */ s32 count;
    /* +0x004 */ nw4r::ef::Effect* effects[30];
    /* +0x07C */ s32 count2;
    /* +0x080 */ nw4r::ef::Effect* effects2[20];
    /* +0x0D0 */ u16 id_base;
    /* +0x0D4 */ u16 kind;
    /* +0x0D8 */ EftSpawnWorkSlot slots[30];
    /* +0x330 */ nw4r::math::MTX34 matrices[30];
    /* +0x8D0 */ s32 slot_count;
    /* +0x8D4 */ u8 pad_0x8D4[0x01];
    /* +0x8D5 */ u8 types_0x8D5[30];
}; /* size: 0x8F3 (lower bound: +0x8D5 is the highest offset any function of this unit reads) */

/* ------------------------------------------------------------------------------------------------ */
/* functions                                                                                          */
/* ------------------------------------------------------------------------------------------------ */

/* The two spawn-set helpers, defined below (same TU). */
extern "C" EftSpawnSet* fn_800FF840(EftSpawnSet* self);
extern "C" EftSpawnSlot* fn_800FF8A0(EftSpawnSlot* self);

/* The family's spawn/init state machine. */
extern "C" void fn_800FE978(_EFT* self)
{
    EftFileStat info;
    nw4r::math::MTX34 mtx;
    EftFileHeader header;
    u32 size;
    char name[0x200];
    EftSpawnSet set;
    EftSpawnWork* work;
    char* src;
    void* mem;
    s32 mapno;
    s32 i;

    fn_8005050C(&mtx);
    fn_800FF840(&set);

    work = (EftSpawnWork*)self->work_0x38;
    self->state_0x05++;
    mapno = get_now_mapno();

    switch (mapno) {
    case 1:  src = lbl_8059BA18[self->area_0x44]; break;
    case 2:  src = lbl_8059BAE0[self->area_0x44]; break;
    case 4:  src = lbl_8059BBF0[self->area_0x44]; break;
    case 6:  src = lbl_8059BC4C[self->area_0x44]; break;
    case 8:  src = lbl_8059BC7C[self->area_0x44]; break;
    case 9:  src = lbl_8059BCAC[self->area_0x44]; break;
    case 12: src = lbl_8059BD24[self->area_0x44]; break;
    case 13: src = lbl_8059BDE8[self->area_0x44]; break;
    case 15: src = lbl_8059BEF8[self->area_0x44]; break;
    case 17: src = lbl_8059BF54[self->area_0x44]; break;
    case 19: src = lbl_8059BF84[self->area_0x44]; break;
    default: src = NULL; break;
    }

    if (src != NULL) {
        char* dst = name;
        s32 n = 0;

        while ((s8)*src != 0) {
            *dst = *src;
            n++;
            dst++;
            src++;
        }
        *(dst + n) = 0;
        size = 0x1000;
    } else {
        size = 0;
    }

    if (size != 0) {
        if (fn_800CEE2C(name, &info) != 0) {
            size = (info.length + 0x1F) & ~0x1FU;
            fn_804A6120(&info);
            mem = work_mem_alloc(size);
            load_file(name, (u32)mem, (s32)size);
            memcpy(&header, mem, 0x98);
            memcpy(set.slots_0x90, (u8*)mem + header.slot_offset, header.slot_count * 0x2C);
            set.field_0x00 = 1;
            set.field_0x02 = header.slot_count;
            set.field_0x01 = header.field_0x00;

            if (self->type_0x02 == 0) {
                i = 0;
                while (i < header.slot_count) {
                    if (set.slots_0x90[i].flags_0x03 & 1) {
                        EftSpawnWorkSlot* slot = &work->slots[work->slot_count];

                        slot->value_a = set.slots_0x90[i].value_0x04;
                        slot->value_b = set.slots_0x90[i].value_0x06;
                        slot->value_c = (s32)(u16)ran_suu(0) % slot->value_b;
                        slot->slot_index = (u8)i;
                        work->slot_count++;
                    }
                    i++;
                }

                i = 0;
                while (i < work->count) {
                    work->effects[i] = res_eft_create((u16)(work->id_base + i), work->kind, 1);
                    if (work->effects[i] == NULL) {
                        work_mem_free(mem);
                        fn_800FFCA8(self);
                        return;
                    }
                    fn_800F996C(work->effects[i], 0);
                    i++;
                }
            } else if (self->type_0x02 == 1) {
                work->count = header.slot_count;

                i = 0;
                while (i < work->count) {
                    if (set.slots_0x90[i].flags_0x03 & 1) {
                        EftSpawnWorkSlot* slot = &work->slots[work->slot_count];

                        slot->value_a = set.slots_0x90[i].value_0x04;
                        slot->value_b = set.slots_0x90[i].value_0x06;
                        slot->value_c = (s32)(u16)ran_suu(0) % slot->value_b;
                        slot->slot_index = (u8)i;
                        work->slot_count++;
                    }
                    i++;
                }

                i = 0;
                while (i < work->count) {
                    work->types_0x8D5[i] = set.slots_0x90[i].field_0x00;
                    cpSetRotMatrix(&set.slots_0x90[i].rot_0x20, &mtx);
                    fn_800FBB90(&mtx, &set.slots_0x90[i].vec_0x14);
                    fn_800532DC(&work->matrices[i], &mtx);

                    if (set.slots_0x90[i].flags_0x03 & 2) {
                        work->effects[i] = fn_800F91C4((u16)(work->id_base + work->types_0x8D5[i]),
                                                       work->kind, 1, ran_suu(0) & 0x3F);
                        if (work->effects[i] != NULL) {
                            work->effects[i]->SetRootMtx(work->matrices[i]);
                            fn_800F996C(work->effects[i], 0);
                        }
                    } else if ((ran_suu(0) & 3) == 0) {
                        work->effects[i] = fn_800F91C4((u16)(work->id_base + work->types_0x8D5[i]),
                                                       work->kind, 1, ran_suu(0) & 0xF);
                        if (work->effects[i] != NULL) {
                            work->effects[i]->SetRootMtx(work->matrices[i]);
                            fn_800F996C(work->effects[i], 0);
                        }
                    } else {
                        work->effects[i] = NULL;
                    }
                    i++;
                }
            }

            work_mem_free(mem);
        } else {
            fn_800FFCA8(self);
            return;
        }
    } else {
        s32 base = 0;
        s32 kind = 0;
        s32 count = 0;

        i = 0;
        while (i < work->count) {
            work->effects[i] = fn_800F91C4((u16)(work->id_base + i), work->kind, 1, 200);
            if (work->effects[i] == NULL) {
                fn_800FFCA8(self);
                return;
            }
            if (mapno == 16 || mapno == 5) {
                switch (self->area_0x44) {
                case 0: case 1: case 2: case 3: case 6: case 8:
                    work->effects[i]->owner_0x44 = self;
                    self->demo_flag_0x08 = 1;
                    break;
                case 9:
                    work->effects[i]->owner_0x44 = self;
                    self->demo_flag_0x08 = 3;
                    setVector3(&self->pos_0x18, lbl_80796690, lbl_80796694, lbl_80796698);
                    break;
                case 10:
                    work->effects[i]->owner_0x44 = self;
                    self->demo_flag_0x08 = 2;
                    setVector3(&self->pos_0x18, lbl_8079669C, lbl_807966A0, lbl_807966A4);
                    break;
                }
            }
            i++;
        }

        if (mapno == 22 || mapno == 21) {
            if (mapno == 22) {
                switch (self->area_0x44) {
                case 0:
                    count = 15;
                    base = 0x653;
                    kind = 0x16;
                    break;
                case 1:
                    count = 13;
                    base = 0x668;
                    kind = 0x19;
                    break;
                case 2:
                    switch (fn_802FB8EC(3)) {
                    case 1: work->effects[i] = fn_800F91C4(0x401, work->kind, 1, 200); i++; work->count++; break;
                    case 2: work->effects[i] = fn_800F91C4(0x402, work->kind, 1, 200); i++; work->count++; break;
                    case 3: work->effects[i] = fn_800F91C4(0x403, work->kind, 1, 200); i++; work->count++; break;
                    }
                    switch (fn_802FB8EC(0)) {
                    case 1: work->effects[i] = fn_800F91C4(0x404, work->kind, 1, 200); i++; work->count++; break;
                    case 2: work->effects[i] = fn_800F91C4(0x405, work->kind, 1, 200); i++; work->count++; break;
                    case 3: work->effects[i] = fn_800F91C4(0x406, work->kind, 1, 200); i++; work->count++; break;
                    }
                    switch (fn_802FB8EC(2)) {
                    case 1: work->effects[i] = fn_800F91C4(0x407, work->kind, 1, 200); work->count++; break;
                    case 2: work->effects[i] = fn_800F91C4(0x408, work->kind, 1, 200); work->count++; break;
                    case 3: work->effects[i] = fn_800F91C4(0x409, work->kind, 1, 200); work->count++; break;
                    }
                    count = 7;
                    base = 0x675;
                    kind = 0x1A;
                    break;
                default:
                    fn_800F886C(self);
                    return;
                }
            } else {
                switch (self->area_0x44) {
                case 0: count = 8;  base = 0x26B; kind = 0xE;  break;
                case 1: count = 12; base = 0x1A;  kind = 6;    break;
                case 2: count = 8;  base = 0x273; kind = 0xF;  break;
                case 3: count = 4;  base = 0x27B; kind = 0x10; break;
                case 5: count = 1;  base = 0x27F; kind = 0x11; break;
                case 6: count = 2;  base = 0x280; kind = 0x12; break;
                case 7: count = 1;  base = 0x282; kind = 0x13; break;
                default:
                    fn_800F886C(self);
                    return;
                }
            }

            work->count2 = count;
            i = 0;
            while (i < work->count2) {
                work->effects2[i] = fn_800F91C4((u16)(base + i), (u16)kind, 1, 200);
                if (work->effects2[i] == NULL) {
                    fn_800FFCA8(self);
                    return;
                }
                i++;
            }

            if (mapno == 22) {
                if (self->area_0x44 == 2) {
                    switch (fn_802FB8EC(3)) {
                    case 1: work->effects2[i] = fn_800F91C4(0x67C, (u16)kind, 1, 200); i++; work->count2++; break;
                    case 2: work->effects2[i] = fn_800F91C4(0x67D, (u16)kind, 1, 200); i++; work->count2++; break;
                    case 3: work->effects2[i] = fn_800F91C4(0x67E, (u16)kind, 1, 200); i++; work->count2++; break;
                    }
                    switch (fn_802FB8EC(0)) {
                    case 1: work->effects2[i] = fn_800F91C4(0x67F, (u16)kind, 1, 200); i++; work->count2++; break;
                    case 2: work->effects2[i] = fn_800F91C4(0x680, (u16)kind, 1, 200); i++; work->count2++; break;
                    case 3: work->effects2[i] = fn_800F91C4(0x681, (u16)kind, 1, 200); i++; work->count2++; break;
                    }
                    switch (fn_802FB8EC(2)) {
                    case 1: work->effects2[i] = fn_800F91C4(0x682, (u16)kind, 1, 200); work->count2++; break;
                    case 2: work->effects2[i] = fn_800F91C4(0x683, (u16)kind, 1, 200); work->count2++; break;
                    case 3: work->effects2[i] = fn_800F91C4(0x684, (u16)kind, 1, 200); work->count2++; break;
                    }
                }
            } else if (LbCheckKujiraEvent() == 1 && self->area_0x44 < 4) {
                i = 0;
                while (i < work->count) {
                    fn_80100088(work->effects[i], 0);
                    i++;
                }
                i = 0;
                while (i < work->count) {
                    work->effects[i] = fn_800F91C4((u16)(work->id_base + i), work->kind, 1, 200);
                    if (work->effects[i] == NULL) {
                        fn_800FFCA8(self);
                        return;
                    }
                    i++;
                }
                i = 0;
                while (i < work->count2) {
                    fn_80100088(work->effects2[i], 1);
                    i++;
                }
                i = 0;
                while (i < work->count2) {
                    work->effects2[i] = fn_800F91C4((u16)(base + i), (u16)kind, 1, 200);
                    if (work->effects2[i] == NULL) {
                        fn_800FFCA8(self);
                        return;
                    }
                    i++;
                }
            }
        }
    }

    self->flag_0x01 = 1;
    fn_800FF8D4(self);
}

/* Releases the spawn set: the header vector, then every slot, and returns the set. */
extern "C" EftSpawnSet* fn_800FF840(EftSpawnSet* self)
{
    EftSpawnSlot* end;
    EftSpawnSlot* slot;

    fn_80043EA8(&self->vec_0x84);
    slot = self->slots_0x90;
    end = self->slots_0x90 + 30;
    do {
        fn_800FF8A0(slot);
        slot++;
    } while (slot < end);
    return self;
}

/* Releases one spawn-set slot's embedded vector state and returns the slot. */
extern "C" EftSpawnSlot* fn_800FF8A0(EftSpawnSlot* self)
{
    fn_80043EA8(&self->vec_0x14);
    return self;
}
