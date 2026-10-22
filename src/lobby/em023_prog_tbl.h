/* Leaf header (docs/plan.md 6.5 rule 2): `em023_prog_tbl`, which `lobby/fn_802FA9A0` defines - the monster program table(s)
 * `enemy/enemy_control.cpp`'s `em_prog_tbl_list` lists (the owner gives each its own record type; this
 * view is the table's words). */
#ifndef MHTRI_LOBBY_EM023_PROG_TBL_H
#define MHTRI_LOBBY_EM023_PROG_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u32 em023_prog_tbl[];

#ifdef __cplusplus
}
#endif

#endif
