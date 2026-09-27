/*
 * camera/fn_802B5C58.cpp - the game camera work block and its controller.
 *
 * `.text` 0x802B5C58-0x802BEAAC (132 functions, 36436 B).  Registered from
 * `proposal/802B5C58_fn_802B5C58.cpp`.
 *
 * Module `camera` (brief section 2 class 3: what the code does, plus the neighbours' scheme).  The
 * range owns four real map names, all camera entry points - `get_current_view_mtx` (0x802BDC40),
 * `get_camera_pos` (0x802BDCE0), `get_camera_direction` (0x802BDE14), `set_quake_sub` (0x802BE4B0) -
 * and the rest drives the camera: `nw4r::g3d::Camera::SetPerspective`, `cpSetRotMatrix`, and
 * `Pl_act_ck`/`Pl_frame_check`, because the camera tracks the player's action state.  No `__FILE__`
 * string covers the range (it references no `.rodata` at all) and the shared runtime dump answers only
 * `zz_XXXXXXXX_` for everything but those four names, so the file keeps the map's stem.
 *
 * The camera work.  `fn_802BECD0` (0x802BECD0, `light/light.cpp`'s first function) returns
 * `lbl_806BB7E0` or `lbl_806BB7E0 + 1272`; `lbl_806BB7E0` is a 0x9F0-byte `.bss` object, i.e. two
 * 0x4F8 `CamWork` slots - that is where `CamWork`'s size comes from.  This unit's functions take that
 * pointer as `self`, or call the accessor for it through this unit's own name for the record
 * (`(CamWork*)fn_802BECD0()`: the owner views the same 0x4F8 bytes as `LightWork` and declares only
 * that spelling - `include/light/light.h`; folding the two views into one definition is the rule-1
 * pass `include/unsplit/camera.h` records), and touch fields up to `+0x4F7`.
 *
 * The seam at 0x802B5C58 is pinned by a `.sdata2` pool jump (`lbl_8079A514` -> `lbl_8079A530`);
 * `tudiscover` reports `MATCH SET 0x802B5C58..0x802BEAAC` (132 functions, 5 must-link anchors).  The
 * right edge is the proposal's `--max-bytes` cap, not a strong seam: tudiscover's best right-side
 * candidates are weak (`0x802BF278`/`0x802BF284`, codegen fingerprint + call closure), so the extent
 * may settle a few functions wider once those match.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x802B5C58 0x802BEAAC --limit 500`, which lists exactly the
 * four camera names above as non-`fn_` entries).
 *
 * Flags.  The file builds with `#pragma peephole off`: retail keeps the unfused form of several folds
 * (`fn_802BC1CC`'s `rlwinm` + branchless `!= 0` where our peephole produces one `extrwi`; the
 * `clrlwi`-before-`add` index masks of `fn_802B81A8`/`fn_802B81C0`/`fn_802BC070`), which is the same
 * finding the sibling unit `stage/fn_802B2978.c` records for the band below this one.  Measured on
 * this unit's symbols, peephole off took the fully matching rows from 19 to 25 of the then-written 37.
 * `fn_802BB0D4` additionally needs `#pragma fp_contract off` (playbook 40): retail keeps `fmuls` +
 * `fadds` where the default contract fuses them into one `fmadds`.
 *
 * Residuals.  The range is reconstructed in address order; the functions not yet in this file are
 * unwritten, and their inventory is `config/RMHE08/symbols.txt` (their target objects are
 * `build/RMHE08/obj/`).  Everything written is byte-identical except `get_camera_pos`, whose residual
 * is on the function (and is shared by the whole accessor family: `get_current_view_mtx`,
 * `get_camera_direction`, `fn_802BDC90`, `fn_802BDDB0`, `fn_802BDE90`, `fn_802BDFC0`, `fn_802BDFFC`,
 * `fn_802BE088`, `fn_802BE1DC`, `fn_802BE1EC`, `fn_802BE0F4` - they all start from `fn_80047398()`,
 * copy the handle through `fn_8004723C` and read a field, and all need MWCC to alias their returned
 * local to the `sret` pointer, which the spellings tried so far do not achieve).
 *
 * Shared-file cost.  Three declarations this unit needs are not in their owner's headers yet, so they
 * were added there in this branch (each an addition to an existing `extern "C"` block):
 * `include/mh3_pad.h` (`fn_8004723C`, owner `src/mh3_pad.cpp`), `include/fn_80047398.h`
 * (`fn_80047398`, owner `src/fn_80047398.cpp`) and `include/g3d/g3d_camera.h` (`fn_800749C8`, owner
 * `src/g3d/g3d_camera.cpp`).
 */

