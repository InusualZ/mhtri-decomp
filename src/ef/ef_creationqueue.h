/* ef/ef_creationqueue.h - the creation queue `ef/ef_creationqueue.cpp` owns: the setting record a queued
 * creation copies, the queue and its entry, and the two `Add` paths (C linkage). */
#ifndef MHTRI_EF_EF_CREATIONQUEUE_H
#define MHTRI_EF_EF_CREATIONQUEUE_H

#include "types.h"
#include "mh3_pad.h"

/* The 0xA-byte effect setting record copied into a queue entry (fn_800A3390 copies it field by field:
 * a `lha` for the first halfword, then eight byte moves at +0x2..+0x9). */
struct Setting {
    /* +0x00 */ s16 mId;
    /* +0x02 */ u8 mArg0;
    /* +0x03 */ u8 mArg1;
    /* +0x04 */ u8 mArg2;
    /* +0x05 */ u8 mArg3;
    /* +0x06 */ u8 mArg4; /* the creation kind: 0 queues a type-0 entry, otherwise a type-1 one */
    /* +0x07 */ u8 mArg5;
    /* +0x08 */ u8 mArg6;
    /* +0x09 */ u8 mArg7;
}; /* size: 0xA */

struct EffectHandle;
struct EffectManager;

/* A pending effect-creation record (stride 0x30). */
struct CreationQueueEntry {
    /* +0x00 */ u8 mType;
    /* +0x01 */ u8 mFlags;
    /* +0x02 */ u16 mLife;
    /* +0x04 */ Setting mSetting;
    /* +0x0E */ u8 pad_0x0E[0x2];
    /* +0x10 */ EffectManager* mpManager;
    /* +0x14 */ EffectHandle* mpHandle;
    /* +0x18 */ Vec3 mPos;
    /* +0x24 */ Vec3 mVel;
}; /* size: 0x30 */

/* The queue: a live-entry count and the 0x400-entry array right behind it. */
struct CreationQueue {
    /* +0x00 */ s32 mCount;
    /* +0x04 */ CreationQueueEntry mEntry[0x400];
}; /* size: 0xC004 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A337C - a referenced object's AddRef: increments its count and returns it. */
u32 ef_ref_object_add_ref(EffectManager* object);

/* 0x800A3044 (0x338): queues a type-0 (particle) creation. */
void ef_creation_queue_add_type0(CreationQueue* self, const Setting* setting, EffectManager* manager,
                                 EffectHandle* eh, u16 life, const Vec3* pos, const Vec3* vel);
/* 0x800A33DC (0x33C): queues a type-1 (emitter) creation (the same shape). */
void ef_creation_queue_add_type1(CreationQueue* self, const Setting* setting, EffectManager* manager,
                                 EffectHandle* eh, u16 life, const Vec3* pos, const Vec3* vel);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_CREATIONQUEUE_H */
