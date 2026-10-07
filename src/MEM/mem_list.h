/*
 * MEM/mem_list.h - declarations of the symbols owned by `MEM/mem_list.c` that other units call or read.
 */
#ifndef MEM_MEM_LIST_H
#define MEM_MEM_LIST_H

#include "types.h"
#include "MEM/mem.h"

/* 0x804C24C0 - empties a list and records the node displacement its objects use. */
void MEMInitList(MEMList* list, u16 offset);

/* 0x804C24E0 - appends an object at the tail. */
void MEMAppendListObject(MEMList* list, void* object); /* untyped: opaque handle passed through */

/* 0x804C2550 - unlinks an object. */
void MEMRemoveListObject(MEMList* list, void* object); /* untyped: opaque handle passed through */

/* 0x804C25C0 - the object after `object` (NULL yields the head). */
void* MEMGetNextListObject(MEMList* list, void* object); /* untyped: opaque handle passed through */

#endif
