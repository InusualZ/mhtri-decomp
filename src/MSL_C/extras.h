/*
 * MSL_C/extras.h - declarations of the symbols owned by `MSL_C/extras.c` that other units call.
 */
#ifndef MSL_C_EXTRAS_H
#define MSL_C_EXTRAS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804681CC (0xA0): compares two strings ignoring case; returns <0, 0 or >0. */
int stricmp(const char* a, const char* b);

#ifdef __cplusplus
}
#endif

#endif
