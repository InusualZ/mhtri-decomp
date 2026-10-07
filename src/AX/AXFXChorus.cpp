/*
 * AX/AXFXChorus.cpp - the AXFX chorus effects' delay-line release (three- and four-line bodies).
 *
 * RANGE. .text 0x80476DF0-0x80476F10 (2 functions, 0x120 B); no data.  Left edge: the end of
 *    `AX/AXFXReverbStdExp.cpp`; right edge: the start of `AX/AXFXDelay.cpp`.  Both edges are UNPROVEN: the range
 *    owns no pooled data, so no pool pins them; they rest on the shutdown loops (free only when non-null, null
 *    the slot inside the branch) differing from the delay loops (null the slot unconditionally), and on the
 *    delay functions at 0x80476F10.. forming one source order of their own.
 * FLAGS. the `OS` lib group with the 16-byte function alignment restored below.
 * NAMES. GUESS: AXFXChorus3Shutdown / AXFXChorus4Shutdown (the number is the delay-line count; flag word at
 *    +0x3C / +0x50).
 * RESIDUALS. none measured.
 */

#pragma function_align 16

#include "types.h"
#include "AX/AXFXChorus.h"
#include "AX/AXFXHooks.h"
#include "OS/OSInterrupt.h"

extern "C" {

/* Disables the effect and frees its three delay lines. */
void AXFXChorus3Shutdown(AXFXChorus3* effect)
{
    u32 i;
    u32 level = OSDisableInterrupts();

    effect->flags |= 1;
    for (i = 0; i < 3; i++) {
        if (effect->line[i] != 0) {
            AXFXFree(effect->line[i]);
            effect->line[i] = 0;
        }
    }
    OSRestoreInterrupts(level);
}

/* Disables the effect and frees its four delay lines. */
void AXFXChorus4Shutdown(AXFXChorus4* effect)
{
    u32 i;
    u32 level = OSDisableInterrupts();

    effect->flags |= 1;
    for (i = 0; i < 4; i++) {
        if (effect->line[i] != 0) {
            AXFXFree(effect->line[i]);
            effect->line[i] = 0;
        }
    }
    OSRestoreInterrupts(level);
}

} /* extern "C" */
