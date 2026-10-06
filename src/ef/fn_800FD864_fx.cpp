/* ef/fn_800FD864_fx.cpp - the eft004 family's map/area spawner and its hooks, the per-spawn state machine and the
 *   head of the family's per-frame handlers.
 * RANGE. .text 0x800FD864-0x80100448 (17 functions); extab 0x8000BE0C-0x8000BE74, extabindex 0x80025BFC-0x80025C98,
 *   .data 0x8059B9A8-0x8059C528, .sdata 0x807916E8-0x80791788, .sdata2 0x80796690-0x807966B8.  `fn_800FD864` takes the
 *   0x48-byte slot `eft_res_slot_get(0x8F4)` hands out, picks the spawn parameters by map and area and installs
 *   `fn_800FE93C` (the four-state dispatcher: `fn_800FE978`, `fn_800FF8D4`, `fn_800FFC98`, `fn_800FFCA8`) and
 *   `fn_800FE8E8` (the release hook); `fn_800FE978` loads the family's effect file per map and area and seeds or
 *   drives the pre-placed slots.  The setters from 0x80100448 are `ef/eft004_fx.cpp`.
 * FLAGS. `cflags_main`; `#pragma peephole off` for `fn_800FD864`, `fn_800FE8E8`, `fn_800FE93C` and from `fn_800FFC98`
 *   to the end (retail keeps the hook address in `r0` and the unfused `rlwinm` + `cmpwi`; playbook 39).
 * NAMES. The map has only `fn_` stems here (the dump has placeholders), so the definitions are `extern "C"`; the
 *   unit is C++ because the mangled callees are called through their real signatures.
 * RESIDUALS. 3 rows unwritten: 0x800FF8D4-0x800FFC98 (`fn_800FF8D4`), 0x800FFCAC-0x800FFF08 (`fn_800FFCAC`),
 *   0x80100088-0x80100300 (`fn_80100088`).
 *   3 partial rows:
 *  - `fn_800FE978`: retail's frame is 0x840 with the dynamic `stwux` prologue saving r22-r31, ours a static 0x8F0
 *    frame saving r21-r31, so `self` and every stack slot move; the `mapno` 21/22 area switches and the
 *    `fn_802FB8EC` sub-switches are compare chains in retail and local jump tables in ours;
 *  - `fn_800FFF08`: `self->work_0x38` and `&eft_control` swap `r3`/`r5`;
 *  - `fn_80100330`: the `IsValidPointer` inline tests the value in `r4` where retail uses `r6`.
 *   Ours emits the eight `fn_800FD864` switch tables and three `fn_800FE978` tables locally (`@523`-`@533`, `@837`)
 *   where retail relocates against the `.data` run's `jumptable_` entries.
 *   flipcheck: `.sdata`/`.sdata2` claimed, not emitted; `.text` (0x219C of 0x2BE4), extab (0x50 of 0x68), extabindex
 *   (0x78 of 0x9C) and `.data` (0x274 of 0xB80) short of the claim and differing.
 * SHAPES. `fn_800FD864`'s `areano` is `u32` (the small area switches then compare `cmplwi`), assigned through a `(u8)`
 *   cast (retail's `clrlwi r3,24`); each case assigns its three parameters in retail's order (`a`, `b`, `c`; `b`, `a`,
 *   `c` for maps 21/22) and the tail stores them as `+0x00 = b`, `+0x0D0 = a`, `+0x0D4 = c`.
 *   `_EFT_MAP_WORK` is the 0x8F4-byte family block at `+0x38`.
 */

#include "types.h"
#include "ef/fn_800FE978.h"
#include "ef/effect.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft004.h"
#include "unsplit/ef.h"
#include "ef/fn_800CDB2C.h"
#include "ef/eft001.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/fn_80101DF4.h"
#include "ef/eft007.h"
#include "sound/fn_800D7F54.h"
#include "ef/fn_800FD864_fx_types.h"

/* ---------------------------------------------------------------------------------------------------
 * the family work block `eft_res_slot_get(0x8F4)` attaches to the slot at `_EFT::work_0x38`
 * ------------------------------------------------------------------------------------------------- */

/* The `eft004` family's work block.  `fn_800FE8E8` releases the two effect-heap runs the family fills
 * (`+0x00`/`+0x04` and `+0x7C`/`+0x80`); `fn_800FD864` writes the first run's count and the two spawn
 * parameters at `+0x0D0`/`+0x0D4` that the state-1 handler reads.  The remaining offsets are present in
 * the original object but untouched by this unit's functions, so they stay padding. size: 0x8F4 */
