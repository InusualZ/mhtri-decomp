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

/* The one-word file/model handle (`ResFile`/`ResMdl` shape: the resource's data address) the effect model creators
 * build and hand to the animation helpers by value.  size: 0x04 */
typedef struct EftResFileHandle {
    /* +0x00 */ u32 data_0x00;
} EftResFileHandle;

#ifdef __cplusplus
extern "C" {
#endif

/* Is the file's header present (< 0 = not loaded yet). */
s32 fn_800D4CE4(const char* path);
/* the address of a resource's data, or a negative code. */
s32 fn_800D4CE4_none(void);
char* fn_800D4D9C(s32 index);
u32 nw_res_mem_size(s32 index);
s32 fn_800D56F4(void* arg, void* name);
/* 0x800D58B0 - clears one resource-table entry outright. */
void nw_res_entry_clear(s32 index);

#ifdef __cplusplus
}

namespace nw4r { namespace g3d { class ScnMdlSimple; } }
struct _g3d_work;

extern "C" {
/* 0x800D3874 - the number of g3d works the model's animation `kind` needs. */
s32 nw_res_anm_work_num(EftResFileHandle res, EftResFileHandle mdl, s32 kind, struct _g3d_work** list);
/* 0x800D38F8 - the animation built over those works (NULL on failure). */
/* untyped: opaque handle - the animation record the caller hands on */
void* nw_res_anm_create(EftResFileHandle res, EftResFileHandle mdl, nw4r::g3d::ScnMdlSimple* scn, s32 kind,
                        struct _g3d_work* first, struct _g3d_work** rest, f32 strength);
}

/* 0x800D5864 - deletes a loaded resource's entry (0 when it was freed). */
s32 nwDelResource(s32 index);

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
