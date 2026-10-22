/* Leaf header (docs/plan.md 6.5 rule 2): `nw_res_entry_get`, which `nw_resource` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_NW_RES_ENTRY_GET_H
#define MHTRI_NW_RES_ENTRY_GET_H

#include "types.h"
#include "nw_resource.h" /* ResEntry */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D5418 - the resource entry of a file handle. */
ResEntry* nw_res_entry_get(s32 handle);

#ifdef __cplusplus
}
#endif

#endif
