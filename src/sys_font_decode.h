/* Leaf header (docs/plan.md 6.5 rule 2): `mh3_pad.cpp`'s `sys_font_decode`, for `font/flfnt.cpp`.  C linkage (the map row is a plain
 * name). */
#ifndef MHTRI_SYS_FONT_DECODE_H
#define MHTRI_SYS_FONT_DECODE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80041138 (0x10): decodes the system font image into `dst`. */
u32 sys_font_decode(u16* dst);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SYS_FONT_DECODE_H */
