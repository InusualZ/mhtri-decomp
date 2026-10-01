/*
 * homebutton/hbm_value.h - declarations of the symbols owned by `homebutton/hbm_value.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_VALUE_H
#define HOMEBUTTON_HBM_VALUE_H

#include "types.h"
#include "homebutton/hbm_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* r3 is the address the caller hands over (the +0x1A04 record of the panel at 0x805482CC and the
 * sub-object at its +0x10); `fn_8055A3E0` additionally takes the mode word the caller materialises. */
/* untyped: opaque band object, typed by the callers' views */
void fn_8055A3E0(void* sub, s32 mode);

u32 fn_8055A888(HbmWidget* self, int arg);

/* untyped: opaque band object, typed by the callers' views */
void* fn_8055B174(void* a, void* b);

/* untyped: opaque band object, typed by the callers' views */
void  fn_8055B7D4(void* sub, s32 flag);

/* untyped: opaque band object, typed by the callers' views */
void* fn_8055B898(void* self, int flag);

/* untyped: opaque band object, typed by the callers' views */
void* fn_8055B8F0(void* o, void* a, u32 b);

/* untyped: opaque band object, typed by the callers' views */
void fn_8055BBAC(void* node);

/* untyped: opaque band object, typed by the callers' views */
u32 fn_8055BEDC(void* current);

/* untyped: opaque band object, typed by the callers' views */
u32 fn_8055BEF0(void* self);

/* untyped: opaque band object, typed by the callers' views */
void fn_8055C1D4(void* record);

/* untyped: opaque band object, typed by the callers' views */
void fn_8055C2CC(void* record);

/* untyped: opaque band object, typed by the callers' views */
void* fn_8055C494(void* self, int flag);

/* untyped: opaque band object, typed by the callers' views */
void fn_8055C638(void* self);

extern f32 lbl_8079D700;

#ifdef __cplusplus
}
#endif

#endif
