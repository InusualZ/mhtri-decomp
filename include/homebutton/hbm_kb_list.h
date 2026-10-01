/*
 * homebutton/hbm_kb_list.h - declarations of the symbols owned by `homebutton/hbm_kb_list.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_KB_LIST_H
#define HOMEBUTTON_HBM_KB_LIST_H

#include "types.h"
#include "homebutton/hbm_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object, typed by the callers' views */
void fn_80554714(void* self, s32 flag);

/* untyped: opaque band object, typed by the callers' views */
void fn_80554BA8(void* self);

extern HbmWidget lbl_806501A0;   /* the band's static widget instance */

/* untyped: opaque band object, typed by the callers' views */
extern void* lbl_80794608;       /* widget-string slot the static initialiser copies */

/* untyped: opaque band object, typed by the callers' views */
extern void* lbl_8079460C;       /* widget-string slot the static initialiser copies */

/* The layout strings the band passes to its widgets (`.sdata` words; their band is unregistered). */
extern char lbl_80794610[5];     /* "N_UP" */

extern char lbl_80794618[7];     /* "N_DOWN" */

#ifdef __cplusplus
}
#endif

#endif
