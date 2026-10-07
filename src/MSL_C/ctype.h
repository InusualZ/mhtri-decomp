/*
 * MSL_C/ctype.h - the inline character-class tests the string-conversion units share: they read the class table
 *    of the active locale (`MSL_C/locale.c`) and answer 0 outside 0..255.
 */
#ifndef MSL_C_CTYPE_H
#define MSL_C_CTYPE_H

#include "MSL_C/locale.h"

#define CTYPE_ALPHA 0x0001
#define CTYPE_DIGIT 0x0008
#define CTYPE_UPPER 0x0200
#define CTYPE_SPACE 0x0100

static inline int ctype_out_of_range(int c)
{
    return c < 0 || c > 255;
}

static inline int ctype_class(int c, int mask)
{
    return ctype_out_of_range(c) ? 0 : (_current_locale.ctype->ctype_map[c] & mask);
}

static inline int ctype_toupper(int c)
{
    return ctype_out_of_range(c) ? c : _current_locale.ctype->upper_map[c];
}

#endif
