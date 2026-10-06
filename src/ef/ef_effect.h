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

/* 0x800A43E8 - appends `node` to an activity list's active list and counts it. */
/* untyped: opaque handle - the list and its node are each caller's own record views */
void ef_activity_list_add(void* list, void* node);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_EFFECT_H */
