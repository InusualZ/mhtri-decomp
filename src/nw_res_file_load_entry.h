/* Leaf header (docs/plan.md 6.5 rule 2): `nw_resource.cpp`'s `nw_res_file_load_entry`, for `Pl/player_control.cpp`. */
#ifndef MHTRI_NW_RES_FILE_LOAD_ENTRY_H
#define MHTRI_NW_RES_FILE_LOAD_ENTRY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D5C4C (0x28): queues the read of one resource table entry's file into `path` (the destination). */
void nw_res_file_load_entry(char* path, u32* entry);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NW_RES_FILE_LOAD_ENTRY_H */
