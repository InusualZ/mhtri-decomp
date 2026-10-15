/*
 * homebutton/hbm_text_panel.h - declarations of the symbols owned by `homebutton/hbm_text_panel.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_TEXT_PANEL_H
#define HOMEBUTTON_HBM_TEXT_PANEL_H

#include "types.h"

#ifdef __cplusplus
class HbmTextBufferSet;
class HbmPanelView;
struct MEMAllocator;

extern "C" {
/* Builds the panel's three text buffers from the allocator. */
void hbmTextBuffersInit(HbmTextBufferSet* set, MEMAllocator* allocator);

/* Hides the writer's selection and caret and re-lays the panel's text out. */
void hbmPanelRelayout(HbmPanelView* self);
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object, typed by the callers' views */
void hbmPanelLayoutText(void* self);

/* untyped: opaque band object, typed by the callers' views */
void hbmPanelGetTextScale(void* self);

/* untyped: opaque band object, typed by the callers' views */
void hbmPanelDrawLineAt(void* self);

/* untyped: opaque band object, typed by the callers' views */
void hbmPanelWriterRefresh(void* self);

/* untyped: opaque band object, typed by the callers' views */
void* hbmPanelClearText(void* p);

/* untyped: opaque band object, typed by the callers' views */
void hbmPanelSetDrawPosition(void* self, void* arg);

/* untyped: opaque band object, typed by the callers' views */
void hbmPanelPrepareGlyph(void* self);

/* untyped: opaque band object, typed by the callers' views */
void hbmPanelNewLine(void* self);

/* untyped: opaque band object, typed by the callers' views */
void hbmPanelDrawComposition(void* self);

/* untyped: opaque band object, typed by the callers' views */
void hbmPanelDrawCaret(void* self);

/* untyped: opaque band object, typed by the callers' views */
void hbmPanelCloseCandidates(void* self);

/* untyped: opaque band object, typed by the callers' views */
void fn_8054CF38(void* self, s32 flag);

/* untyped: opaque band object, typed by the callers' views */
void hbmKbWidgetReset(void* self);

/* untyped: opaque band object, typed by the callers' views */
void hbmKbWidgetCalc(void* self);

/* untyped: opaque band object, typed by the callers' views */
void hbmKbWidgetUpdate(void* self);

/* untyped: opaque band object, typed by the callers' views */
void hbmKbWidgetInput(void* self);

/* untyped: opaque band object, typed by the callers' views */
void fn_8054E05C(void* panel, struct HkbManager* manager);

/* untyped: opaque band object, typed by the callers' views */
void hbmPanelGetTextBoxScale(void* self);

/* untyped: opaque band object, typed by the callers' views */
void fn_8054E81C(void* panel);

/* untyped: opaque band object, typed by the callers' views */
void fn_8054E858(void* panel);

/* Callees inside the unit, declared ahead of their bodies (address order). */
/* untyped: opaque band object, typed by the callers' views */
void fn_8054F128(void* self);

/* untyped: opaque band object, typed by the callers' views */
void fn_8054F12C(void* self);

/* untyped: opaque band object, typed by the callers' views */
void fn_8054F130(void* self);

extern u8  lbl_8064F0B0[];   /* the vtable/type record a factory stores into its fresh object */

/* Returns the spacing character of the dead key `c` (the key the keyboard animates for it). */
u16 hbmKbDeadKeyToSpacing(u16 c);

/* Clears the panel's candidate selection. */
void hbmPanelResetCandidate(struct HbmTextPanel* self);

#ifdef __cplusplus
}
#endif

#endif