#include "types.h"
#include "nw4r/math.h"

#include "fn_80047398.h"
#include "g3d/g3d_camera.h"
#include "mh3_pad.h"
#include "Pl/pl_master.h"
#include "camera/camera.h"
#include "unsplit/camera.h"

/* The low half of `CamWork`'s +0x040 word, which the camera also views as four flag bytes. */
typedef struct CamFlagWord {
    /* +0x00 */ u8 pad_0x00;
    /* +0x01 */ u8 field_0x041;             /* invalid flag (bit 4); fn_802BD5CC tests it */
    /* +0x02 */ u8 field_0x042;             /* armed flag, set by fn_802BAB0C */
    /* +0x03 */ u8 field_0x043;             /* latched by fn_802BAB54 */
} CamFlagWord; /* size: 0x04 */

/* `CamWork`'s +0x040 word: a float `fn_802B7980` compares against a threshold, and the flag bytes the
 * follow and target paths latch.  Anchored at the word-aligned +0x040; both arms are 4 bytes, so the
 * union keeps the field size and the struct's later offsets. */
typedef union CamWord040 {
    /* +0x040 */ f32 value;
    /* +0x040 */ CamFlagWord flags;
} CamWord040; /* size: 0x04 */

/* One quake slot: CamWork carries three of them, at +0x4A8 / +0x4BC / +0x4D0 (the `cam + 1192`,
 * `+ 1212`, `+ 1232` of fn_802BE4FC / fn_802BE568 / fn_802BE714 / fn_802BE77C). size: 0x14 */
typedef struct CamQuake {
    /* +0x00 */ nw4r::math::VEC3 vec_0x00;   /* origin, or direction */
    /* +0x0C */ u8 active_0x0C;              /* set by every arm */
    /* +0x0D */ u8 kind_0x0D;
    /* +0x0E */ s16 timer_0x0E;              /* from lbl_805D1E7C[kind & 7] */
    /* +0x10 */ u8 flag_0x10;
    /* +0x11 */ u8 pad_0x11[0x3];
} CamQuake; /* size: 0x14 */

/* The caller record fn_802BE568 / fn_802BE714 read a vector at +0x3C from; the callers pass a player
 * work record, and fn_8026FD94 is the only other thing this unit asks of it. size: >= 0x48 */
typedef struct CamWorkSrc {
    /* +0x00 */ u8 pad_0x00[0x3C];
    /* +0x3C */ nw4r::math::VEC3 vec_0x3C;
} CamWorkSrc; /* size: 0x48 (a lower bound, and marked as one: the caller's record is a player
              * work block, and only its +0x3C vector is read here) */

/*
 * The camera work block: 0x4F8 bytes, from `lbl_806BB7E0` (a 0x9F0-byte `.bss` object = two slots) and
 * the accessor's `base + 1272`.  Only the fields this unit touches are named; untouched runs keep
 * their offset as padding.
 *
 * `fn_802BECD0` is declared by its owner (`include/light/light.h`) over the same bytes, under that
 * unit's own view name `LightWork`; this unit's name for the record is `CamWork`, so every read of
 * the accessor goes through a cast to it - a type adaptation, no instruction (rule 1's two-views
 * residual, not a rename of either view).
 */
