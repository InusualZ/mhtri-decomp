/* The master-band declarations of `Pl/pl_act.cpp` that the player-record consumers include as one set.
 *
 * The C-linkage ones (`fn_8026FD94` ... `pl_act_set_flag`) live in `Pl/fn_8026FD94.h`, which this header
 * includes; only the C++-linkage `Pl_master_ck`/`Pl_act_ck` are declared here.
 */
#ifndef MHTRI_PL_PL_MASTER_CK_H
#define MHTRI_PL_PL_MASTER_CK_H

#include "types.h"
#include "nw4r/math.h"
#include "Pl/fn_8026FD94.h"

struct _PLW;

/* `Pl/pl_act.cpp` defines it at C++ scope and the target object references
 * `Pl_master_ck__FP4_PLW`, so it is declared with C++ linkage (relocaudit). */
#ifdef __cplusplus
u32 Pl_master_ck(struct _PLW* plw);
u32 Pl_act_ck(struct _PLW* self, u8 group, u16 action);
#endif

#endif /* MHTRI_PL_PL_MASTER_CK_H */
