/*
 * MSL_C/scanf.h - the formatted-input string reader, owned by `MSL_C/scanf.c`.
 */
#ifndef MSL_C_SCANF_H
#define MSL_C_SCANF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The cursor the string reader advances. */
typedef struct StringReadState {
    /* +0x00 */ const char* cursor;     /* next character to read */
    /* +0x04 */ int hit_nul;            /* set once the terminator was read */
} StringReadState; /* size: 0x8 */

/* 0x8045F2F4 (0x88): reads, ungets or tests the end of a string source (action 0, 1, 2). */
int __StringRead(StringReadState* state, int ch, int action);

#ifdef __cplusplus
}
#endif

#endif
