/* ef/ef_creationqueue.h - declarations (C linkage) of the `ef/ef_creationqueue.cpp` symbols other units call: the two
 * queue `Add` paths.  The record types are the owner's (declared by name here). */
#ifndef MHTRI_EF_EF_CREATIONQUEUE_H
#define MHTRI_EF_EF_CREATIONQUEUE_H

#include "types.h"
#include "nw4r/math.h"

struct CreationQueue;
struct Setting;
struct EffectManager;
struct EffectHandle;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A3044 (0x338): queues a type-0 creation. */
void ef_creation_queue_add_type0(struct CreationQueue* self, const struct Setting* setting,
                                 struct EffectManager* manager, struct EffectHandle* eh, u16 life, const Vec3* pos,
                                 const Vec3* vel);
/* 0x800A33DC (0x33C): queues a type-1 creation (the same shape). */
void ef_creation_queue_add_type1(struct CreationQueue* self, const struct Setting* setting,
                                 struct EffectManager* manager, struct EffectHandle* eh, u16 life, const Vec3* pos,
                                 const Vec3* vel);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_CREATIONQUEUE_H */
