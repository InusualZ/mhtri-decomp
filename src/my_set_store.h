/* `my_set_store`, defined by `src/fn_8004CAD8.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MY_SET_STORE_H
#define MHTRI_MY_SET_STORE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Stores the worn equipment ids in saved equipment set `index`. */
/* untyped: the save block, seen by each caller through its own record view */
void my_set_store(void* userdata, u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MY_SET_STORE_H */
