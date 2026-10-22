/* Leaf header (docs/plan.md 6.5 rule 2): `em_file_name_tbl`, which `draw_shape` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_DRAW_SHAPE_EM_FILE_NAME_TBL_H
#define MHTRI_DRAW_SHAPE_EM_FILE_NAME_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8058A9C8 - per monster kind the name of its file ("emNN..."). */
extern char* em_file_name_tbl[];

#ifdef __cplusplus
}
#endif

#endif
