/* ef/sys_tex_image_get.h - leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s `sys_tex_image_get`. */
#ifndef MHTRI_EF_SYS_TEX_IMAGE_GET_H
#define MHTRI_EF_SYS_TEX_IMAGE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D05DC (0x3C): the image data of `system_w`'s texture slot `index`, or 0 when the slot is out of range or idle. */
u32 sys_tex_image_get(s32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_SYS_TEX_IMAGE_GET_H */
