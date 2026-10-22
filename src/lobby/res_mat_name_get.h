/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_equip_page.cpp`'s res_mat_name_get */
#ifndef MHTRI_LOBBY_RES_MAT_NAME_GET_H
#define MHTRI_LOBBY_RES_MAT_NAME_GET_H

#include "types.h"
#include "g3d/g3d_resmat.h"

#ifdef __cplusplus
extern "C" {

/* 0x802235A4 - the material's name (nw4r's inline `ResMat::GetName`; this is the kept copy), NULL when unnamed. */
const char* res_mat_name_get(const nw4r::g3d::ResMat* mat);
}
#endif

#endif /* MHTRI_LOBBY_RES_MAT_NAME_GET_H */
