/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `camera/camera_main.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_CAMERA_STAGE_DCM_BUFFER_TBL_H
#define MHTRI_CAMERA_STAGE_DCM_BUFFER_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x806BC1D0 - the demo-camera key buffers, one pointer per demo id (null while not loaded). */
extern struct CamDcmKey* stage_dcm_buffer_tbl[50];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_STAGE_DCM_BUFFER_TBL_H */
