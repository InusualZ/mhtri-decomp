/*
 * homebutton/hbm_kb_event.h - declarations of the symbols owned by `homebutton/hbm_kb_event.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_KB_EVENT_H
#define HOMEBUTTON_HBM_KB_EVENT_H

#include "types.h"
#include "homebutton/hbm_vu_object.h"

#ifdef __cplusplus
extern "C" {
#endif

void fn_8056083C(VuObject* self, VuSub* arg);

/* The two dispatchers of the registered neighbour `homebutton/keyboard.cpp` that fn_8056329C tail-calls. */
/* untyped: opaque band object, typed by the callers' views */
void fn_805632BC(void* self);

/* untyped: opaque band object, typed by the callers' views */
void fn_805634A4(void* self);

extern f32 lbl_807946D8[2];  /* a small-data record whose float at +4 is written by fn_8056167C */

#ifdef __cplusplus
}
#endif

#endif