struct _EFT_MAP_WORK {
    /* +0x000 */ s32 count_0x000;             /* the first effect-heap run's count */
    /* +0x004 */ nw4r::ef::Effect* effects_0x004; /* its first handle */
    /* +0x008 */ u8 pad_0x008[0x7C - 0x08];
    /* +0x07C */ s32 count_0x07C;             /* the second effect-heap run's count */
    /* +0x080 */ nw4r::ef::Effect* effects_0x080; /* its first handle */
    /* +0x084 */ u8 pad_0x084[0xD0 - 0x84];
    /* +0x0D0 */ s32 effect_id_0x0D0;         /* the effect id the state-1 handler spawns */
    /* +0x0D4 */ s32 param_0x0D4;             /* its spawn parameter */
    /* +0x0D8 */ u8 pad_0x0D8[0x8D0 - 0xD8];
    /* +0x8D0 */ s32 field_0x8D0;             /* cleared with the counts on every spawn */
    /* +0x8D4 */ u8 flag_0x8D4;               /* per-entry flag the map/area switch sets */
    /* +0x8D5 */ u8 pad_0x8D5[0x8F4 - 0x8D5];
}; /* size: 0x8F4 */

/* ---------------------------------------------------------------------------------------------------
 * externs - the plain helpers, the mangled callees and the sibling's handlers
 * ------------------------------------------------------------------------------------------------- */

/* The map's plain `.text` names carry C linkage.  Their module is undecided (the address band between
 * `sound/fn_800E46E8` and `ef/eft002` names different modules), so they are declared here as the
 * documented rule-2 gap the shared band header leaves. */
extern "C" void* eft_res_slot_get(u32 work_size);
extern "C" void eft_res_slot_release(void* self);
extern "C" u8 quest_id_head_ck(void);

/* The mangled callees, called through their real signatures (rule 9). */
u8 get_now_mapno();
u8 get_now_areano();
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);

/* The state-0 handler (this unit's own `fn_800FE978`, declared in `ef/fn_800FE978.h`) and the three handlers of
 * states 1-3 (declared in `ef/eft004.h`) the dispatcher tail-calls. */

/* This file's own two hooks, used by the spawner before their definitions. */
extern "C" void fn_800FE8E8(_EFT* self);
extern "C" void fn_800FE93C(_EFT* self);

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

/* MTX34 identity (`MTX34_ctor`), VEC3 clear (`VEC3_ctor`) - both stubs the map still spells. */

/* The nw4r::ef / engine helpers, reached through their real signatures (rule 9). */
nw4r::ef::Effect* res_eft_create(u16 id, u16 kind, u32 arg);
extern "C" nw4r::ef::Effect* fn_800F91C4(u16 id, u16 kind, s32 a, s32 b);
extern "C" void fn_800532DC(nw4r::math::MTX34* dst, nw4r::math::MTX34* src);
/* C++ callees: the target object references their manglings (cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34,
 * work_mem_alloc__FUl, work_mem_free__FPv, load_file__FPcUll, ran_suu__Fl), so no extern "C"
 * (relocaudit).  fn_802FB8EC is the reverse: its target spelling is plain, so it keeps C linkage. */
void cpSetRotMatrix(_CP_VECTOR* rot, nw4r::math::MTX34* mtx);
void setVector3(nw4r::math::VEC3* out, f32 x, f32 y, f32 z);

/* File load (`fn_800CEE2C`/`fn_804A6120`) and the work heap.  The eft004 pool helpers and `memcpy`
 * come from their owners' headers (rule 2). */
extern "C" s32 fn_804A6120(void* info);
void* work_mem_alloc(u32 size);
void work_mem_free(void* ptr);
u16 ran_suu(s32 index);

extern "C" u8 fn_802FB8EC(u8 index);
u32 LbCheckKujiraEvent();

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                              */
/* ------------------------------------------------------------------------------------------------ */

/* The 0x3C-byte file stat `fn_800CEE2C` fills; only the byte count at +0x34 is read here.  It keeps its own name
 * because `src/fn_80040598.cpp` carries the SDK spelling of the same record and this unit reads one field. */
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

namespace nw4r {

namespace db {
/* `Panic(const char* pFile, int line, const char* pFmt, ...)`. */
void Panic(const char* pFile, int line, const char* pFmt, ...);
}
}

