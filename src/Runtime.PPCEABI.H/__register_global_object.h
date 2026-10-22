/* Leaf header (docs/plan.md 6.5 rule 2): `Runtime.PPCEABI.H/global_destructor_chain.c`'s `__register_global_object`
 * and its chain node type, for a consumer that registers a global by hand.  C linkage (the map row is a plain name). */
#ifndef MHTRI_RUNTIME_PPCEABI_H___REGISTER_GLOBAL_OBJECT_H
#define MHTRI_RUNTIME_PPCEABI_H___REGISTER_GLOBAL_OBJECT_H

#include "types.h"

typedef struct DestructorChain DestructorChain;   /* `global_destructor_chain.c` */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804566A4 - pushes `chain` on the global destructor chain, to call `destructor(object, -1)` at exit. */
/* untyped: an opaque handle passed through - the registered object */
void __register_global_object(void* object, void (*destructor)(void*, int), DestructorChain* chain);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RUNTIME_PPCEABI_H___REGISTER_GLOBAL_OBJECT_H */
