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

/* The pane widget's primary base: built with an allocator, then initialised; both slots are the pane widget's.
 * size: 0x4 */
class HbmGuiAttachable {
public:
    /* slot +0x008 */ virtual void create(MEMAllocator* allocator) = 0;
    /* slot +0x00C */ virtual void init() = 0;
}; /* size: 0x4 */

/* The pane widget: a pane manager (at +0x04) behind an allocator-built front.  homebutton/hbm_kb_event.cpp's
 * factory (0x805615F8, a 0xAC-byte allocation) builds it; its destructor and calcComponents override are that unit's
 * (0x80560B68, 0x805646E4).  size: 0xAC */
class HbmGuiPaneWidget : public HbmGuiAttachable, public HbmPaneManager {
public:
    virtual void create(MEMAllocator* allocator);
    virtual void init();
    virtual ~HbmGuiPaneWidget();
    virtual void calcComponents();
}; /* size: 0xAC */
#endif

#endif
