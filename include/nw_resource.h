/*
 * Declarations owned by `nw_resource.cpp` (docs/plan.md 6.5 rule 2): the four lookup/load helpers the
 * effect resource manager calls.  Consumers include this header instead of declaring them themselves.
 */
#ifndef MHTRI_NW_RESOURCE_H
#define MHTRI_NW_RESOURCE_H

#include "types.h"

/* One entry of the resource name table (`ResManager.resTable`, stride 0x4C): the loaded file's name, its
 * table index, the file data and a flag word.  size: 0x4C */
struct ResEntry {
    /* +0x00 */ char name[64];
    /* +0x40 */ s32 index;
    /* +0x44 */ void* data;
    /* +0x48 */ u32 flag;
};

#ifdef __cplusplus
extern "C" {
#endif

/* Is the file's header present (< 0 = not loaded yet). */
s32 fn_800D4CE4(const char* path);
/* the address of a resource's data, or a negative code. */
s32 fn_800D4CE4_none(void);
char* fn_800D4D9C(s32 index);
u32 fn_800D5138(s32 index);
s32 fn_800D56F4(void* arg, void* name);

#ifdef __cplusplus
}

/* The C++-linkage entry points: the map's `pull_res_mem__FPcUll` and friends are the compiler's spelling
 * of these (rule 9).  `pull_res_mem` reserves a resource-memory slot for `path` (a negative handle when
 * none is free), `getResMemAdrs` its address, `push_res_mem` gives the slot back, `nwAddResource`
 * registers loaded file data under `name` and `ckResourceName` finds the entry of a loaded name. */
/* untyped: byte range - the address of the resource-memory slot's buffer */
void* getResMemAdrs(s32 index);
s32 pull_res_mem(char* path, u32 size, s32 mode);
s32 push_res_mem(s32 index);
ResEntry* ckResourceName(char* path);
/* untyped: byte range - the loaded file image `data` registers under `name` */
s32 nwAddResource(char* name, void* data);

/* 0x800D5D18/0x800D5D2C - open and close the loader work's per-frame move window (`nwMoveStart__Fv`,
 * `nwMoveEnd__Fv`).  Added with `quest/arenatask.cpp`'s task step. */
void nwMoveStart(void);
void nwMoveEnd(void);
#endif

#endif /* MHTRI_NW_RESOURCE_H */
