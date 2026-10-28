/*
 * homebutton/hbm_kb_widget.h - declarations of the symbols owned by `homebutton/hbm_kb_widget.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_KB_WIDGET_H
#define HOMEBUTTON_HBM_KB_WIDGET_H

#include "types.h"
#include "homebutton/hkb_widget.h"

class HbmKbWidget;
class HbmKbCandidateList;
class HbmKbSwitchHandler;
class HbmKbPartView;
struct HbmKbInputEvent;

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object - the widget or a class derived from it */
void* hbmKbWidgetDestroy(void* self, s32 flag);

void hbmKbWidgetShowPredictInput(HbmKbWidget* self);

void hbmKbWidgetDrawCandidates(HbmKbWidget* self);

void hbmKbWidgetUpdateCandidates(HbmKbWidget* self);

void hbmKbWidgetHandleEvent(HbmKbWidget* self, u32 sender, s32 event, s32 arg);

void fn_80551EB0(HkbWidget* self);

void fn_80551F08(HkbWidget* self);

void hbmKbCandScrollBy(HbmKbCandidateList* self, s32 delta);

void hbmKbCandOnScrolled(HbmKbCandidateList* self, s32 kind, s32 delta);

void hbmKbCandSelect(HbmKbCandidateList* self, s32 index);

void hbmKbCandShowSelected(HbmKbCandidateList* self);

void hbmKbSwitchOnPartEvent(HbmKbSwitchHandler* self, s32 kind, HbmKbPartView* sender, const HbmKbInputEvent* input);

#ifdef __cplusplus
}
#endif

#endif
