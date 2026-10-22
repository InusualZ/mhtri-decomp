/* Leaf header (docs/plan.md 6.5 rule 2): `em012_prog_tbl`, which `enemy/em012_prog` defines - the monster program table(s)
 * `enemy/enemy_control.cpp`'s `em_prog_tbl_list` lists (the owner gives each its own record type; this
 * view is the table's words). */
#ifndef MHTRI_ENEMY_EM012_PROG_TBL_H
#define MHTRI_ENEMY_EM012_PROG_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u32 em012_prog_tbl[];

#ifdef __cplusplus
}
#endif

#endif