/* The emitter `fn_800A51C8` answers for; only the +0x90 position this unit copies is named.
 * size: 0x9C - lower bound, an approximation */
struct EftEmitter {
    /* +0x00 */ u8 unused_0x00[0x90];
    /* +0x90 */ nw4r::math::VEC3 pos_0x90;
};


/* ---------------------------------------------------------------------------------------------------
 * externs
 * ------------------------------------------------------------------------------------------------- */
#include "unsplit/ef_control.h" /* eft_control (rule 2: the band) */

extern "C" u32 fn_80100330(u32* p);
extern "C" void* fn_800A485C(u32 color);
extern "C" void* fn_800A60C0(void* self);
extern "C" void fn_800A4AF8(nw4r::ef::Effect* effect);
extern "C" nw4r::ef::EffectSystem* fn_800A4420(nw4r::ef::EffectSystem* system);
extern "C" EftEmitter* fn_800A51C8(void* self);

extern "C" void fn_80100024(EftEmitter* self, nw4r::math::VEC3* out);

/* The unit's own assert strings, referenced but not emitted (the pool lives in another unit). */
extern "C" const char lbl_8059C4D0[]; /* "emitter.h"                                            .data */
extern "C" const char lbl_8059C4AC[]; /* "NW4R:Failed assertion trans != NULL"                 .data */
extern "C" const char lbl_8059C510[]; /* "res_emitter_ac.h"                                    .data */
extern "C" const char lbl_8059C4DC[]; /* "NW4R:Pointer Error\nmData(=%p) is not valid pointer." .data */

