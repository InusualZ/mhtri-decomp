/*
 * menu/menu_item_effect.cpp - the selection screen's effect tail: the updaters that drive the selected item's 3D effect
 *   model (`MenuEff` records), after `menu/fn_8031A6C0.cpp`.  C++ callees, `extern "C"` bodies.
 * RANGE. .text 0x8031DAA8-0x8031EA8C (14 functions); extab, extabindex, .data 0x805DCF38-0x805DD298, .sdata
 *   0x80792C48-0x80792C50, .sdata2 0x8079AE70-0x8079AEA8.
 * FLAGS. `cflags_menu` (configure.py).
 * NAMES. The file name is a GUESS from what the tail drives; no `__FILE__` string covers it.
 * RESIDUALS. 10 rows unwritten (objdiff scores them zero): 0x8031DAA8-0x8031E568, 0x8031E57C-0x8031EA78.
 *   `fn_8031E568`/`fn_8031EA78`: our `(u8)(step + 1)` store keeps a `clrlwi r0,r0,24` retail does not have.
 *   flipcheck: `.data` (0x360), `.sdata` (0x8), `.sdata2` (0x38), extab (0x40) and extabindex (0x60) claimed but not
 *   emitted; short `.text` 0x30 of 0xFE4, its bytes differ.
 */

#include "types.h"
#include "ef/eft_res.h"
#include "menu/fn_8031A6C0.h"
#include "menu/menu_item.h"
#include "menu/menu_message.h"
#include "sound/fn_800D7F54.h"

extern "C" {
}

/* ============================================================================================== */
/* 0x8031E568 / 0x8031EA78 - step counters; 0x8031E578 / 0x8031EA88 / 0x8031DA50 - tail calls    */
/* ============================================================================================== */
extern "C" void fn_8031E568(MenuEff* self) {
    self->step_0x005 = (u8)(self->step_0x005 + 1);
}

extern "C" void fn_8031EA78(MenuEff* self) {
    self->step_0x005 = (u8)(self->step_0x005 + 1);
}

extern "C" void fn_8031E578(MenuEff* self) {
    eft_record_retire(self);
}

extern "C" void fn_8031EA88(MenuEff* self) {
    eft_record_retire(self);
}

