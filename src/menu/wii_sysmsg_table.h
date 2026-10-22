/* `wii_sysmsg_table` (.data 0x8060A960, 0x18 B), owned by `menu/menu_sysmsg.cpp`'s `.data` (leaf header, rule 2).
 * GUESS name: `wii_sysmsg_gen` indexes it by `system_w`'s message-language byte, then by the message id. */
#ifndef MHTRI_MENU_WII_SYSMSG_TABLE_H
#define MHTRI_MENU_WII_SYSMSG_TABLE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Per message language (six): the language's table of the 24 Wii system message texts. */
extern char** wii_sysmsg_table[6];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_WII_SYSMSG_TABLE_H */