typedef struct CamWork {
    /* +0x000 */ u8 pad_0x000[0x9];
    /* +0x009 */ u8 field_0x009;             /* mode byte `fn_802B7980` compares against 3 */
    /* +0x00A */ u8 pad_0x00A[0x36];
    /* +0x040 */ CamWord040 word_0x040;
    /* +0x044 */ s16 field_0x044;            /* fn_802BAB0C stores `arg - 1` */
    /* +0x046 */ u8 pad_0x046[0x2];
    /* +0x048 */ f32 field_0x048;            /* fn_802BAB0C: a constant divided by the argument */
    /* +0x04C */ u8 pad_0x04C[0xA];
    /* +0x056 */ u8 field_0x056;             /* gate read by fn_802BAB54 */
    /* +0x057 */ u8 pad_0x057;
    /* +0x058 */ void* field_0x058;          /* record fn_802BAB54 reads a byte from at +0x48 */
    /* +0x05C */ u8 pad_0x05C[0x18];
    /* +0x074 */ u8 field_0x074;             /* fn_802B7980's second gate */
    /* +0x075 */ u8 pad_0x075[0x7];
    /* +0x07C */ void* field_0x07C;          /* 0x24-byte record list fn_802BD894 walks */
    /* +0x080 */ u8 pad_0x080[0xC];
    /* +0x08C */ void* field_0x08C;          /* the camera's target record */
    /* +0x090 */ u8 pad_0x090[0x2];
    /* +0x092 */ u8 field_0x092;             /* raised by fn_802B8B8C */
    /* +0x093 */ u8 pad_0x093[0x9];
    /* +0x09C */ u8 field_0x09C;             /* returned by fn_802B8B68 */
    /* +0x09D */ u8 pad_0x09D[0x19];
    /* +0x0B6 */ u8 field_0x0B6;             /* returned by fn_802B8E48 */
    /* +0x0B7 */ u8 pad_0x0B7[0x49];
    /* +0x100 */ u8 taken_0x100[0x2];        /* fn_802B81A8 tests, fn_802B81C0 sets */
    /* +0x102 */ u8 field_0x102[0xA];        /* fn_802B81C0 stores its value argument here */
    /* +0x10C */ u8 field_0x10C[0x8];        /* fn_802B81C0 clears one byte here */
    /* +0x114 */ u8 pad_0x114[0x4];
    /* +0x118 */ s8 field_0x118;             /* fn_802B9574 sets -1, fn_802B95B4 tests > 0 */
    /* +0x119 */ u8 pad_0x119[0x2C];
    /* +0x145 */ u8 field_0x145;             /* fn_802B954C raises it */
    /* +0x146 */ u8 pad_0x146[0x36];
    /* +0x17C */ f32 field_0x17C;            /* fn_802B9740 subtracts field_0x1D0 from it */
    /* +0x180 */ u8 pad_0x180[0x50];
    /* +0x1D0 */ f32 field_0x1D0;
    /* +0x1D4 */ u8 pad_0x1D4[0x4];
    /* +0x1D8 */ f32 field_0x1D8;            /* fn_802B9740's scale factor */
    /* +0x1DC */ u8 pad_0x1DC[0x10];
    /* +0x1EC */ u8 field_0x1EC;             /* returned by fn_802B9740 */
    /* +0x1ED */ u8 field_0x1ED;             /* fn_802B9740 copies it out through a parameter */
    /* +0x1EE */ u8 field_0x1EE;             /* fn_802B9740 gates on it */
    /* +0x1EF */ u8 pad_0x1EF[0x95];
    /* +0x284 */ u8 field_0x284;             /* camera_work_ck tests it against 1 */
    /* +0x285 */ u8 field_0x285;             /* cleared by fn_802BC468 */
    /* +0x286 */ u8 field_0x286;
    /* +0x287 */ u8 field_0x287;
    /* +0x288 */ s16 field_0x288;            /* fn_802BC82C returns it, or one less */
    /* +0x28A */ u8 pad_0x28A[0x44];
    /* +0x2CE */ u8 field_0x2CE;
    /* +0x2CF */ u8 field_0x2CF;
    /* +0x2D0 */ u32 field_0x2D0;
    /* +0x2D4 */ u8 pad_0x2D4[0x6D];
    /* +0x341 */ u8 field_0x341;
    /* +0x342 */ u8 pad_0x342[0x2];
    /* +0x344 */ s16 field_0x344;            /* returned by fn_802BC878 */
    /* +0x346 */ u8 field_0x346;
    /* +0x347 */ u8 pad_0x347[0x41];
    /* +0x388 */ u8 field_0x388;             /* fn_802BE41C tests it against 1 */
    /* +0x389 */ u8 pad_0x389[0x3F];
    /* +0x3C8 */ u8 field_0x3C8;             /* raised by fn_802B8B8C */
    /* +0x3C9 */ u8 pad_0x3C9[0x97];
    /* +0x460 */ u8 field_0x460;             /* returned by fn_802BBA94 */
    /* +0x461 */ u8 pad_0x461[0x19];
    /* +0x47A */ u8 field_0x47A;             /* fn_802BBA3C clears it */
    /* +0x47B */ u8 pad_0x47B[0x3];
    /* +0x47E */ u8 field_0x47E;             /* fn_802BBA64 stores its argument here */
    /* +0x47F */ u8 field_0x47F;             /* fn_802BBAC4 stores its argument here */
    /* +0x480 */ u8 pad_0x480[0x19];
    /* +0x499 */ u8 field_0x499;             /* the camera slot fn_802BB0EC selects */
    /* +0x49A */ u8 pad_0x49A;
    /* +0x49B */ u8 field_0x49B;             /* set to 0xFF by fn_802B8B8C */
    /* +0x49C */ u8 pad_0x49C[0x2];
    /* +0x49E */ u16 field_0x49E;            /* fn_802B700C clears the five below */
    /* +0x4A0 */ u16 field_0x4A0;
    /* +0x4A2 */ u16 field_0x4A2;
    /* +0x4A4 */ u16 field_0x4A4;
    /* +0x4A6 */ u16 field_0x4A6;
    /* +0x4A8 */ CamQuake quake_0x4A8;
    /* +0x4BC */ CamQuake quake_0x4BC;
    /* +0x4D0 */ CamQuake quake_0x4D0;
    /* +0x4E4 */ u8 pad_0x4E4[0x8];
    /* +0x4EC */ u32 field_0x4EC;            /* fn_802BE060 returns its low half */
    /* +0x4F0 */ u32 field_0x4F0;            /* fn_802BE038 returns its low half */
    /* +0x4F4 */ u8 pad_0x4F4[0x3];
    /* +0x4F7 */ u8 field_0x4F7;             /* camera-slot bit flags (fn_802BC000/70/1CC) */
} CamWork; /* size: 0x4F8 */

