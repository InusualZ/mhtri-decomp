/*
 * OS/OSLink.h - declarations of the symbols owned by `OS/OSLink.c` that other units call or read.
 */
#ifndef OS_OSLINK_H
#define OS_OSLINK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D1440 - clears the module queue and the string table in low memory. */
void __OSModuleInit(void);

#ifdef __cplusplus
}
#endif

#endif
