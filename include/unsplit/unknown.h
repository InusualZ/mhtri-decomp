/*
 * Symbols whose module is UNDECIDED (docs/plan.md 6.5 rule 2).
 *
 * The registered unit bands interleave across modules, so an address whose nearest registered unit below
 * and nearest above name different modules (or, as here, sit in a section with no registered range at all)
 * has no sound `<module>.h` to move to.  Those sites are documented here rather than guessed into a wrong
 * module - a wrong module is worse than a documented unknown.
 *
 * `system_w` (.bss 0x806585E0, 0xA5C bytes, scope global) is the game's system state block.  It is read by
 * `main.cpp` (root), `fn_80040598.cpp` (root) and `enemy/fn_8014A1BC.c`, so the type they each used to
 * define locally lives here once (rule 1).  No `.bss` range is registered in splits.txt, so the address
 * band gives no module for it.
 */
#ifndef MHTRI_UNSPLIT_UNKNOWN_H
#define MHTRI_UNSPLIT_UNKNOWN_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"

struct MHchar;
struct _CP_VECTOR;
struct _PLW;
struct _ENEMY_WORK;

#ifdef __cplusplus
/* Declared, never included: these declarations only need the type by pointer, and
 * including `pl.h`/`ef.h` here would pull their full records into units that carry their own view. */
namespace nw4r { namespace ef { struct Effect; } }
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations whose module band is ambiguous (the registered units bracketing the address name
 * different modules).  Added with `Pl/fn_80262940.cpp` (proposal 80262940).  `fn_80223E54` was
 * originally here too; MAIN later registered the lobby range `lobby/fn_8021E1EC.cpp`
 * (0x8021E1EC-0x80224AC4) over it, so its declaration moved to that owner's header (rule 2). */
s32 fn_8035B700(s32 a, s32 b, u16 c); /* 0x8035B700 - bracket: hud below, Network above */


/*
 * The game/system state block (`system_w`, 0xA5C B in retail).  Union of the three private copies:
 *   - `main.cpp`: the largest view, naming the bytes it polls at +0x01, +0x08 and +0x863..+0x931;
 *   - `fn_80040598.cpp`: the keyboard entry points at +0x8E8..+0x908;
 *   - `enemy/fn_8014A1BC.c`: the +0x0C word.
 *
 * Disagreement found: the copies disagree on the extent, not on any field's offset.  `fn_80040598.cpp`'s
 * copy stops at +0x90C and `enemy/fn_8014A1BC.c`'s at +0x10; the retail object is 0xA5C and `main.cpp`
 * zeroes all 0xA5C, so the size here is 0xA5C and the trailing bytes are padding.  Field offsets are
 * identical in every copy; the union below reproduces them.
 * size: 0xA5C
 */

/* One 0x38-byte slot of `system_w`'s task table at +0xBC (fn_800D0568/FDB0 index it by
 * `index * 0x38`); only the +0x00 live flag and the +0x04 body are touched by this unit. */
typedef struct SysTask {
    /* +0x00 */ u8 active;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ u8 body[0x20];
    /* +0x24 */ u32 field_0x24;
    /* +0x28 */ u8 pad_0x28[0x10];
} SysTask; /* size: 0x38 */

/* The four-byte record at +0x7BC: fn_800CFBE0 reads it whole, fn_800CFBA0 sets its top byte. */
typedef union SysWord {
    u32 value;
    u8 bytes[4];
} SysWord; /* size: 0x04 */

/* `system_w`'s move-work record: seven u16 maxima followed by seven pointers (work_mem_alloc(0x2C)
 * builds it; the get_move_work and set_move_work_max accessors index it). */
typedef struct MoveWork {
    /* +0x00 */ u16 max[7];
    /* +0x0E */ u8 pad_0x0e[0x2];
    /* +0x10 */ void* addr[7];
} MoveWork; /* size: 0x2C */

