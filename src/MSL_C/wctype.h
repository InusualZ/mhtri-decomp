/*
 * MSL_C/wctype.h - the wide-character class and case tables `MSL_C/wctype.c` owns.
 */
#ifndef MSL_C_WCTYPE_H
#define MSL_C_WCTYPE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80572B88 - class flags of the wide characters 0..255. */
extern const u16 wctype_class_map[256];

/* 0x80572D88 - the tolower map of the wide characters 0..255. */
extern const u16 wctype_lower_map[256];

/* 0x80572F88 - the toupper map of the wide characters 0..255. */
extern const u16 wctype_upper_map[256];

#ifdef __cplusplus
}
#endif

#endif
