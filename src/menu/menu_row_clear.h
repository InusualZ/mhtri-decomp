/* `menu_row_clear`, defined by `menu/menu_row.cpp` (leaf header, rule 2; `menu/menu_row.h` does not declare it).  C
 * linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_ROW_CLEAR_H
#define MHTRI_MENU_MENU_ROW_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8034C4B4 - clears the row at `index` in the 8-row item table. */
void menu_row_clear(u16 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_ROW_CLEAR_H */
