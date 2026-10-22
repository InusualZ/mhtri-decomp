/* `my_set_clear`, defined by `src/fn_8004CAD8.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MY_SET_CLEAR_H
#define MHTRI_MY_SET_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Clears saved equipment set `index`. */
/* untyped: the save block, seen by each caller through its own record view */
void my_set_clear(void* userdata, u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MY_SET_CLEAR_H */
