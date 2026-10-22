/* `my_set_used_ck`, defined by `src/fn_8004CAD8.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MY_SET_USED_CK_H
#define MHTRI_MY_SET_USED_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Whether saved equipment set `index` holds a weapon. */
/* untyped: the save block, seen by each caller through its own record view */
u32 my_set_used_ck(void* userdata, u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MY_SET_USED_CK_H */
