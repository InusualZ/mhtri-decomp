/* TPL/tpl.cpp - the texture palette loader (`TPLBind`, `TPLGet`).
 * RANGE. .text 0x804E45B0-0x804E46F0 (2 functions); .data 0x8062AB68-0x8062AB98; .sdata 0x80794148-0x80794150.
 *   Edges: the `TPL.c` __FILE__ string (.sdata 0x80794148) and the "invalid version number for texture palette"
 *   string (.data 0x8062AB68) are read only by TPLBind; 0x804E46F0 starts the USB print helper that reads
 *   "USB: ".
 * FLAGS. `cflags_base` (-O4,p, 16-byte function alignment): both starts are 16-aligned.
 * NAMES. `TPLBind` and `TPLGet` are the map's names.
 * RESIDUALS. none known.
 */
#include "TPL/tpl.h"
#include "OS/OSError.h"

/* 0x804E45B0 (0x118): relocates the offsets of a texture palette file into pointers. */
void TPLBind(TPLPalette* pal)
{
    u16 i;

    if (pal->version != TPL_VERSION) {
        OSPanic("TPL.c", 25, "invalid version number for texture palette");
    }

    pal->descriptorArray = (TPLDescriptor*)((u32)pal->descriptorArray + (u32)pal);

    for (i = 0; i < pal->numDescriptors; i++) {
        if (pal->descriptorArray[i].textureHeader) {
            pal->descriptorArray[i].textureHeader =
                (TPLImageHeader*)((u32)pal + (u32)pal->descriptorArray[i].textureHeader);
            if (!pal->descriptorArray[i].textureHeader->unpacked) {
                pal->descriptorArray[i].textureHeader->data = (void*)((u32)pal + (u32)pal->descriptorArray[i].textureHeader->data);
                pal->descriptorArray[i].textureHeader->unpacked = 1;
            }
        }
        if (pal->descriptorArray[i].clutHeader) {
            pal->descriptorArray[i].clutHeader = (TPLClutHeader*)((u32)pal + (u32)pal->descriptorArray[i].clutHeader);
            if (!pal->descriptorArray[i].clutHeader->unpacked) {
                pal->descriptorArray[i].clutHeader->data = (void*)((u32)pal + (u32)pal->descriptorArray[i].clutHeader->data);
                pal->descriptorArray[i].clutHeader->unpacked = 1;
            }
        }
    }
}

/* 0x804E46D0 (0x20): returns the descriptor of a texture, wrapping the id by the palette size. */
TPLDescriptor* TPLGet(TPLPalette* pal, u32 id)
{
    return &pal->descriptorArray[id % pal->numDescriptors];
}
