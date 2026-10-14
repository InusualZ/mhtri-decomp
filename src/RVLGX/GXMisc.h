/*
 * RVLGX/GXMisc.h - declarations of the non-static functions of RVLGX/GXMisc.c (docs/plan.md 6.5 rule 2).
 *   `GXAbortFrame` keeps its leaf header, RVLGX/GXAbortFrame.h.
 */
#ifndef MHTRI_RVLGX_GXMISC_H
#define MHTRI_RVLGX_GXMISC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The draw-sync token callback (receives the PE token) and the draw-done callback. */
typedef void (*GXDrawSyncCallback)(u16 token);
typedef void (*GXDrawDoneCallback)(void);

/* 0x804B5DB0 (0x80): sets a miscellaneous GX option (1 = vertex-count flush, 2 = display-list context save,
 *   3 = abort waits for the PE copy). */
void GXSetMisc(s32 token, u32 val);
/* 0x804B5E30 (0x164): resets the graphics processor through the PI reset register. */
void __GXAbort(void);
/* 0x804B6160 (0xCC): issues a draw-done token and waits for the finish interrupt. */
void GXDrawDone(void);
/* 0x804B6230 (0x24): re-sends the pixel-control register to synchronise the pixel mode. */
void GXPixModeSync(void);
/* 0x804B6260 (0x10): writes the PE alpha-compare register. */
void GXPokeAlphaMode(s32 func, u8 threshold);
/* 0x804B6270 (0x18): writes the PE alpha-read mode. */
void GXPokeAlphaRead(u32 mode);
/* 0x804B6290 (0x14): sets the PE alpha-update bit. */
void GXPokeAlphaUpdate(u8 update_enable);
/* 0x804B62B0 (0x5C): writes the PE blend mode. */
void GXPokeBlendMode(s32 type, u32 src_factor, u32 dst_factor, u32 op);
/* 0x804B6310 (0x14): sets the PE colour-update bit. */
void GXPokeColorUpdate(u8 update_enable);
/* 0x804B6330 (0x18): writes the PE destination-alpha register. */
void GXPokeDstAlpha(u8 enable, u8 alpha);
/* 0x804B6350 (0x14): sets the PE dither bit. */
void GXPokeDither(u8 dither);
/* 0x804B6370 (0x1C): writes the PE Z-mode register. */
void GXPokeZMode(u8 compare_enable, u32 func, u8 update_enable);
/* 0x804B6390 (0x18): reads one EFB pixel. */
void GXPeekARGB(u16 x, u16 y, u32* color);
/* 0x804B63B0 (0x14): writes one EFB pixel. */
void GXPokeARGB(u16 x, u16 y, u32 color);
/* 0x804B63D0 (0x44): installs the draw-sync callback and returns the previous one. */
GXDrawSyncCallback GXSetDrawSyncCallback(GXDrawSyncCallback cb);
/* 0x804B64B0 (0x44): installs the draw-done callback and returns the previous one. */
GXDrawDoneCallback GXSetDrawDoneCallback(GXDrawDoneCallback cb);
/* 0x804B6580 (0x64): installs the PE token and finish interrupt handlers and unmasks them. */
void __GXPEInit(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXMISC_H */
