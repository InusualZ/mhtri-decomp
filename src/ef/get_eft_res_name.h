/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft_res.cpp`'s get_eft_res_name (C++ linkage, `get_eft_res_name__FUs`). */
#ifndef MHTRI_EF_GET_EFT_RES_NAME_H
#define MHTRI_EF_GET_EFT_RES_NAME_H

#include "types.h"

#ifdef __cplusplus
/* 0x800F91A0 - the resource name of effect model `id`. */
char* get_eft_res_name(u16 id);
#endif

#endif /* MHTRI_EF_GET_EFT_RES_NAME_H */