#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* Spawns the family's effect for the current map and area. */
extern "C" void fn_800FD864(void)
{
    _EFT* self;
    _EFT_MAP_WORK* work;
    u8 mapno;
    u32 areano;
    s32 a;
    s32 b;
    s32 c;

    self = (_EFT*)eft_res_slot_get(0x8F4);
    if (self == 0) {
        return;
    }
    work = (_EFT_MAP_WORK*)self->work_0x38;
    work->flag_0x8D4 = 0;
    mapno = (u8)get_now_mapno();
    areano = (u8)get_now_areano();
switch (mapno) {
    case 1:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                if (quest_id_head_ck() == 1) {
                    a = 0x3E;
                    b = 7;
                } else {
                    a = 0x3C;
                    b = 9;
                }
                c = 0xA;
                work->flag_0x8D4 = 1;
                break;
            case 1:
                self->type_0x02 = 1;
                a = 112;
                b = 18;
                c = 15;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 69;
                b = 6;
                c = 11;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 92;
                b = 5;
                c = 13;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 359;
                b = 7;
                c = 42;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 130;
                b = 2;
                c = 16;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 370;
                b = 6;
                c = 44;
                break;
            case 7:
                self->type_0x02 = 1;
                a = 520;
                b = 5;
                c = 53;
                work->flag_0x8D4 = 1;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 348;
                b = 11;
                c = 41;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 75;
                b = 16;
                c = 12;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 100;
                b = 3;
                c = 14;
                break;
            case 11:
                self->type_0x02 = 0;
                a = 389;
                b = 22;
                c = 45;
                break;
            case 12:
                self->type_0x02 = 1;
                a = 135;
                b = 17;
                c = 17;
                work->flag_0x8D4 = 1;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 2:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 559;
                b = 7;
                c = 57;
                break;
            case 1:
                self->type_0x02 = 1;
                a = 428;
                b = 3;
                c = 46;
                break;
            case 2:
                self->type_0x02 = 1;
                a = 438;
                b = 2;
                c = 47;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 578;
                b = 1;
                c = 59;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 450;
                b = 7;
                c = 48;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 463;
                b = 9;
                c = 49;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 527;
                b = 4;
                c = 54;
                break;
            case 7:
                self->type_0x02 = 1;
                a = 162;
                b = 2;
                c = 18;
                break;
            case 8:
                self->type_0x02 = 1;
                a = 549;
                b = 2;
                c = 55;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 572;
                b = 3;
                c = 58;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 552;
                b = 7;
                c = 56;
                break;
            case 11:
                self->type_0x02 = 0;
                a = 510;
                b = 9;
                c = 52;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 3:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 707;
                b = 25;
                c = 68;
                work->flag_0x8D4 = 0;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 757;
                b = 17;
                c = 72;
                work->flag_0x8D4 = 1;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 877;
                b = 8;
                c = 77;
                work->flag_0x8D4 = 1;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 895;
                b = 4;
                c = 78;
                work->flag_0x8D4 = 1;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 735;
                b = 11;
                c = 70;
                work->flag_0x8D4 = 1;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 797;
                b = 1;
                c = 74;
                work->flag_0x8D4 = 0;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 983;
                b = 9;
                c = 83;
                work->flag_0x8D4 = 1;
                break;
            case 7:
                self->type_0x02 = 0;
                a = 923;
                b = 8;
                c = 80;
                work->flag_0x8D4 = 0;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 969;
                b = 11;
                c = 82;
                work->flag_0x8D4 = 1;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 953;
                b = 10;
                c = 81;
                work->flag_0x8D4 = 0;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 915;
                b = 3;
                c = 79;
                work->flag_0x8D4 = 0;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 4:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 1379;
                b = 18;
                c = 102;
                break;
            case 1:
                self->type_0x02 = 1;
                a = 1397;
                b = 2;
                c = 103;
                break;
            case 2:
                self->type_0x02 = 1;
                a = 1409;
                b = 2;
                c = 104;
                break;
            case 3:
                self->type_0x02 = 1;
                a = 665;
                b = 6;
                c = 64;
                break;
            case 4:
                self->type_0x02 = 1;
                a = 1415;
                b = 12;
                c = 105;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 1427;
                b = 9;
                c = 106;
                break;
            case 6:
                self->type_0x02 = 1;
                a = 1445;
                b = 15;
                c = 107;
                break;
            case 7:
                self->type_0x02 = 1;
                a = 1460;
                b = 1;
                c = 108;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 1467;
                b = 5;
                c = 109;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 1483;
                b = 1;
                c = 110;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 5:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 812;
                b = 11;
                c = 75;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 1068;
                b = 5;
                c = 86;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 1080;
                b = 2;
                c = 87;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 1132;
                b = 10;
                c = 89;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 1170;
                b = 4;
                c = 92;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 1190;
                b = 8;
                c = 93;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 1247;
                b = 3;
                c = 96;
                break;
            case 7:
                self->type_0x02 = 0;
                a = 1333;
                b = 8;
                c = 99;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 1345;
                b = 16;
                c = 100;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 1362;
                b = 2;
                c = 101;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 782;
                b = 6;
                c = 73;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 6:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 1600;
                b = 6;
                c = 115;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 651;
                b = 3;
                c = 61;
                break;
            case 2:
                self->type_0x02 = 1;
                a = 1606;
                b = 4;
                c = 116;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 7:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 1096;
                b = 22;
                c = 88;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 2470;
                b = 2;
                c = 181;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 2473;
                b = 1;
                c = 182;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 1302;
                b = 7;
                c = 97;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 8:
        switch (areano) {
            case 1:
                self->type_0x02 = 1;
                a = 1008;
                b = 4;
                c = 84;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 9:
        switch (areano) {
            case 0:
                self->type_0x02 = 1;
                a = 1014;
                b = 4;
                c = 85;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 10:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 1720;
                b = 13;
                c = 118;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 1745;
                b = 12;
                c = 119;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 12:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                if (quest_id_head_ck() == 1) {
                    a = 1768;
                    b = 7;
                } else {
                    a = 1766;
                    b = 9;
                }
                c = 120;
                work->flag_0x8D4 = 1;
                break;
            case 1:
                self->type_0x02 = 1;
                a = 1856;
                b = 5;
                c = 129;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 1775;
                b = 7;
                c = 121;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 1782;
                b = 5;
                c = 122;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 1787;
                b = 7;
                c = 123;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 1794;
                b = 2;
                c = 124;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 1799;
                b = 6;
                c = 125;
                break;
            case 7:
                self->type_0x02 = 1;
                a = 1838;
                b = 4;
                c = 128;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 1813;
                b = 11;
                c = 126;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 1824;
                b = 11;
                c = 127;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 1861;
                b = 2;
                c = 130;
                break;
            case 11:
                self->type_0x02 = 0;
                a = 1870;
                b = 22;
                c = 131;
                break;
            case 12:
                self->type_0x02 = 1;
                a = 1892;
                b = 4;
                c = 132;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 13:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 1896;
                b = 6;
                c = 133;
                break;
            case 1:
                self->type_0x02 = 1;
                a = 1902;
                b = 4;
                c = 134;
                break;
            case 2:
                self->type_0x02 = 1;
                a = 1907;
                b = 3;
                c = 135;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 1913;
                b = 5;
                c = 136;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 1924;
                b = 8;
                c = 137;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 1939;
                b = 9;
                c = 138;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 1953;
                b = 5;
                c = 139;
                break;
            case 7:
                self->type_0x02 = 1;
                a = 1975;
                b = 1;
                c = 140;
                break;
            case 8:
                self->type_0x02 = 1;
                a = 1977;
                b = 3;
                c = 141;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 1980;
                b = 3;
                c = 142;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 1986;
                b = 7;
                c = 143;
                break;
            case 11:
                self->type_0x02 = 0;
                a = 1993;
                b = 9;
                c = 144;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 14:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 2002;
                b = 21;
                c = 145;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 2023;
                b = 9;
                c = 146;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 2040;
                b = 9;
                c = 147;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 2062;
                b = 3;
                c = 148;
                work->flag_0x8D4 = 1;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 2069;
                b = 13;
                c = 149;
                work->flag_0x8D4 = 1;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 2082;
                b = 4;
                c = 150;
                work->flag_0x8D4 = 0;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 2092;
                b = 9;
                c = 151;
                work->flag_0x8D4 = 1;
                break;
            case 7:
                self->type_0x02 = 0;
                a = 2106;
                b = 2;
                c = 152;
                work->flag_0x8D4 = 0;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 2114;
                b = 9;
                c = 153;
                work->flag_0x8D4 = 1;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 2123;
                b = 7;
                c = 154;
                work->flag_0x8D4 = 0;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 2132;
                b = 3;
                c = 155;
                work->flag_0x8D4 = 0;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 15:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 2141;
                b = 18;
                c = 157;
                break;
            case 1:
                self->type_0x02 = 1;
                a = 2159;
                b = 2;
                c = 158;
                break;
            case 2:
                self->type_0x02 = 1;
                a = 2171;
                b = 2;
                c = 159;
                break;
            case 3:
                self->type_0x02 = 1;
                a = 2174;
                b = 5;
                c = 160;
                break;
            case 4:
                self->type_0x02 = 1;
                a = 2212;
                b = 12;
                c = 162;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 2224;
                b = 9;
                c = 163;
                break;
            case 6:
                self->type_0x02 = 1;
                a = 2242;
                b = 15;
                c = 164;
                break;
            case 7:
                self->type_0x02 = 1;
                a = 2257;
                b = 1;
                c = 165;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 2261;
                b = 5;
                c = 166;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 2277;
                b = 1;
                c = 167;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 16:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 2298;
                b = 9;
                c = 168;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 2307;
                b = 5;
                c = 169;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 2318;
                b = 2;
                c = 170;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 2326;
                b = 10;
                c = 171;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 2349;
                b = 4;
                c = 172;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 2368;
                b = 8;
                c = 173;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 2384;
                b = 3;
                c = 174;
                break;
            case 7:
                self->type_0x02 = 0;
                a = 2398;
                b = 8;
                c = 175;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 2411;
                b = 16;
                c = 176;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 2429;
                b = 2;
                c = 177;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 2434;
                b = 6;
                c = 178;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 17:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 2498;
                b = 6;
                c = 186;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 2504;
                b = 3;
                c = 187;
                break;
            case 2:
                self->type_0x02 = 1;
                a = 2516;
                b = 4;
                c = 188;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 18:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 2455;
                b = 15;
                c = 180;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 2477;
                b = 2;
                c = 183;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 2480;
                b = 1;
                c = 184;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 2484;
                b = 7;
                c = 185;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 19:
        switch (areano) {
            case 1:
                self->type_0x02 = 1;
                a = 2441;
                b = 4;
                c = 179;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 21:
        self->field_0x07 = 1;
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                b = 4;
                a = 603;
                c = 8;
                break;
            case 1:
                self->type_0x02 = 0;
                b = 10;
                a = 3;
                c = 4;
                break;
            case 2:
                self->type_0x02 = 0;
                b = 4;
                a = 607;
                c = 9;
                break;
            case 3:
                self->type_0x02 = 0;
                b = 2;
                a = 611;
                c = 10;
                break;
            case 5:
                self->type_0x02 = 0;
                b = 1;
                a = 613;
                c = 11;
                break;
            case 6:
                self->type_0x02 = 0;
                b = 2;
                a = 614;
                c = 12;
                break;
            case 7:
                self->type_0x02 = 0;
                b = 1;
                a = 616;
                c = 13;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
    case 22:
        self->field_0x07 = 1;
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                b = 18;
                a = 175;
                c = 7;
                break;
            case 1:
                self->type_0x02 = 0;
                b = 10;
                a = 1048;
                c = 23;
                break;
            case 2:
                self->type_0x02 = 0;
                b = 6;
                a = 1019;
                c = 21;
                break;
            default:
                eft_res_slot_release(self);
                return;
        }
        break;
default:
    eft_res_slot_release(self);
    return;
}
work->count_0x000 = b;
work->effect_id_0x0D0 = a;
work->param_0x0D4 = c;
work->count_0x07C = 0;
work->field_0x8D0 = 0;
self->field_0x03 = 3;
self->area_0x44 = areano;
eft_state_flags_set(self, 0, 0);
self->release_0x40 = fn_800FE8E8;
self->dispatch_0x34 = fn_800FE93C;
}

/* The `+0x40` release hook: hands the two effect-heap runs back to the pool and zeroes their counts. */
void fn_800FE8E8(_EFT* self)
{
    _EFT_MAP_WORK* work;

    work = (_EFT_MAP_WORK*)self->work_0x38;
    push_eft_effect_heap_num(&work->effects_0x004, work->count_0x000);
    work->count_0x000 = 0;
    push_eft_effect_heap_num(&work->effects_0x080, work->count_0x07C);
    work->count_0x07C = 0;
}

/* The `+0x34` state dispatcher: tail-calls the handler for the current state. */
void fn_800FE93C(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_800FE978(self);
        return;
    case 1:
        fn_800FF8D4(self);
        return;
    case 2:
        fn_800FFC98(self);
        return;
    case 3:
        fn_800FFCA8(self);
        return;
    }
}

#pragma peephole on

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

    MTX34_ctor(&mtx);
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
                    eft_res_slot_release(self);
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
                    eft_res_slot_release(self);
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

    VEC3_ctor(&self->vec_0x84);
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
    VEC3_ctor(&self->vec_0x14);
    return self;
}

