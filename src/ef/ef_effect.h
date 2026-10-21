/* ef/ef_effect.h - the symbols `ef/ef_effect.cpp` owns that other units call (C linkage). */
#ifndef MHTRI_EF_EF_EFFECT_H
#define MHTRI_EF_EF_EFFECT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The emitter description record a resource's body holds; each consumer reads it through its own view. */
struct EfEmitterDesc;

/* 0x800A4864 - the emitter description behind a resource (the record past its 8-byte header). */
/* untyped: opaque handle - the resource object is passed through under each caller's own view */
struct EfEmitterDesc* ef_res_emitter_desc(void* res);

/* The effect system's memory manager (an object whose first word is its virtual table). */
struct EfMemoryManager;

/* 0x800A4420 - the effect system's memory manager (the system record's first word). */
/* untyped: opaque handle - the effect system is passed through under each caller's own view */
struct EfMemoryManager* ef_system_memory_manager(void* system);

/* 0x800A4428 - empties an activity list (both lists and the count). */
/* untyped: opaque handle - the list is each caller's own record view */
void ef_activity_list_clear(void* list);

/* 0x800A444C - a referenced object's initialisation: no link, state 1 (live). */
/* untyped: opaque handle - the object is each caller's own record view */
u32 ef_ref_object_init(void* self);

/* 0x800A5114 - sets or clears the effect's calc flag (bit 16 of its flags word). */
/* untyped: opaque handle - the effect is each caller's own record view */
void ef_effect_set_calc_flag(void* effect, u32 on);

/* 0x800A43E8 - appends `node` to an activity list's active list and counts it. */
/* untyped: opaque handle - the list and its node are each caller's own record views */
void ef_activity_list_add(void* list, void* node);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace nw4r {
namespace ef {

/* The reference-counted base of every pooled ef object (effect, emitter, particle manager, particle): the pool
 * link, the life status and the reference count, then its virtual table at +0x1C.  `DestroyFunc` is its key
 * function (`ef/ef_effect.cpp`), so the table is emitted there; `SendClosing`'s kept copy is
 * `ef/ef_creationqueue.cpp`'s. */
struct ReferencedObject {
    /* +0x00 */ u8 pad_0x00[0x0C];  /* the pool link the memory manager chains it by */
    /* +0x0C */ s32 mLifeStatus;    /* 1 = active, 2 = closing, 3 = retired */
    /* +0x10 */ u32 mRefCount;
    /* +0x14 */ u8 pad_0x14[0x08];
    /* +0x1C: the vtable pointer */
    virtual void SendClosing() {}
    virtual void DestroyFunc();
}; /* size: 0x20 */

} // namespace ef
} // namespace nw4r

#ifdef MHTRI_EF_EFFECT_OWNER
/* The full effect class, for its owner only: every other unit still reaches the effect through `ef.h`'s stub of
 * the same class until the two are folded (request ef-data#1). */
#include "nw4r/math.h"
#include "ef/ef_draworder.h" /* nw4r::ef::DrawOrderBase, the effect's draw-order object (rule 1) */

/* `nw4r::ut::List` as the ef units build it: head, tail, live count and the link offset. */
typedef struct EfEffList {
    /* +0x00 */ void* head;
    /* +0x04 */ void* tail;
    /* +0x08 */ u16 numObjects;
    /* +0x0A */ u16 linkOffset;
} EfEffList; /* size: 0xC */

/* The effect's emitter group: the live list, the retired list and the live count. */
typedef struct EfEffActivityList {
    /* +0x00 */ EfEffList mActiveList;
    /* +0x0C */ EfEffList mRetireList;
    /* +0x18 */ u16 mNumActive;
    /* +0x1A */ u16 pad_0x1A;
} EfEffActivityList; /* size: 0x1C */

/* The object the effect keeps at +0x20 (`mManagerES`): its first word is the record `ef_system_memory_manager`
 * dereferences and its +0x04 word is the effect's +0xA0 back-pointer. */
typedef struct EfEffManager {
    /* +0x00 */ void* field_0x00;
    /* +0x04 */ void* field_0x04;
    /* +0x08 */ u8 pad_0x08[0x08];
    /* +0x10 */ u32 field_0x10; /* the pool link fn_800A5104 tests and fn_800A3718 resets */
} EfEffManager; /* size: 0x14 (lower bound: only +0x00/+0x04/+0x10 are touched) */

namespace nw4r {
namespace ef {

class EffectSystem;
class EmitterResource;

/* The emitter resource handle `CreateEmitter` takes by value (`ResEmitter`): one pointer to the resource. */
struct ResEmitter {
    /* +0x00 */ EmitterResource* mData;
}; /* size: 0x4 */

/* The pooled effect object: the reference-counted base, the effect system that owns it, its emitter group, the
 * root matrix and velocity, the draw list and the draw-order object. */
class Effect : public ReferencedObject {
public:
    Effect();
    ~Effect();

    virtual void SendClosing();
    virtual void DestroyFunc();
    virtual u32 Initialize(EffectSystem* system, EmitterResource* resource, u16 calcRemain);
    virtual u32 CreateEmitter(ResEmitter resource, u8 drawWeight, u16 calcRemain);
    virtual void Calc(u32 flag);
    virtual void Draw(const EfDrawInfo* info);

    void SetRootMtx(const nw4r::math::MTX34& mtx);
    u32 RetireEmitterAll();
    /* untyped: caller-owned payload - the callback gets each particle manager under its own view */
    u32 ForeachParticleManager(void (*cb)(void*, u32), u32 arg, bool flag);

    /* +0x20 */ EfEffManager* mManagerES; /* the effect system that owns it */
    /* +0x24 */ EfEffActivityList mEmitters;
    /* +0x40 */ u32 mGroupID;
    /* +0x44 */ u8 pad_0x44[0x04];
    /* +0x48 */ u32 mField_0x48; /* four words cleared by Initialize */
    /* +0x4C */ u32 mField_0x4C;
    /* +0x50 */ u32 mField_0x50;
    /* +0x54 */ u32 mFlags_0x54; /* bit 0, bit 1 and bit 16 the accessors touch */
    /* +0x58 */ MTX34 mRootMtx; /* SetRootMtx writes it */
    /* +0x88 */ VEC3 mVelocity; /* added to each new particle's (`ef/ef_particle.cpp`) */
    /* +0x94 */ EfEffList mDrawList; /* the particle managers in draw order (`ef/ef_draworder.cpp`) */
    /* +0xA0 */ DrawOrderBase* mDrawOrder; /* the effect system's draw-order object Draw dispatches to */
}; /* size: 0xA4 (lower bound: +0xA0 is the highest field any body touches) */

} // namespace ef
} // namespace nw4r
#endif /* MHTRI_EF_EFFECT_OWNER */
#endif

#endif /* MHTRI_EF_EF_EFFECT_H */
