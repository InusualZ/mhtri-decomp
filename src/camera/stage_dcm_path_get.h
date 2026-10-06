/* Leaf header (docs/plan.md 6.5 rule 2): `camera/camera_main.cpp` symbols `quest/quest_entry.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names).  GUESS names, derived from the
 * call sites and bodies. */
#ifndef MHTRI_CAMERA_STAGE_DCM_PATH_GET_H
#define MHTRI_CAMERA_STAGE_DCM_PATH_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802BECB8 - writes stage `stage`'s `07/dcm%03d.bin` archive path into `path`. */
void stage_dcm_path_get(char* path, u8 stage);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_STAGE_DCM_PATH_GET_H */