#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* Bumps the effect's state index. */
extern "C" void fn_800FFC98(_EFT* effect)
{
    Eft004* self = (Eft004*)effect;
    self->state_0x05++;
}

/* Destroys an effect record. */
extern "C" void fn_800FFCA8(_EFT* effect)
{
    eft_res_slot_release(effect);
}

/* Copies one 32-bit value. */
extern "C" void fn_80100300(u32* dst, const u32* src)
{
    *dst = *src;
}

/* Returns the emitter the color is resolved through. */
extern "C" void* fn_8010030C(void* p)
{
    return fn_800A485C(fn_80100330((u32*)p));
}

/* Asserts that the value is a valid console pointer and returns it. */
extern "C" u32 fn_80100330(u32* p)
{
    if (!IsValidPointer(*p)) {
        nw4r::db::Panic(lbl_8059C510, 102, lbl_8059C4DC);
    }
    return *p;
}

/* Copies the emitter's position out and, when the emitter is live, offsets it by the current view
 * matrix's translation. Answers whether the offset was applied. */
extern "C" s32 fn_800FFF98(void* arg0, nw4r::math::VEC3* out)
{
    EftEmitter* emitter = fn_800A51C8(arg0);

    if (emitter != NULL) {
        fn_80100024(emitter, out);
        nw4r::math::MTX34* mtx = (nw4r::math::MTX34*)fn_800A60C0(arg0);
        out->x += mtx->m[0][3];
        out->y += mtx->m[1][3];
        out->z += mtx->m[2][3];
        return 1;
    }
    return 0;
}

/* Asserts the destination vector, then copies the emitter's position into it. */
extern "C" void fn_80100024(EftEmitter* self, nw4r::math::VEC3* out)
{
    if (out == NULL) {
        nw4r::db::Panic(lbl_8059C4D0, 391, lbl_8059C4AC);
    }
    copyVec3(out, &self->pos_0x90);
}

/* Retires one live effect: runs the nw4r teardown, releases it from the system and clears its slot. */
extern "C" void fn_800FFF08(Eft004* self, u32 idx)
{
    EftEffectPool* pool = (EftEffectPool*)self->work_0x38;
    nw4r::ef::EffectSystem* system = eft_control.effect_system_0x04;
    nw4r::ef::Effect** slot = &pool->effects[idx];

    if (*slot != NULL) {
        fn_800A60C0(*slot);
        fn_800A4AF8(*slot);
        (*slot)->RetireEmitterAll();
        system->RetireEffect(*slot);
        fn_800A4420(system)->virtual_0x0C();
        *slot = NULL;
    }
}