/* The four floats fn_802BAAE8 moves: +0x38 of its source record, +0x60 of its destination. */
typedef struct CamVec4 {
    /* +0x00 */ f32 v[4];
} CamVec4; /* size: 0x10 */

/* fn_802BAAE8's source record (0x48 bytes; only the tail is read). */
typedef struct CamMoveSrc {
    /* +0x00 */ u8 pad_0x00[0x38];
    /* +0x38 */ CamVec4 vec_0x38;
} CamMoveSrc; /* size: 0x48 */

/* fn_802BAAE8's destination record (0x70 bytes; only the tail is written). */
typedef struct CamMoveDst {
    /* +0x00 */ u8 pad_0x00[0x60];
    /* +0x60 */ CamVec4 vec_0x60;
} CamMoveDst; /* size: 0x70 */

/* The 0x24-byte record list fn_802BD894 walks; it reads the first word of each. */
typedef struct CamListEntry {
    /* +0x00 */ s32 value;
    /* +0x04 */ u8 pad_0x04[0x20];
} CamListEntry; /* size: 0x24 */

/* The camera's target record: a live flag, the two vectors `fn_802BD54C`/`fn_802BD588` copy out, and
 * the two mode bytes `fn_802BD5CC`/`fn_802BD618` test. */
typedef struct CamTarget {
    /* +0x000 */ u8 alive;
    /* +0x001 */ u8 pad_0x001[0x187];
    /* +0x188 */ nw4r::math::VEC3 vec_0x188;
    /* +0x194 */ u8 pad_0x194[0x28];
    /* +0x1BC */ u32 field_0x1BC;
    /* +0x1C0 */ u32 field_0x1C0;
    /* +0x1C4 */ u8 pad_0x1C4[0x1E];
    /* +0x1E2 */ u8 mode_0x1E2;
    /* +0x1E3 */ u8 mode_0x1E3;
} CamTarget; /* size: 0x1E4 */

/* The previous/current value pairs `fn_802BD260` latches: eight slots, four of them vectors that go
 * through `copyVec3` and the rest scalars.  The size is a lower bound - the record's real extent
 * belongs to whoever allocates it. size: >= 0xF0 */
typedef struct CamTrack {
    /* +0x000 */ u8 pad_0x000[0x4C];
    /* +0x04C */ nw4r::math::VEC3 prev_0x4C;
    /* +0x058 */ nw4r::math::VEC3 cur_0x58;
    /* +0x064 */ nw4r::math::VEC3 prev_0x64;
    /* +0x070 */ nw4r::math::VEC3 cur_0x70;
    /* +0x07C */ u8 pad_0x07C[0x2C];
    /* +0x0A8 */ u32 prev_0xA8;
    /* +0x0AC */ u32 cur_0xAC;
    /* +0x0B0 */ u32 prev_0xB0;
    /* +0x0B4 */ u32 cur_0xB4;
    /* +0x0B8 */ u8 pad_0x0B8[0x8];
    /* +0x0C0 */ s16 prev_0xC0;
    /* +0x0C2 */ s16 cur_0xC2;
    /* +0x0C4 */ u8 pad_0x0C4[0x8];
    /* +0x0CC */ s16 prev_0xCC;
    /* +0x0CE */ s16 cur_0xCE;
    /* +0x0D0 */ u8 pad_0x0D0[0x8];
    /* +0x0D8 */ u32 prev_0xD8;
    /* +0x0DC */ u32 cur_0xDC;
    /* +0x0E0 */ u8 pad_0x0E0[0x8];
    /* +0x0E8 */ u32 prev_0xE8;
    /* +0x0EC */ u32 cur_0xEC;
} CamTrack; /* size: 0xF0 (a lower bound, and marked as one: only the eight latching pairs are
             * named, so the record's real extent is not evidenced here) */

