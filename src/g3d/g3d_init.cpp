/*
 * g3d/g3d_init.cpp - nw4r g3d's library start-up: G3dInit (registers the version string, enables or disables the
 *   locked cache, sets the fast-cast quantisers, picks the render mode) and G3dReset (invalidates the G3D state).
 * RANGE. .text 0x8007A400-0x8007A518 (7 functions); extab 0x80008408-0x80008418, extabindex 0x80020C34-0x80020C4C,
 *   .data 0x8058EB38-0x8058EB80 (the "<< NW4R    - G3D ... >>" version string), .sdata 0x80791200-0x80791208 (its
 *   pointer, which G3dInit hands to OSRegisterVersion).  The string sits between `g3d/g3d_fog.cpp`'s last
 *   string and "g3d_light.cpp"; the GQR setters 0x8007A46C..OSInitFastCast are the fast-cast set-up the start-up
 *   calls, and 0x8007A510 (`g3d_state_invalidate(0x7FF)`) is G3dReset.
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` throughout (playbook idea 106).
 * NAMES. G3dInit (0x8007A400) and NW4R_G3D_Version_ are nw4r's (the body and the version string); fn_8007A510 keeps
 *   its stem until its consumer `pad_connect.cpp` is swept to G3dReset.  GUESS: g3d_init_fast_cast (0x8007A468, the
 *   one-call wrapper), g3d_init_fast_cast_gqrs (0x8007A46C), g3d_set_gqr6_s16_5 (0x8007A4B8) and
 *   g3d_set_gqr7_s16_8 (0x8007A494) - named for the quantiser each writes.  OSSetGQR6/OSSetGQR7 are the SDK's
 *   GQR writers (OSFastCast.h): static `asm` functions, one local copy in this unit.
 * RESIDUALS. g3d_state_set_render_mode (G3DState's render-mode setter, `g3d/g3d_state.cpp`) is declared here until its owner names
 *   it.  The `.sdata` claim is 8 bytes; the object emits the 4-byte pointer (the rest is link fill).
 *   The map's g3d_set_gqr7_s16_8/g3d_set_gqr6_s16_5 rows (0x24 each) cover the setter (0xC: two `li` and a tail
 *   `b`) and the OSSetGQR7/OSSetGQR6 copy that follows it (0x18); objdiff pairs them once the rows are split at
 *   +0xC.  The setter's `b` is relocated in the object and resolves to the next word at link time.
 * SHAPES. Each GQR setter tail-calls a static `asm` function (`nofralloc` ... `blr`) over the register
 *   parameters; MWCC never inlines an `asm` function, and it emits each one where it is defined, right after its
 *   setter (an `asm` block inside an inline C function folds its constants at -O3 instead).
 *   OSInitFastCast is an `asm` function, so it stays out of line.
 */

#include "types.h"
#include "OS/OS.h"                 /* OSRegisterVersion (rule 2) */
#include "VI/vi.h"                 /* VIGetTvFormat (rule 2) */
#include "RVLGX/GXFrameBuf.h"      /* GXNtsc480IntDf / GXPal528IntDf (rule 2) */
#include "nw4r/ut_LockedCache.h"   /* nw4r::ut::LC (rule 2) */
#include "g3d/g3d_state.h"         /* g3d_state_invalidate (rule 2) */
#include "g3d/g3d_init.h"

#pragma peephole off

extern "C" {
u32 g3d_state_set_render_mode(const GXRenderModeObj* pMode);

void OSInitFastCast(void);
void g3d_init_fast_cast(void);
void g3d_init_fast_cast_gqrs(void);
void g3d_set_gqr6_s16_5(void);
void g3d_set_gqr7_s16_8(void);
}

/* The version string the start-up registers with the OS. */
const char* NW4R_G3D_Version_ = "<< NW4R    - G3D \tfinal   build: Jan 30 2010 10:51:46 (0x4302_158) >>";

extern "C" {
static asm void OSSetGQR6(register u32 type, register u32 scale);
static asm void OSSetGQR7(register u32 type, register u32 scale);
}

namespace nw4r { namespace g3d {

/* 0x8007A400 (0x68): registers the version, sets the locked cache, the fast-cast quantisers and the render mode. */
void G3dInit(bool enableLockedCache) {
    OSRegisterVersion(NW4R_G3D_Version_);
    if (enableLockedCache) {
        ut::LC::Enable();
    } else {
        ut::LC::Disable();
    }
    g3d_init_fast_cast();
    g3d_state_set_render_mode(VIGetTvFormat() == 1 ? &GXPal528IntDf : &GXNtsc480IntDf);
}

}}

extern "C" {

/* 0x8007A468 (0x4): sets every fast-cast quantiser. */
void g3d_init_fast_cast(void) {
    g3d_init_fast_cast_gqrs();
}

/* 0x8007A46C (0x28): sets GQR2..GQR7. */
void g3d_init_fast_cast_gqrs(void) {
    OSInitFastCast();
    g3d_set_gqr6_s16_5();
    g3d_set_gqr7_s16_8();
}

/* 0x8007A494: sets GQR7 to s16 with scale 8. */
void g3d_set_gqr7_s16_8(void) {
    OSSetGQR7(7, 8);
}

/* Writes GQR7 (both halves) from a type and a scale. */
static asm void OSSetGQR7(register u32 type, register u32 scale) {
    nofralloc
    slwi r0, scale, 8
    or type, r0, type
    slwi r0, type, 16
    or r0, type, r0
    mtspr 919, r0
    blr
}

/* 0x8007A4B8: sets GQR6 to s16 with scale 5. */
void g3d_set_gqr6_s16_5(void) {
    OSSetGQR6(7, 5);
}

/* Writes GQR6 (both halves) from a type and a scale. */
static asm void OSSetGQR6(register u32 type, register u32 scale) {
    nofralloc
    slwi r0, scale, 8
    or type, r0, type
    slwi r0, type, 16
    or r0, type, r0
    mtspr 918, r0
    blr
}

/* 0x8007A4DC (0x34): sets GQR2..GQR5 to the u8/u16/s8/s16 fast-cast pairs. */
asm void OSInitFastCast(void) {
    nofralloc
    li r3, 4
    oris r3, r3, 4
    mtspr 914, r3
    li r3, 5
    oris r3, r3, 5
    mtspr 915, r3
    li r3, 6
    oris r3, r3, 6
    mtspr 916, r3
    li r3, 7
    oris r3, r3, 7
    mtspr 917, r3
    blr
}

/* 0x8007A510 (0x8): invalidates the whole G3D state. */
void fn_8007A510(void) {
    g3d_state_invalidate(0x7FF);
}

}