typedef struct SystemWork {
    /* +0x000 */ u8 pad_0x00[0x1];
    /* +0x001 */ u8 unk1;
    /* +0x002 */ u8 pad_0x02[0x3];
    /* +0x005 */ u8 field_0x05;
    /* +0x006 */ u8 pad_0x06[0x2];
    /* +0x008 */ u8 unk8;
    /* +0x009 */ u8 field_0x09; /* message-language index selecting the 0x8060B1A8/B200/B2F0 tables */
    /* +0x00A */ u8 pad_0x0A[0x2];
    /* +0x00C */ u32 field_0x0c;
    /* +0x010 */ u8 pad_0x10[0x8];
    /* +0x018 */ u16 field_0x18[4];   /* ran_suu's u16 ring, cells +0x18/+0x1A/+0x1C/+0x1E */
    /* +0x020 */ u8 field_0x20;
    /* +0x021 */ u8 field_0x21;
    /* +0x022 */ u8 field_0x22;      /* fn_800CFD20: < 2 selects the C64 path */
    /* +0x023 */ u8 pad_0x23[0x1];
    /* +0x024 */ u8 game_mode;        /* GameMode_set/GameMode_ck (values < 4) */
    /* +0x025 */ u8 play_mode;        /* PlayMode_set/PlayMode_ck (values < 7) */
    /* +0x026 */ u8 field_0x26;
    /* +0x027 */ u8 field_0x27;
    /* +0x028 */ u8 field_0x28;
    /* +0x029 */ u8 field_0x29;
    /* +0x02A */ u8 pad_0x2a[0x2];
    /* +0x02C */ u8 field_0x2c;      /* fn_800D2108: 1 or 2, the hbm state */
    /* +0x02D */ u8 pad_0x2d[0x3];
    /* +0x030 */ u8 field_0x30;
    /* +0x031 */ u8 field_0x31;
    /* +0x032 */ u8 field_0x32;
    /* +0x033 */ u8 field_0x33[4]; /* per-channel WPAD motor state */
    /* +0x037 */ u8 field_0x37[4]; /* per-channel WPAD motor timer */
    /* +0x03B */ u8 pad_0x3b[0x11];
    /* +0x04C */ u32 field_0x4c;
    /* +0x050 */ u8 pad_0x50[0x10];
    /* +0x060 */ u32 field_0x60;      /* MEMCreateExpHeapEx base */
    /* +0x064 */ u32 field_0x64;      /* MEMCreateExpHeapEx size */
    /* +0x068 */ u32 field_0x68;
    /* +0x06C */ u32 field_0x6c;
    /* +0x070 */ u32 field_0x70;
    /* +0x074 */ u32 field_0x74;
    /* +0x078 */ u8 pad_0x78[0x18];
    /* +0x090 */ void* field_0x90;    /* the stream handle fn_800CF61C reads */
    /* +0x094 */ u8 mem_allocator[0x10]; /* MEMFreeToAllocator's allocator record */
    /* +0x0A4 */ MoveWork* field_0xa4; /* the move-work record work_mem_alloc builds */
    /* +0x0A8 */ u8 pad_0xa8[0x14];
    /* +0x0BC */ SysTask tasks[0x20];  /* fn_800D0568/FDB0's 0x38-stride table, ends at +0x7BC */
    /* +0x7BC */ SysWord field_0x7bc;
    /* +0x7C0 */ u8 pad_0x7c0[0xC];
    /* +0x7CC */ u8 field_0x7cc;
    /* +0x7CD */ u8 field_0x7cd;
    /* +0x7CE */ u8 pad_0x7ce[0x5];
    /* +0x7D3 */ u8 field_0x7d3;    /* fn_800D0708: == 1 */
    /* +0x7D4 */ u8 pad_0x7d4[0x1];
    /* +0x7D5 */ u8 field_0x7d5;
    /* +0x7D6 */ u8 pad_0x7d6[0x6];
    /* +0x7DC */ u8 field_0x7dc[4]; /* per-channel motor-on state */
    /* +0x7E0 */ u8 field_0x7e0[4];
    /* +0x7E4 */ u8 field_0x7e4[4];
    /* +0x7E8 */ u8 pad_0x7e8[0x7B];
    /* +0x863 */ u8 unk2147;
    /* +0x864 */ u8 unk2148;
    /* +0x865 */ u8 unk2149;
    /* +0x866 */ u8 field_0x866;
    /* +0x867 */ u8 field_0x867;      /* TPLtexLoad group: loading display flag */
    /* +0x868 */ u8 field_0x868;
    /* +0x869 */ u8 pad_0x869[0x2];
    /* +0x86B */ u8 field_0x86b;      /* hbm_enable: == 1 keeps the menu suppressed */
    /* +0x86C */ u8 pad_0x86c[0x2];
    /* +0x86E */ u8 unk2158;
    /* +0x86F */ u8 unk2159;
    /* +0x870 */ u8 hbm_disabled;     /* hbm_disable sets 1; hbm_enable follows +0x86B */
    /* +0x871 */ u8 unk2161;
    /* +0x872 */ u8 pad_0x872[0x5];
    /* +0x877 */ u8 field_0x877;
    /* +0x878 */ u32 field_0x878;
    /* +0x87C */ u32 field_0x87c;
    /* +0x880 */ u8 pad_0x880[0x4];
    /* +0x884 */ u8 field_0x884;
    /* +0x885 */ u8 pad_0x885[0x1];
    /* +0x886 */ u8 field_0x886;
    /* +0x887 */ u8 pad_0x887[0x9];
    /* +0x890 */ u32 field_0x890;
    /* +0x894 */ u32 field_0x894;
    /* +0x898 */ u8 pad_0x898[0x3C];
    /* +0x8D4 */ void (*field_0x8d4)(void);
    /* +0x8D8 */ u32 (*unk2264)(void);
    /* +0x8DC */ void (*field_0x8dc)(void);
    /* +0x8E0 */ void (*unk2272)(void);
    /* +0x8E4 */ void (*unk2276)(void);
    /* +0x8E8 */ void (*kbd_init)(u8);
    /* +0x8EC */ int (*kbd_open)(u8);
    /* +0x8F0 */ int (*kbd_move)(void);
    /* +0x8F4 */ void (*set_kbd_param)(char*, u32);
    /* +0x8F8 */ u8 (*get_kbd_setup_type)(void);
    /* +0x8FC */ void (*kbd_reset)(void);
    /* +0x900 */ int (*kbd_close)(void);
    /* +0x904 */ void (*kbd_exit)(void);
    /* +0x908 */ int (*kbd_input)(void);
    /* +0x90C */ u8 pad_0x90C[0x25];
    /* +0x931 */ u8 unk2353;
    /* +0x932 */ u8 pad_0x932[0x16];
    /* +0x948 */ void* field_0x948;    /* work-heap base, cleared by fn_800CE5B4 */
    /* +0x94C */ u8 pad_0x94c[0x108];
    /* +0xA54 */ void (*field_0xa54)(s32, s32);
    /* +0xA58 */ u8 field_0xa58;      /* pmic_disp_off clears */
    /* +0xA59 */ u8 pad_0xa59[0x3];
} SystemWork;