/* The record fn_802BAB54 reads its +0x48 byte from. */
typedef struct CamLead {
    /* +0x00 */ u8 pad_0x00[0x48];
    /* +0x48 */ u8 count_0x48;
} CamLead; /* size: 0x49 */

#pragma peephole off

extern "C" {

/* Defined later in this file. */
void fn_802BB870(u32 mode, u32 a, u32 b, u8 c);
void fn_802B8B8C(void);
void fn_802BC4AC(u32 kind, u32 arg);
void fn_802BB118(CamWork* self);
void fn_802BBEE0(void);

/*
 * Clears the five 16-bit records the camera carries over from the previous frame.
 */
void fn_802B700C(CamWork* self)
{
    self->field_0x49E = 0;
    self->field_0x4A0 = 0;
    self->field_0x4A2 = 0;
    self->field_0x4A4 = 0;
    self->field_0x4A6 = 0;
}

/*
 * Copies one 16-bit record.
 */
void fn_802B7028(u16* dst, const u16* src)
{
    *dst = *src;
}

/*
 * Whether the camera is in its third mode, its timer is above the threshold and its target is live.
 */
u32 fn_802B7980(CamWork* self)
{
    if (self->field_0x009 == 3 && self->word_0x040.value > lbl_8079A56C && self->field_0x074 != 0)
        return 1;
    return 0;
}

/*
 * Whether the indexed slot is still free.
 */
bool fn_802B81A8(CamWork* self, u8 index)
{
    return self->taken_0x100[index] == 0;
}

/*
 * Marks the indexed slot taken and stores its value.
 */
void fn_802B81C0(CamWork* self, u8 index, u8 value)
{
    self->taken_0x100[index] = 1;
    self->field_0x102[index] = value;
    self->field_0x10C[index] = 0;
}

/*
 * Reads the +0x9C byte of the current camera work.
 */
u8 fn_802B8B68(void)
{
    return ((CamWork*)fn_802BECD0())->field_0x09C;
}

/*
 * Reads the +0xB6 byte of the current camera work.
 */
u8 fn_802B8E48(void)
{
    return ((CamWork*)fn_802BECD0())->field_0x0B6;
}

/*
 * Raises the +0x145 flag of the current camera work.
 */
void fn_802B954C(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    self->field_0x145 = 1;
}

/*
 * Invalidates the +0x118 timer of the current camera work when `reset` is zero.
 */
void fn_802B9574(u8 reset)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    if (reset == 0)
        self->field_0x118 = -1;
}

/*
 * Whether the +0x118 timer of the current camera work is positive.
 */
bool fn_802B95B4(void)
{
    return ((CamWork*)fn_802BECD0())->field_0x118 > 0;
}

/*
 * Copies two state bytes out of the camera work, writes the timer scaled by the work's own factor, and
 * returns the mode byte.
 */
u8 fn_802B9740(void* unused, u8* out_a, u8* out_b, f32* out_timer)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    *out_a = self->field_0x1ED;
    *out_b = self->field_0x1EE;
    if (self->field_0x1EE != 0) {
        f32 t = self->field_0x17C - self->field_0x1D0;
        *out_timer = t;
        *out_timer = t * self->field_0x1D8;
    }
    return self->field_0x1EC;
}

/*
 * Copies the four floats at +0x38 of the source record into +0x60 of the destination.
 */
void fn_802BAAE8(CamMoveDst* dst, const CamMoveSrc* src)
{
    dst->vec_0x60.v[0] = src->vec_0x38.v[0];
    dst->vec_0x60.v[1] = src->vec_0x38.v[1];
    dst->vec_0x60.v[2] = src->vec_0x38.v[2];
    dst->vec_0x60.v[3] = src->vec_0x38.v[3];
}

/*
 * The interval from the argument, as the constant at lbl_8079A4EC divided by it.
 */
void fn_802BAB0C(CamWork* self, s32 value)
{
    self->word_0x040.flags.field_0x042 = 1;
    self->field_0x044 = (s16)(value - 1);
    self->field_0x048 = lbl_8079A4EC / (f32)value;
}

/*
 * Starts the camera follow when the lead record has a positive count, and latches +0x43 when the
 * camera is already fading.
 */
