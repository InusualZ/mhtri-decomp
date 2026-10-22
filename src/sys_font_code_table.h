/* Leaf header (docs/plan.md 6.5 rule 2): `mh3_pad.cpp`'s `sys_font_code_table`, the one declaration
 * `menu/menu_sysmsg.cpp` needs.  GUESS name, from the data (listed on the NAMES line of menu/menu_sysmsg.cpp). */
#ifndef MHTRI_SYS_FONT_CODE_TABLE_H
#define MHTRI_SYS_FONT_CODE_TABLE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .data 0x80580058 (0x394): the system font's {character code, glyph} pairs */
extern const u16 sys_font_code_table[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SYS_FONT_CODE_TABLE_H */
