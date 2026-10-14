/* BTE/sdp_api.h - the SDP client API entry points `BTE/sdp_api.c` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_SDP_API_H
#define MHTRI_BTE_SDP_API_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804A0290 - sets the SDP trace level (0xFF only reads it); returns the level. */
u8 SDP_SetTraceLevel(u8 new_level);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_SDP_API_H */