void fn_802BAB54(CamWork* self)
{
    s32 count;

    if (self->field_0x056 == 0)
        return;
    count = 15;
    if (self->field_0x058 != NULL)
        count = ((CamLead*)self->field_0x058)->count_0x48;
    if (count < 1) {
        self->word_0x040.flags.field_0x042 = 0;
        return;
    }
    if (self->word_0x040.flags.field_0x042 == 2) {
        self->word_0x040.flags.field_0x043 = 1;
        return;
    }
    fn_802BAB0C(self, count);
}

/*
 * Blends two values by `t`.  Retail keeps `a * t` and `b * (c - t)` as separate multiplies and
 * adds, so this function builds with the contract pass off (playbook 40).
 */
#pragma fp_contract off
f32 fn_802BB0D4(f32 a, f32 b, f32 t)
{
    f32 other = lbl_8079A4EC - t;

    return a * t + b * other;
}
#pragma fp_contract on

/*
 * Arms the current camera work through the `fn_802BB870` entry point with no arguments.
 */
void fn_802BBA2C(u32 mode)
{
    fn_802BB870(mode, 0, 0, 0);
}

/*
 * Clears the mode flag of the current camera work.
 */
void fn_802BBA3C(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    self->field_0x47A = 0;
}

/*
 * Stores the +0x47E byte of the current camera work.
 */
void fn_802BBA64(u8 value)
{
    ((CamWork*)fn_802BECD0())->field_0x47E = value;
}

/*
 * Reads the +0x460 byte of the current camera work.
 */
u8 fn_802BBA94(void)
{
    return ((CamWork*)fn_802BECD0())->field_0x460;
}

/*
 * Clears the mode flag with no argument.
 */
void fn_802BBAC0(void)
{
    fn_802BBA3C();
}

/*
 * Stores the +0x47F byte of the current camera work.
 */
void fn_802BBAC4(u8 value)
{
    ((CamWork*)fn_802BECD0())->field_0x47F = value;
}

/*
 * Arms the camera work with mode 0.
 */
void fn_802BBAB8(void)
{
    fn_802BBA2C(0);
}

/*
 * Clears the mode flag with no argument.
 */
void fn_802BBB78(void)
{
    fn_802BBA3C();
}

/*
 * Sets or clears one camera-slot bit: with `set` == 1 the bit for `slot` is raised, otherwise bit 7 is.
 */
void fn_802BC000(u8 slot, u8 set)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    if (set == 1)
        self->field_0x4F7 |= 1 << slot;
    else
        self->field_0x4F7 |= 0x80;
}

/*
 * Whether the indexed camera-slot bit is set.
 */
bool fn_802BC070(CamWork* self, u8 slot)
{
    return (self->field_0x4F7 & (1 << slot)) != 0;
}

/*
 * Whether the high camera-slot bit is set.
 */
bool fn_802BC1CC(CamWork* self)
{
    return (self->field_0x4F7 & 0x80) != 0;
}

/*
 * Clears the camera mode bytes the quake and fade paths latch.
 */
void fn_802BC468(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    self->field_0x286 = 0;
    self->field_0x287 = 0;
    self->field_0x2CE = 0;
    self->field_0x2CF = 0;
    self->field_0x2D0 = 0;
    self->field_0x285 = 0;
    self->field_0x341 = 0;
    self->field_0x346 = 0;
}

/*
 * Returns the +0x288 counter, one lower when `full` is set.
 */
s16 fn_802BC82C(u8 full)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    if (full == 0)
        return self->field_0x288;
    return (s16)(self->field_0x288 - 1);
}

/*
 * Returns the +0x344 counter.
 */
s16 fn_802BC878(void)
{
    return ((CamWork*)fn_802BECD0())->field_0x344;
}

/*
 * Whether the camera work is usable and its target record is alive in the first mode.
 */
bool fn_802BD5CC(CamWork* self)
{
    CamTarget* target;

    if ((self->word_0x040.flags.field_0x041 & 0x10) != 0)
        return false;
    target = (CamTarget*)self->field_0x08C;
    if (target != NULL && target->alive != 0 && target->mode_0x1E2 == 1)
        return true;
    return false;
}

/*
 * Whether the target record is alive and in the second mode.
 */
bool fn_802BD618(CamWork* self)
{
    CamTarget* target = (CamTarget*)self->field_0x08C;

    if (target != NULL && target->alive != 0 && target->mode_0x1E2 == 1 && target->mode_0x1E3 == 2)
        return true;
    return false;
}

/*
 * Returns half of the first value of the record at +0x7C, or zero when the list is empty.
 */
