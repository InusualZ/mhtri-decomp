/*
 * AX/AXFXDelay.cpp - the AXFX delay effects' delay-line release (three- and four-line bodies) and the shutdown
 *    wrappers over it.
 *
 * RANGE. .text 0x80476F10-0x80477090 (4 functions, 0x180 B); no data.  Left edge: the end of
 *    `AX/AXFXChorus.cpp`; right edge: the start of `AX/AXFXHooks.cpp`.  Both edges are UNPROVEN (no pooled
 *    data); the source order Shutdown3, Shutdown4, Free3, Free4 shows the four functions are one file's.
 * FLAGS. the `OS` lib group with the 16-byte function alignment restored below.
 * NAMES. GUESS: AXFXDelay3/4Shutdown and AXFXDelay3/4Free (the number is the delay-line count; flag word at
 *    +0x7C / +0x90).
 * RESIDUALS. none measured.
 */

#pragma function_align 16

#include "types.h"
#include "AX/AXFXDelay.h"
#include "AX/AXFXHooks.h"
#include "OS/OSInterrupt.h"

extern "C" {

/* Frees the delay lines of a three-channel delay and reports success. */
BOOL AXFXDelay3Shutdown(AXFXDelay3* effect)
{
    AXFXDelay3Free(effect);
    return TRUE;
}

/* Frees the delay lines of a four-channel delay and reports success. */
BOOL AXFXDelay4Shutdown(AXFXDelay4* effect)
{
    AXFXDelay4Free(effect);
    return TRUE;
}

/* Disables a three-channel delay and frees its delay lines. */
void AXFXDelay3Free(AXFXDelay3* effect)
{
    u32 i;
    u32 level = OSDisableInterrupts();

    effect->flags |= 1;
    for (i = 0; i < 3; i++) {
        if (effect->line[i] != 0) {
            AXFXFree(effect->line[i]);
        }
        effect->line[i] = 0;
    }
    OSRestoreInterrupts(level);
}

/* Disables a four-channel delay and frees its delay lines. */
void AXFXDelay4Free(AXFXDelay4* effect)
{
    u32 i;
    u32 level = OSDisableInterrupts();

    effect->flags |= 1;
    for (i = 0; i < 4; i++) {
        if (effect->line[i] != 0) {
            AXFXFree(effect->line[i]);
        }
        effect->line[i] = 0;
    }
    OSRestoreInterrupts(level);
}

} /* extern "C" */
