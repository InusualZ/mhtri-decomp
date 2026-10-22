/* userdata_online_first_set.h - leaf header (docs/plan.md 6.5 rule 2): `fn_8004CAD8.cpp`'s
 *   `userdata_online_first_set` (0x8004DDA4), for `lobby/fn_802FA9A0.cpp`.  C linkage.  GUESS name (registry). */
#ifndef MHTRI_USERDATA_ONLINE_FIRST_SET_H
#define MHTRI_USERDATA_ONLINE_FIRST_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004DDA4: sets the user block's first-online mark (+0x3DE4); 1 when it was clear, 0 when it was already set. */
s32 userdata_online_first_set(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ONLINE_FIRST_SET_H */
