/* userdata_arena_rows_export.h - leaf header (docs/plan.md 6.5 rule 2) for `fn_8004CAD8.cpp`'s arena-row copies.
 *   C linkage (the map rows are plain names). */
#ifndef MHTRI_USERDATA_ARENA_ROWS_EXPORT_H
#define MHTRI_USERDATA_ARENA_ROWS_EXPORT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004EC44 (0x68): copies the save's twelve arena rows out to `dst`. */
void userdata_arena_rows_export(u8* dst);
/* 0x8004ECAC (0x68): copies twelve arena rows from `src` back into the save. */
void userdata_arena_rows_import(const u8* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ARENA_ROWS_EXPORT_H */
