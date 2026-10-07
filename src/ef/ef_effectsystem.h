/*
 * ef/ef_effectsystem.h - the effect system singleton `EfSys` (nw4r's `EffectSystem`; `ef.h` still carries a stub
 *   under that name, so this layout keeps its own) and the group records it owns.  The system's constructor and
 *   destructor are `ef/ef_effect.cpp`'s, its instance and the rest of its members `ef/ef_effectsystem.cpp`'s.
 */
#ifndef MHTRI_EF_EF_EFFECTSYSTEM_H
#define MHTRI_EF_EF_EFFECTSYSTEM_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/ef_random.h"

/* The library's list record: head, tail, live count and the link offset `fn_800A4030` sets. */
typedef struct EfSysList {
    /* +0x00 */ void* head;
    /* +0x04 */ void* tail;
    /* +0x08 */ u16 numObjects;
    /* +0x0A */ u16 linkOffset;
} EfSysList; /* size: 0x0C */

/* One `mActivityList` group record: the group's live list, its retiring list and the live count.  The
 * assert names it (`mActivityList[group].mActiveList`). */
typedef struct EfSysActivityList {
    /* +0x00 */ EfSysList mActiveList;
    /* +0x0C */ EfSysList mRetireList;
    EfSysActivityList(); /* defined inline by ef/ef_effectsystem.cpp */

    /* +0x18 */ u16 mNumActive;
    /* +0x1A */ u16 pad_0x1A;
} EfSysActivityList; /* size: 0x1C */

class EfSysEffect;

/* The allocator/pool object the system was handed (`fn_800D3D48` stores it into the system's +0x00): the
 * effect pool at table slots +0x10/+0x14 and the byte allocator at +0x60; the slots between are declared only
 * to place it.  Declared only, so no unit emits its table. */
class EfSysMemoryManager {
public:
    /* +0x00: the vtable pointer */
    virtual void slot_0x08();
    virtual void slot_0x0C();
    virtual EfSysEffect* GetEffect();
    virtual void ReleaseEffect(EfSysEffect* effect);
    virtual void slot_0x18();
    virtual void slot_0x1C();
    virtual void slot_0x20();
    virtual void slot_0x24();
    virtual void slot_0x28();
    virtual void slot_0x2C();
    virtual void slot_0x30();
    virtual void slot_0x34();
    virtual void slot_0x38();
    virtual void slot_0x3C();
    virtual void slot_0x40();
    virtual void slot_0x44();
    virtual void slot_0x48();
    virtual void slot_0x4C();
    virtual void slot_0x50();
    virtual void slot_0x54();
    virtual void slot_0x58();
    virtual void slot_0x5C();
    /* untyped: byte range - the allocator hands out raw storage */
    virtual void* Alloc(u32 size);
}; /* size: 0x04 (lower bound: the manager's own state is not this unit's) */

/* The effect system.  Only the fields the bodies touch are named; the size is the symbol map's own for the
 * instance. */
class EfSys {
public:
    EfSys();
    ~EfSys();

    /* +0x0000 */ EfSysMemoryManager* mMemoryManager;
    /* +0x0004 */ void* mDrawOrder;     /* the draw-order list object */
    /* +0x0008 */ void* mStrategy;      /* the draw-strategy builder */
    /* +0x000C */ void* mLineStrategy;  /* the emitter-form builder */
    /* +0x0010 */ u8 mCreationQueue[0xC004]; /* the queue the constructor builds (`fn_800A2FA4`) */
    /* +0xC014 */ u32 mMaxGroupID;
    /* +0xC018 */ EfSysActivityList* mActivityList;
    /* +0xC01C */ nw4r::ef::Random mRandom;
    /* +0xC020 */ nw4r::math::VEC3 mRefPos; /* copyVec3 copies the caller's vector into it */
    /* +0xC02C */ nw4r::math::MTX34 mRefMtx;
    /* +0xC05C */ f32 mRangeB;
    /* +0xC060 */ f32 mRangeA;
    /* +0xC064 */ u8 mField_0xC064;     /* cleared by the constructor */
    /* +0xC065 */ u8 pad_0xC065[0x03];
}; /* size: 0xC068 */

/* The system's one instance (`ef/ef_effectsystem.cpp`). */
extern EfSys ef_system_instance;

/* Set once the library version has been registered (the system's constructor tests and sets it). */
extern u32 ef_system_version_registered;

#endif /* MHTRI_EF_EF_EFFECTSYSTEM_H */
