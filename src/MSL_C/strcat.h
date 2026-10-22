/* MSL_C/strcat.h - leaf header (docs/plan.md 6.5 rule 2): `MSL_C/string.c`'s `strcat`, for a consumer whose other
 * headers spell `strcpy` with `s8` and so cannot include `MSL_C/string.h`. */
#ifndef MHTRI_MSL_C_STRCAT_H
#define MHTRI_MSL_C_STRCAT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045F658 (0x2C): appends the string `src` to the end of `dst`. */
char* strcat(char* dst, const char* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MSL_C_STRCAT_H */
