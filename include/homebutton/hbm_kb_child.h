/*
 * homebutton/hbm_kb_child.h - declarations of the symbols owned by `homebutton/hbm_kb_child.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_KB_CHILD_H
#define HOMEBUTTON_HBM_KB_CHILD_H

#include "types.h"
#include "homebutton/hbm_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Callees inside the band, declared ahead of their bodies (address order). */
/* untyped: opaque band object, typed by the callers' views */
void fn_80555B0C(void* self, s32 flag);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80555BF4(void* self, int flag);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80555F5C(void* self, int flag);

void fn_80555FB4(HbmWidget* self, int flag);

void fn_805563D0(HbmWidget* self, int flag);

#ifdef __cplusplus
}
#endif

#endif