s16 fn_802BD894(CamWork* self)
{
    CamListEntry* entry = (CamListEntry*)self->field_0x07C;
    s32 value = 0;

    if (entry->value >= 0) {
        while (entry->value >= 0)
            entry++;
        value = entry[-1].value;
    }
    return (s16)(value / 2);
}

/*
 * The 16-bit difference of two values, shifted right.
 */
s32 fn_802BDC2C(s16 a, s16 b, u8 shift)
{
    return (s16)(a - b) >> shift;
}

/*
 * Returns the low half of the +0x4EC word.
 */
u32 fn_802BE060(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    return (u16)self->field_0x4EC;
}

/*
 * Returns the low half of the +0x4F0 word.
 */
u32 fn_802BE038(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    return (u16)self->field_0x4F0;
}

/*
 * Whether the +0x284 byte holds 1.
 */
bool camera_work_ck(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    return self->field_0x284 == 1;
}

/*
 * Whether the +0x388 byte holds 1.
 */
bool fn_802BE41C(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    return self->field_0x388 == 1;
}


/*
 * Copies the target record's +0x1BC word and +0x188 vector out, or raises the camera's invalid flag
 * when there is no live target.
 */
void fn_802BD54C(CamWork* self, u32* out, nw4r::math::VEC3* vec)
{
    CamTarget* target = (CamTarget*)self->field_0x08C;

    if (target != NULL && target->alive != 0) {
        *out = target->field_0x1C0;
        copyVec3(vec, &target->vec_0x188);
        return;
    }
    self->word_0x040.flags.field_0x041 |= 1;
}

/*
 * Copies the target record's +0x1BC and +0x1C0 words and its +0x188 vector out, or raises the camera's
 * invalid flag when there is no live target.
 */
void fn_802BD588(CamWork* self, u32* out_a, u32* out_b, nw4r::math::VEC3* vec)
{
    CamTarget* target = (CamTarget*)self->field_0x08C;

    if (target != NULL && target->alive != 0) {
        *out_a = target->field_0x1BC;
        *out_b = target->field_0x1C0;
        copyVec3(vec, &target->vec_0x188);
        return;
    }
    self->word_0x040.flags.field_0x041 |= 1;
}

/*
 * Latches the current value of every slot selected by `mask` into its previous value.
 */
void fn_802BD260(CamTrack* self, u32 mask)
{
    if ((mask & 1) != 0)
        copyVec3(&self->cur_0x58, &self->prev_0x4C);
    if ((mask & 2) != 0)
        copyVec3(&self->cur_0x70, &self->prev_0x64);
    if ((mask & 4) != 0)
        self->cur_0xAC = self->prev_0xA8;
    if ((mask & 8) != 0)
        self->cur_0xB4 = self->prev_0xB0;
    if ((mask & 0x10) != 0)
        self->cur_0xC2 = self->prev_0xC0;
    if ((mask & 0x20) != 0)
        self->cur_0xCE = self->prev_0xCC;
    if ((mask & 0x40) != 0)
        self->cur_0xDC = self->prev_0xD8;
    if ((mask & 0x80) != 0)
        self->cur_0xEC = self->prev_0xE8;
}

/*
 * Arms one quake slot: the origin vector, the kind and the duration `lbl_805D1E7C` gives it.
 */
void fn_802BE44C(CamQuake* slot, const nw4r::math::VEC3* origin, u8 kind, u8 flag)
{
    slot->active_0x0C = 1;
    slot->kind_0x0D = kind;
    slot->timer_0x0E = lbl_805D1E7C[kind & 0x1F];
    copyVec3(&slot->vec_0x00, origin);
    slot->flag_0x10 = flag;
}

/*
 * Starts the first quake slot from the zero vector, with the kind's high bit set.
 */
void fn_802BE4FC(u8 kind)
{
    CamWork* self = (CamWork*)fn_802BECD0();
    nw4r::math::VEC3 origin;

    VEC3_ctor(&origin);
    setVector3(&origin, lbl_8079A4D8, lbl_8079A4D8, lbl_8079A4D8);
    fn_802BE44C(&self->quake_0x4A8, &origin, (u8)(kind | 0x80), 0);
}

/*
 * Starts the first quake slot from the work record's +0x3C vector when the player has a live action.
 */
void fn_802BE568(CamWorkSrc* work, u8 kind)
{
    CamQuake* slot = &((CamWork*)fn_802BECD0())->quake_0x4A8;

    if (fn_8026FD94((_PLW*)work) != 0)
        fn_802BE44C(slot, &work->vec_0x3C, kind, 0);
}

