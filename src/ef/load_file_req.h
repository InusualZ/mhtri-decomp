/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `load_file_req` (0x800CEAB0), the asynchronous file read `ef/system_core.cpp`
 * owns: `name` is read into `dst` (`size` bytes) and `callback` runs with `ctx` once it is done.  C++ scope: the map row
 * is the mangling `load_file_req__FPcUllUllPUl`.
 */
#ifndef MHTRI_EF_LOAD_FILE_REQ_H
#define MHTRI_EF_LOAD_FILE_REQ_H

#include "types.h"

#ifdef __cplusplus
void load_file_req(char* name, u32 dst, s32 size, u32 callback, s32 flag, u32* ctx);
#endif

#endif /* MHTRI_EF_LOAD_FILE_REQ_H */
