/* Pl/pl_act6_class_ck.h - leaf header: `pl_act6_class_ck` (0x8027E06C), owned by `Pl/pl_act.cpp`. */
#ifndef MHTRI_PL_PL_ACT6_CLASS_CK_H
#define MHTRI_PL_PL_ACT6_CLASS_CK_H
#include "types.h"
struct _PLW;
#ifdef __cplusplus
extern "C" {
#endif
/* Returns 1 when the player's act kind is 6 and its act number is in the class `cls` (0..2) lists. */
s32 pl_act6_class_ck(struct _PLW* self, s32 cls);
#ifdef __cplusplus
}
#endif
#endif /* MHTRI_PL_PL_ACT6_CLASS_CK_H */