/*
 * Starts the second quake slot from the work record's +0x3C vector when the player has a live action.
 */
void fn_802BE714(CamWorkSrc* work, u8 kind)
{
    CamQuake* slot = &((CamWork*)fn_802BECD0())->quake_0x4BC;

    if (fn_8026FD94((_PLW*)work) != 0)
        fn_802BE44C(slot, &work->vec_0x3C, kind, 0);
}

/*
 * Starts the third quake slot from the zero vector, with the kind's high bits set.
 */
void fn_802BE77C(u8 kind)
{
    CamWork* self = (CamWork*)fn_802BECD0();
    nw4r::math::VEC3 origin;

    VEC3_ctor(&origin);
    setVector3(&origin, lbl_8079A4D8, lbl_8079A4D8, lbl_8079A4D8);
    fn_802BE44C(&self->quake_0x4D0, &origin, (u8)(kind | 0xA0), 0);
}

/*
 * Resets the three camera slot flags and the three state bytes the follow path uses.
 */
void fn_802B8B8C(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    self->quake_0x4D0.active_0x0C = 0;
    self->quake_0x4BC.active_0x0C = 0;
    self->quake_0x4A8.active_0x0C = 0;
    fn_802BBEE0();
    fn_802BB118(self);
    self->field_0x49B = 0xFF;
    self->field_0x092 = 1;
    self->field_0x3C8 = 1;
}

/*
 * Selects the fourth camera slot and resets the follow state.
 */
void fn_802BB0EC(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    self->field_0x499 = 4;
    fn_802B8B8C();
}

/*
 * Puts the camera into its second mode with a 40-frame hold.
 */
void fn_802BC65C(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    fn_802BC4AC(2, 0);
    self->field_0x285 = 40;
}

/*
 * Puts the camera into its third mode with an 8-frame hold.
 */
void fn_802BC69C(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    fn_802BC4AC(3, 0);
    self->field_0x285 = 8;
}


/*
 * Puts the camera into mode 12 with the caller's argument.
 */
void fn_802BC7B0(u32 arg)
{
    fn_802BC4AC(12, arg);
}

/*
 * Puts the camera into mode 17 with a zero argument.
 */
void fn_802BC7BC(void)
{
    fn_802BC4AC(17, 0);
}

/*
 * Puts the camera into mode `base + 17`.
 */
void fn_802BC820(u32 base, u32 arg)
{
    fn_802BC4AC((u8)(base + 17), arg);
}

/*
 * Puts the camera into `mode` and holds it for 8 frames.
 */
void fn_802BC7C8(u8 mode, u32 arg)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    fn_802BC4AC(mode, arg);
    self->field_0x285 = 8;
}

/*
 * Clears the quake/fade state and switches mode, but only from the +0x284 == 1 state.
 */
void fn_802BC89C(u8 mode, u32 arg)
{
    if (camera_work_ck()) {
        fn_802BC468();
        fn_802BC4AC(mode, arg);
    }
}

} /* extern "C" */

/*
 * The current camera's world position.  The four camera entry points share one shape: build a camera
 * handle from `fn_80047398`, copy it through `fn_8004723C`, then read the wanted field out of it.
 *
 * Residual: 59.4 %.  The three calls are right and in retail's order, but retail's second object is
 * the return slot itself (MWCC aliased the returned local to the `sret` pointer), while ours keeps
 * both objects on the stack and copies the result out at the end: frame 0x30 vs 0x20, two extra
 * `lwz/stw` pairs and `addi r3, r1, 0xC` where retail passes `r31`.  Declaration order of the two
 * locals, and a `&&`-style single-return spelling, were tried - neither makes MWCC alias the local to
 * the return slot.  The sibling accessors (`get_current_view_mtx`, `get_camera_direction`,
 * `fn_802BDC90`/`fn_802BDE90`/`fn_802BDFC0`) have the same shape and the same blocker, so they are left
 * unwritten rather than landed at 0 %.
 */
nw4r::math::VEC3 get_camera_pos(void)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 tmp;
    void* cam;

    VEC3_ctor(&pos);
    cam = fn_80047398();
    fn_8004723C(&tmp, &cam);
    fn_800749C8(&tmp, &pos);
    return pos;
}

/*
 * Starts a camera quake at `origin` in the first quake slot.
 */
void set_quake_sub(u8 kind, nw4r::math::VEC3* origin)
{
    fn_802BE44C(&((CamWork*)fn_802BECD0())->quake_0x4A8, origin, kind, 0);
}
