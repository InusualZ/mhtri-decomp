/* Leaf header (docs/plan.md 6.5 rule 2): `eft053_shell_pos_project`, defined by `ef/eft053.cpp`. */
#ifndef MHTRI_EF_EFT053_SHELL_POS_PROJECT_H
#define MHTRI_EF_EFT053_SHELL_POS_PROJECT_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80366618: projects the player's shell frame into a world position. */
void eft053_shell_pos_project(_PLW* plw, VEC3* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT053_SHELL_POS_PROJECT_H */
