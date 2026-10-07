/*
 * TPL/tpl.h - the texture palette file layout and the entry points of `TPL/tpl.cpp`.
 */
#ifndef TPL_TPL_H
#define TPL_TPL_H

#include "types.h"

#define TPL_VERSION 0x0020AF30

/* size: 0x24 (the image header fields the loader touches; the rest is GX texture data) */
struct TPLImageHeader {
    /* +0x00 */ u8 pad_0x00[8];
    /* +0x08 */ void* data; /* image data offset, relocated to a pointer on bind */
    /* +0x0C */ u8 pad_0x0C[0x17];
    /* +0x23 */ u8 unpacked; /* set once the data offset has been relocated */
};

/* size: 0x0C (the palette header fields the loader touches) */
struct TPLClutHeader {
    /* +0x00 */ u8 pad_0x00[2];
    /* +0x02 */ u8 unpacked; /* set once the data offset has been relocated */
    /* +0x03 */ u8 pad_0x03[5];
    /* +0x08 */ void* data; /* palette data offset, relocated to a pointer on bind */
};

/* size: 0x08 */
struct TPLDescriptor {
    /* +0x00 */ TPLImageHeader* textureHeader;
    /* +0x04 */ TPLClutHeader* clutHeader;
};

/* size: 0x0C */
struct TPLPalette {
    /* +0x00 */ u32 version;
    /* +0x04 */ u32 numDescriptors;
    /* +0x08 */ TPLDescriptor* descriptorArray;
};

#ifdef __cplusplus
extern "C" {
#endif

void TPLBind(TPLPalette* pal);
TPLDescriptor* TPLGet(TPLPalette* pal, u32 id);

#ifdef __cplusplus
}
#endif

#endif
