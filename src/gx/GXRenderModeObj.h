/* gx/GXRenderModeObj.h - the SDK's render-mode record (`GXRenderModeObj`) that VI and GX configuration take. */
#ifndef MHTRI_GX_GXRENDERMODEOBJ_H
#define MHTRI_GX_GXRENDERMODEOBJ_H

#include "types.h"

/* size: 0x3C */
typedef struct GXRenderModeObj {
    /* +0x00 */ u32 viTVmode;
    /* +0x04 */ u16 fbWidth;
    /* +0x06 */ u16 efbHeight;
    /* +0x08 */ u16 xfbHeight;
    /* +0x0A */ u16 viXOrigin;
    /* +0x0C */ u16 viYOrigin;
    /* +0x0E */ u16 viWidth;
    /* +0x10 */ u16 viHeight;
    /* +0x12 */ u16 pad_0x12;
    /* +0x14 */ u32 xfbMode;
    /* +0x18 */ u8 field_rendering;
    /* +0x19 */ u8 aa;
    /* +0x1A */ u8 sample_pattern[12][2];
    /* +0x32 */ u8 vfilter[7];
    /* +0x39 */ u8 pad_0x39[3];
} GXRenderModeObj;

#endif /* MHTRI_GX_GXRENDERMODEOBJ_H */
