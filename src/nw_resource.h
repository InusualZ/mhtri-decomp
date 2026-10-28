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
s32 res_mem_index_ref(const char* path);
char* res_mem_name_get(s32 index);
u32 nw_res_mem_size(s32 index);
/* 0x800D56F4 - registers a resource-table entry for a new file without the name lookup (-1 when full). */
/* untyped: byte range - the loaded file image `data` registers under `name` */
s32 nw_res_entry_add(char* name, void* data);
/* 0x800D58B0 - clears one resource-table entry outright. */
void nw_res_entry_clear(s32 index);

#ifdef __cplusplus
}

namespace nw4r { namespace g3d { class ScnMdlSimple; class G3dObj; } }

/* One g3d work slot of the resource manager's pool: in use, the kind of object it holds (1 simple model, 2 model,
 * 3 texture-SRT animation, 4 texture-SRT override, 5 scene animation), its lifetime stage (1 = freed with the stage),
 * the object and the size word (the measured size for a simple model, else the object again). */
struct _g3d_work {
    /* +0x00 */ s16 used;
    /* +0x02 */ u8 kind;
    /* +0x03 */ u8 stage;
    /* +0x04 */ nw4r::g3d::G3dObj* obj;
    /* +0x08 */ u32 size;
}; /* size: 0xC */

extern "C" {
/* 0x800D3874 - builds the texture-SRT override work of lifetime `kind` into `*list`; returns the file's texture-SRT
 * animation count. */
s32 nw_res_anm_work_num(EftResFileHandle res, EftResFileHandle mdl, s32 kind, struct _g3d_work** list);
/* 0x800D38F8 - attaches a looping texture-SRT animation per animation of the file, played at twice `strength`, to the
 * override in `first` and sets the override on `scn`; returns the override (NULL on failure).  The rate comes second:
 * every caller evaluates it before the handles. */
/* untyped: opaque handle - the animation record the caller hands on */
void* nw_res_anm_create(EftResFileHandle res, f32 strength, EftResFileHandle mdl, nw4r::g3d::ScnMdlSimple* scn, s32 kind,
                        struct _g3d_work* first, struct _g3d_work** rest);
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

class EfSys;

extern "C" {
/* 0x800D3C0C - the effect system the resource manager bound (`ef/ef_effectsystem.cpp`'s instance), or NULL before
 * the manager exists. */
EfSys* nw_res_ef_system_get(void);
}

/* 0x800D5D18/0x800D5D2C - open and close the loader work's per-frame move window (`nwMoveStart__Fv`,
 * `nwMoveEnd__Fv`).  Added with `quest/arenatask.cpp`'s task step. */
void nwMoveStart(void);
void nwMoveEnd(void);
#endif

#ifdef __cplusplus
/* 0x800D5C74 nwWorkInitialize__Fv - clears the network resource work (C++ scope in the owner). */
void nwWorkInitialize(void);
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x800D32CC - sets the network resource manager up (the system core's memory layout runs it). */
void nw_resource_init(void);
#ifdef __cplusplus
}
#endif

#if defined(__cplusplus) && defined(MHTRI_NW_RESOURCE_OWNER)
/* The four nw4r::ef object kinds the effect memory manager pools, for the owner only: the element views its pools
 * construct (`new (block) T[n]` names each class' constructor, and the destructor where the owner's is C++) and the fields its garbage
 * collection walks.  Their full classes live in their own units (`ef/ef_effect.cpp`, `ef/ef_emitter.cpp`,
 * `ef/ef_particlemanager.cpp`, `ef/ef_particle.cpp`); these are views, sized from the pool strides. */
#include "ef/ef_effectsystem.h" /* EfSys, EfSysActivityList */

namespace nw4r {
namespace ef {

class Emitter;
class ParticleManager;

/* The pooled object's common head (nw4r's `ReferencedObject`): the pool link, the pool slot id whose high half
 * counts reuses, the life status and the reference count, then its vtable pointer. */
class PooledObject {
public:
    u32 GetRefCount() const { return mRefCount; }

    /* +0x00 */ u8 pad_0x00[0x08];  /* the pool link */
    /* +0x08 */ u32 mPoolId;        /* the slot index; the pool adds 0x10000 on each allocation */
    /* +0x0C */ s32 mLifeStatus;
    /* +0x10 */ u32 mRefCount;
    /* +0x14 */ u8 pad_0x14[0x0C];  /* the closing links and the vtable pointer */
}; /* size: 0x20 */

class Effect : public PooledObject {
public:
    Effect();
    ~Effect();

    /* +0x20 */ EfSys* mManagerES;
    /* +0x24 */ EfSysActivityList mEmitters;
    /* +0x40 */ u32 mGroupID;
    /* +0x44 */ u8 pad_0x44[0x60];
}; /* size: 0xA4 */

class Emitter : public PooledObject {
public:
    Emitter();

    /* +0x20 */ u8 pad_0x20[0x9C];
    /* +0xBC */ Effect* mEffect;
    /* +0xC0 */ EfSysActivityList mParticleManagers;
    /* +0xDC */ u8 pad_0xDC[0x78];
}; /* size: 0x154 */

/* Constructed through `ef_particle_manager_ctor`/`ef_particle_manager_delete`, its owner's C-linkage pair. */
class ParticleManager : public PooledObject {
public:
    /* +0x20 */ Emitter* mEmitter;
    /* +0x24 */ u8 pad_0x24[0x14];
    /* +0x38 */ EfSysActivityList mParticles;
    /* +0x54 */ u8 pad_0x54[0x68];
}; /* size: 0xBC */

class Particle : public PooledObject {
public:
    Particle();

    /* +0x20 */ u8 pad_0x20[0xA8];
    /* +0xC8 */ ParticleManager* mParticleManager;
    /* +0xCC */ u8 pad_0xCC[0x1C];
}; /* size: 0xE8 */

/* The effect draw state the resource manager allocates once (0xA0 bytes, `ef/ef_effectsystem.cpp`'s neighbour
 * defines the constructor). */
class DrawInfo {
public:
    DrawInfo();

    /* +0x00 */ MTX34 mViewMtx; /* the camera view matrix, refreshed each frame */
    /* +0x30 */ u8 pad_0x30[0x70];
}; /* size: 0xA0 */

}  // namespace ef
}  // namespace nw4r
#endif

#endif /* MHTRI_NW_RESOURCE_H */
