/* Leaf header for `mhchar_construct` (0x801FF984), owned by `lobby/lb_npc.cpp`: the constructor of the model object `MHchar`.
 */
struct MHchar;

#ifndef MHTRI_LOBBY_MHCHAR_CONSTRUCT_H
#define MHTRI_LOBBY_MHCHAR_CONSTRUCT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constructs the model object `self` points at. */
void mhchar_construct(struct MHchar* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_MHCHAR_CONSTRUCT_H */
