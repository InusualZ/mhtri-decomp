/* `data_file_options_get` (0x8044AFC4), defined by `menu/menu_plsearch.cpp` (leaf header, rule 2).  C linkage. */
#ifndef MHTRI_MENU_DATA_FILE_OPTIONS_GET_H
#define MHTRI_MENU_DATA_FILE_OPTIONS_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Copies the read data file's 0x1F option bytes to `options`. */
void data_file_options_get(u8* options);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_DATA_FILE_OPTIONS_GET_H */
