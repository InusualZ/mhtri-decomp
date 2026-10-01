/*
 * AX/AXFXReverbStd.h - entry points of the AXFXReverbStd unit (0x80475E30..0x804770E0) that the AXFXReverbHi wrappers call.
 */
#ifndef AX_AXFXREVERBSTD_H
#define AX_AXFXREVERBSTD_H

#include "AX/AXFXReverbHi.h"

BOOL fn_80475E30(AXFXReverbHi* reverb);
BOOL fn_80475FF0(AXFXReverbHi* reverb);
void AXFXReverbStdShutdown(AXFXReverbHi* reverb);
void fn_80476120(AXFXBuffer* buffer, AXFXReverbHi* reverb, s32* out0, s32* out1);

#ifdef __cplusplus
extern "C" {
#endif

/* The two hooks AXFXSetHooks installs; the map names them lbl_80793D20 / lbl_80793D24. */
/* untyped: opaque band object, typed by the callers' views */
extern void* lbl_80793D20; /* alloc hook (sda21) */

/* untyped: opaque band object, typed by the callers' views */
extern void* lbl_80793D24; /* free hook (sda21)  */

#ifdef __cplusplus
}
#endif

#endif
