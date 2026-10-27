/* ef/resident_file_load.h - leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s resident-file load and work
 * stack reset, for `fn_80059550.cpp`.  C linkage (the map rows are plain names). */
#ifndef MHTRI_EF_RESIDENT_FILE_LOAD_H
#define MHTRI_EF_RESIDENT_FILE_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800CFBF0 (0x74): loads the resident files. */
void resident_file_load(void);
/* 0x800D00EC (0x24): resets the work stack. */
void work_stack_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_RESIDENT_FILE_LOAD_H */
