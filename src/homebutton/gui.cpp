/*
 * homebutton/gui.cpp - software-keyboard code of the tiHKBManager band: the pane widget's create/init pair, its class
 * table and its adjustor thunks.
 *
 * RANGE.     .text 0x80569DAC..0x80569E64 (5 functions: create, init and the three `addi r3,r3,-4` thunks);
 *            .data 0x80658200..0x80658290 (the pane widget's class table).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * NAMES.     GUESS file name: the band's former `gui` unit; no `__FILE__` string, no runtime-dump name.
 * NAMES.     GUESS classes HbmGuiPaneWidget (the 0xAC-byte object homebutton/hbm_kb_event.cpp's 0x805615F8 builds) and
 *            HbmGuiAttachable (its abstract primary base, homebutton/gui.h); the pane manager at +0x04 is
 *            homebutton/hbm_value.h's HbmPaneManager.
 * SHAPES.    The pane widget is a real class (two bases), so MWCC emits the class table 0x80658200 (primary part:
 *            create, init; the +0x04 part: the destructor, init and calcComponents thunks; then the destructor and
 *            calcComponents again) and the three adjustor thunks at the end of the unit.
 * SHAPES.    create converts `this` through a reference (`HbmPaneManager& base = *this`): a pointer cast adds the null
 *            check retail does not have.
 * RESIDUALS. Our class table is 0x8C bytes, the claimed .data 0x90: the last word is zero in retail (no relocation) and
 *            no concrete class can emit a null slot, so it is read as the next object's alignment padding.
 */

#include "types.h"
#include "sys_mem.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/gui.h"
#include "homebutton/hbm_value.h"
#include "homebutton/hkb_layout.h"

/* 0x80569DAC (0x48): builds the pane manager with the allocator, then initialises the widget. */
void HbmGuiPaneWidget::create(MEMAllocator* allocator)
{
    HbmPaneManager& base = *this;

    base.HbmPaneManager::create(allocator, NULL);
    init();
}

/* 0x80569DF4 (0x58): advances the layout's animations and recomputes its matrices. */
void HbmGuiPaneWidget::init()
{
    mpLyt->Animate(0);
    mpLyt->CalculateMtx(mDrawInfo);
}
