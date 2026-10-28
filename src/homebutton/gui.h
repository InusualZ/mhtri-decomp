/*
 * homebutton/gui.h - the classes owned by `homebutton/gui.cpp`: the pane widget whose class table (0x80658200) the
 * unit's .data holds.
 */
#ifndef HOMEBUTTON_GUI_H
#define HOMEBUTTON_GUI_H

#include "types.h"
#include "homebutton/hbm_value.h"

#ifdef __cplusplus
struct MEMAllocator;

/* The pane widget's primary base: built with an allocator, then initialised; both slots are the pane widget's.  Its
 * own slots are empty inline functions (homebutton/hbm_kb_event.cpp's table 0x80657858 holds them).
 * size: 0x4 */
class HbmGuiAttachable {
public:
    /* slot +0x008 */ virtual void create(MEMAllocator* allocator) {}
    /* slot +0x00C */ virtual void init() {}
}; /* size: 0x4 */

/* The pane widget: a pane manager (at +0x04) behind an allocator-built front.  homebutton/hbm_kb_event.cpp's
 * factory (0x805615F8, a 0xAC-byte allocation) builds it; its destructor and calcComponents override are that unit's
 * (0x80560B68, 0x805646E4).  size: 0xAC */
class HbmGuiPaneWidget : public HbmGuiAttachable, public HbmPaneManager {
public:
    HbmGuiPaneWidget(nw4r::lyt::ResourceAccessor* resAccessor, const char* layoutName, HbmSoundPlayer* sound)
        : HbmPaneManager(resAccessor, layoutName, sound) {}

    virtual void create(MEMAllocator* allocator);
    virtual void init();
    virtual ~HbmGuiPaneWidget();
    virtual void calcComponents();
}; /* size: 0xAC */
#endif

#endif
