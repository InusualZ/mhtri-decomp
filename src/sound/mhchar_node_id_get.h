/* `mhchar_node_id_get`, defined by `sound/mhchar.cpp` (leaf header, rule 2). */
#ifndef MHTRI_SOUND_MHCHAR_NODE_ID_GET_H
#define MHTRI_SOUND_MHCHAR_NODE_ID_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the id of the model node named `name` of `chr`, 0 when it has none. */
u32 mhchar_node_id_get(struct MHchar* chr, const char* name);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_NODE_ID_GET_H */
