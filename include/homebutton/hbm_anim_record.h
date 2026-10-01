/*
 * homebutton/hbm_anim_record.h - declarations of the symbols owned by `homebutton/hbm_anim_record.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_ANIM_RECORD_H
#define HOMEBUTTON_HBM_ANIM_RECORD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object, typed by the callers' views */
void fn_80557228(void* self, s32 flag);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80557310(void* self, int flag);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80557350(void* self, int flag);

/* untyped: opaque band object, typed by the callers' views */
void* fn_805576EC(void* self, int flag);

/* untyped: opaque band object, typed by the callers' views */
void fn_80557744(void* self);

/* untyped: opaque band object, typed by the callers' views */
void fn_80557E8C(void* self);

extern const u8 lbl_8057BD60[];  /* the animation-record table of fn_80556FC4 */

#ifdef __cplusplus
}
#endif

#endif
