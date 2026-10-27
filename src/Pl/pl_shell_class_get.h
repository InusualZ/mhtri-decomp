/* Leaf header (docs/plan.md 6.5 rule 2): `pl_shell_class_get` (0x80279490), defined by `src/Pl/pl_act.cpp`. */
#ifndef MHTRI_PL_PL_SHELL_CLASS_GET_H
#define MHTRI_PL_PL_SHELL_CLASS_GET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

s32 pl_shell_class_get(struct _PLW* self, u8 slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SHELL_CLASS_GET_H */
