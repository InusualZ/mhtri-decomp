/*
 * homebutton/hbm_kb_widget.h - declarations of the symbols owned by `homebutton/hbm_kb_widget.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_KB_WIDGET_H
#define HOMEBUTTON_HBM_KB_WIDGET_H

#include "types.h"
#include "homebutton/fn_8054E894.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object, typed by the callers' views */
void fn_8054F90C(void* self, s32 flag);

void fn_805504EC(HkbWidget* self);

void fn_80550770(HkbWidget* self);

void fn_80550804(HkbWidget* self);

void fn_805516E4(HkbWidget* self);

void fn_80551EB0(HkbWidget* self);

void fn_80551F08(HkbWidget* self);

/* untyped: opaque band object, typed by the callers' views */
void fn_80553074(void* self, s32 value);

void fn_805536FC(HkbWidget* self);

void fn_805542BC(HkbWidget* self);

#ifdef __cplusplus
}
#endif

#endif
