/*
 * homebutton/gui.cpp - HOME-button (HBM) software-keyboard code, `.text` 0x80569DAC..0x8056A478 (7 functions, 2 reconstructed).
 *
 * Phase 4 recut: 2 function(s) of 0x80569DAC..0x8056A478 from the former registered unit `homebutton/gui.cpp`.
 *
 * Name: the homebutton::gui component / pane-manager classes (their real spellings come from the shared dump, see homebutton/gui_manager.h); most functions keep their `fn_<addr>` placeholders.  The band was built with C++ exceptions off (no
 * extab/extabindex in the target objects), so the scoped `#pragma exceptions off` below is carried from the absorbed sources.
 *
 * Residuals: partial reconstruction - the functions not defined below keep the target object's bytes; the evidence, type views and
 * per-function residuals of the absorbed sources are in docs/splits/phase4/homebutton-carried-notes.md; the type views are in
 * `homebutton/fn_8054E894.h`, `homebutton/hbm_widget.h`, `homebutton/hbm_vu_object.h`, `homebutton/hbm_kb_object.h` and `homebutton/gui_manager.h`.
 */

#include "types.h"
#include "sys_mem.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/gui_manager.h"
#include "homebutton/hbm_value.h"

/* The unowned helpers this band dispatches into.  They sit in the unsplit `main` band before this
 * unit (fn_80501xxx / fn_8055xxxx), so rule 2 leaves them a counted gap (tools/units/stylelint.py,
 * "address band interleaves modules"); MEMAlloc-from / MEMFree-to are the same. */

/* The `.data` vtables this unit's objects point at (they live outside the claimed .text range).
 * They are the map's `lbl_` objects; referencing them keeps MWCC from emitting a table of its own. */

/* --- 0x80569DAC: re-init the object at +4 with `a`, then run its virtual slot 3. --- */
extern "C" void fn_80569DAC(Manager* self, void* a) {
    fn_8055B8F0(&self->field_0x04, a, 0);
    ((void (*)(Manager*))self->vtable->slot_0C)(self);
}

/* --- 0x80569DF4: retarget the allocator's slot 0x38 and free the +0x1C block through slot 0x30. --- */
extern "C" void fn_80569DF4(Manager* self) {
    Manager* a = (Manager*)self->allocator;
    ((void (*)(Manager*, int))a->vtable->slot_38)(a, 0);
    a = (Manager*)self->allocator;
    ((void (*)(Manager*, void*))a->vtable->slot_30)(a, &self->field_0x1C);
}
