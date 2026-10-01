/*
 * nw4r/fn_805012C4.h - declarations of the symbols owned by `nw4r/fn_805012C4.cpp` that other units call or read.
 */
#ifndef NW4R_FN_805012C4_H
#define NW4R_FN_805012C4_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object, typed by the callers' views */
void  MEMInitList(void* list, u16 offset);   /* ut::List initialiser */

/* --------------------------------------------------------------------------------------------- */
/* Callees whose band holds no registered unit                                                    */
/* --------------------------------------------------------------------------------------------- */
/* untyped: opaque band object, typed by the callers' views */
void fn_80501A64(void* list, void* node);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80501BF4(void* list, void* node);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80501C60(void* list, void* node);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80501C9C(void* list, u16 n);            /* List_GetNth */

#ifdef __cplusplus
}
#endif

#endif
