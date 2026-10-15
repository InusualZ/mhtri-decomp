/*
 * g3d/g3d_anmobj.cpp - nw4r g3d `g3d_anmobj.cpp`: the frame play policies (`PlayPolicy_Onetime`,
 *   `PlayPolicy_Loop`) with their float helpers, and `AnmObj`'s flag members.
 * RANGE. .text 0x800648A8-0x800649CC; extab 0x800078EC-0x800078F4, extabindex 0x8001FC8C-0x8001FC98, .rodata
 *   0x8056F568-0x8056F578, .data 0x8058C248-0x8058C288, .sdata 0x80791168-0x80791170, .sdata2 0x80795D68-0x80795D70.
 *   The TU is `g3d_anmobj.cpp`: its one-copy `__FILE__` string (0x8058C248) is cited by PlayPolicy_Loop, the previous
 *   TU's (`g3d_anmclr.cpp`) by AnmObjMatClr::TestExistence and the next TU's (`g3d_anmscn.cpp`, 0x8058C288) from
 *   0x800649CC on.  The "AnmObj" type-name record (0x8056F568) and `FrameCtrl::smBaseUpdateRate` (0x80791168) sit
 *   between the neighbours' data in link order; only `g3d/g3d_anmchr.cpp` reads them.
 * FLAGS. `cflags_g3d` (docs/g3d.md).
 * NAMES. math_fmod and f32_select_nonneg are GUESSES (the bodies' behaviour); anmobj_cpp_file and
 *   anmobj_start_lt_end_msg name the strings.  PlayPolicy_Onetime/PlayPolicy_Loop and AnmObj::SetAnmFlag/TestAnmFlag
 *   are nw4r's.  The cross-unit declarations are `g3d/g3d_anmobj.h`.
 * RESIDUALS. PlayPolicy_Loop: the `fmr` after the first fmodf branch (retail keeps the result in the frame's
 *   register).  flipcheck: `.text` and `.sdata2` differ; `.rodata`, `.data` and `.sdata` are claimed and not emitted.
 * SHAPES. The unit compiles with `#pragma peephole off` throughout (playbook idea 106).
 */

#include "types.h"
#include "nw4r/db_assert.h"   /* nw4r::db::Panic, owner nw4r/db_assert.cpp (rule 2) */
#include "g3d/g3d_anmobj.h"
#include "g3d/g3d_obj.h"      /* nw4r::g3d::AnmObj (rule 1) */

#pragma peephole off

extern const char anmobj_cpp_file[];          /* "g3d_anmobj.cpp" */
extern const char anmobj_start_lt_end_msg[];  /* "NW4R:Failed assertion startFrame < endFrame" */

/* 0x8006497C (0x8): `looped` when `frame` is not negative, else `oneTime`. */
extern "C" asm f32 f32_select_nonneg(register f32 frame, register f32 looped, register f32 oneTime)
{
    nofralloc
    fsel frame, frame, looped, oneTime
    blr
}

/* 0x80064988 (0x2C): sets or clears animation flag `flag`. */
void nw4r::g3d::AnmObj::SetAnmFlag(AnmFlag flag, bool on)
{
    if (on)
        mFlags |= flag;
    else
        mFlags &= ~flag;
}

/* 0x800649B4 (0x18): whether any of `flag`'s bits is set. */
bool nw4r::g3d::AnmObj::TestAnmFlag(AnmFlag flag) const
{
    return (mFlags & flag) != 0;
}

extern "C" f32 fmodf(f32 x, f32 y);

/* 0x80064984 (0x4): the float remainder. */
extern "C" f32 math_fmod(f32 x, f32 y)
{
    return fmodf(x, y);
}

/* The first of the two `PlayPolicy` names: `Onetime` keeps its end frame. */
namespace nw4r {
namespace g3d {

f32 PlayPolicy_Onetime(f32 frame, f32 start, f32 end)
{
    (void)frame;
    (void)start;
    return end;
}

/* 0x800648B0 (0xCC): folds `frame` into [0, end - start) by the remainder, a negative frame wrapping from the end. */
f32 PlayPolicy_Loop(f32 start, f32 end, f32 frame)
{
    if (!(start < end)) {
        nw4r::db::Panic(anmobj_cpp_file, 38, anmobj_start_lt_end_msg);
    }
    f32 duration = end - start;
    frame = (frame >= 0.0f) ? math_fmod(frame, duration) : math_fmod(frame + duration, duration);
    return frame + f32_select_nonneg(frame, 0.0f, duration);
}

} /* namespace g3d */
} /* namespace nw4r */