extern SystemWork system_w;

/* The unowned C helpers `enemy/fn_801A4504.cpp` calls: addresses whose bracketing registered units
 * name different modules (0x802B/0x803), so no `<module>.h` is sound - the rule 2 named gap.  The
 * signatures are the call sites' (r3 the work record; `fn_80304510`'s fifth argument is the s32
 * `0`/0xF4A0/0xB61 the target materialises; `fn_803B50A8` returns the r3 word compared against 1). */
u8 fn_802B0668(u8 kind);
void fn_802BE638(struct _ENEMY_WORK* self, s32 a, Vec3* v);
void fn_80304510(struct _ENEMY_WORK* self, u32 a, u32 b, void* v, s32 d, f32 s);
void fn_80306A98(struct _ENEMY_WORK* self, u32 a);
u32 fn_803B50A8(void);
void fn_803B993C(s32 handle, Vec3* v, u8 area);


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
#ifdef __cplusplus
}
#endif

/* The target objects reference these by their C++ manglings (`get_now_areano__Fv`,
 * `get_stg_eft_col__FUcUc`, `cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34`, ...), so they are
 * declared with C++ linkage.  The one remaining C consumer (`sound/fn_800DCFEC.c`) keeps the plain C
 * declarations below until its own language is resolved (that unit is a follow-up). */
#ifdef __cplusplus
u8 get_now_areano();
u8 get_now_mapno();
void* res_eft_model_create(struct MHchar* chr, u16 id, u32 arg);
void* res_eft_model_create_light(struct MHchar* chr, u16 id, u32 a, long b);
u32 get_stg_eft_col(u8 area, u8 which);
u8 eftGetKeyAlpha(u8* key, long frame);
void eftGetKeyRGB(u8* keys, long frame, u8* r, u8* g, u8* b);
u16 Get_motion_no(struct _PLW* plw);
void cpSetRotMatrix(struct _CP_VECTOR* rot, Mtx34* mtx);
#else
u8 get_now_areano();
u8 get_now_mapno();
void* res_eft_model_create(struct MHchar* chr, u16 id, u32 arg);
void* res_eft_model_create_light(struct MHchar* chr, u16 id, u32 a, long b);
u32 get_stg_eft_col(u8 area, u8 which);
u8 eftGetKeyAlpha(u8* key, long frame);
void eftGetKeyRGB(u8* keys, long frame, u8* r, u8* g, u8* b);
u16 Get_motion_no(struct _PLW* plw);
void cpSetRotMatrix(struct _CP_VECTOR* rot, Mtx34* mtx);
#endif

#ifdef __cplusplus /* C++-only: outside the extern "C" block, so C++ linkage is kept */
void SetRootMtxTrans(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos);
u32 effect_move(nw4r::ef::Effect* effect);
void change_paramscale_eff(nw4r::ef::Effect* effect, f32 scale);
void change_paramscale_eff_vec3(nw4r::ef::Effect* effect, nw4r::math::VEC3* vec);
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);
void change_color_eff(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos, _GXColor color);
nw4r::ef::Effect* res_eft_create(u16 id, u16 param, u32 idx);
#endif

#endif /* MHTRI_UNSPLIT_UNKNOWN_H */
