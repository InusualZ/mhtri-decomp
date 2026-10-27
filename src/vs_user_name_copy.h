/*
 * `vs_user_name_copy` (0x8004F2FC), defined by `src/fn_8004CAD8.cpp` (leaf header, rule 2): copies the versus user
 * `index`'s name into a player work.
 */
#ifndef MHTRI_VS_USER_NAME_COPY_H
#define MHTRI_VS_USER_NAME_COPY_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

void vs_user_name_copy(struct _PLW* plw, u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_VS_USER_NAME_COPY_H */
