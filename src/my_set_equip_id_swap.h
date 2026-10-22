/* `my_set_equip_id_swap`, defined by `src/fn_8004CAD8.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MY_SET_EQUIP_ID_SWAP_H
#define MHTRI_MY_SET_EQUIP_ID_SWAP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Swaps two box record ids in the 20 saved equipment sets. */
/* untyped: the save block, seen by each caller through its own record view */
void my_set_equip_id_swap(void* userdata, u16 id_a, u16 id_b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MY_SET_EQUIP_ID_SWAP_H */
