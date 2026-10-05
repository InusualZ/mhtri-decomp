/*
 * menu/menu_item_effect.cpp - unit, `.text` 0x8031DAA8..0x8031EA8C (14 functions, 4068 bytes).
 *
 * 4 of 14 functions have a body here.
 *
 * FLAGS.  `cflags_menu`.  GUESS (rule 7): the stem names what the old header says the tail drives (the selected item's
 * 3D effect model).
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.data, .sdata, .sdata2, .text, extab, extabindex).
 */
/* ---- header inherited from src/menu/fn_8031A6C0.cpp (written against its pre-phase-4 range) ---- */
/*
 * src/menu/fn_8031A6C0.cpp - the menu selection-screen band, `.text` 0x8031A6C0..0x8031EA8C.
 *
 * WHAT IT IS.  The item/equipment selection screen.  `MenuSel` (this band's view of the 0x330-byte
 * menu working record `menu_item.h` names `MenuSlot`) is the screen state: the input bits at +0x4/
 * +0x8, the selection cursor at +0x1EC, the 3+8 0x18-byte entry arrays at +0x24/+0x6C, the resolved
 * slot pairs at +0x210 and the item data at +0x1F0.  `self[1]` is the work area's second slot (the
 * +0x330 accesses).  `fn_8031A6C0` is the per-frame state step and `fn_8031B774` the state dispatch
 * (both switch on `state_0x001`); `fn_8031AE38`/`fn_8031B080`/`fn_8031BD7C` are the three draw
 * passes and the tail (0x8031D294..) drives the selected item's 3D effect model.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string covers the range: the
 * only source-file string in the band's whole `.data` run is `menu_infomation.cpp` (0x805DCCDC), and
 * it is loaded only by functions of the *previous* proposal range (0x80312F84..0x80316DDC), never by
 * this one - the flanking TU's string, not this range's (the convention `menu/fn_802E4978.cpp`'s
 * registration records).  2. `dumpmap.py lookup` answers only `zz_031a6c0_` for the code.  3. The
 * `menu` module is certain: the band calls the menu library's `get_menu_lsp_tbl`/`GetMenuFontColor`/
 * `GetItemData`/`ItemName`/`draw_font`, drives the `_PLW`/`_EQUIP` selection, and both bracketing
 * registrations are `menu`.  The file therefore keeps the map's stem (brief option 4).
 *
 * LANGUAGE.  C++: every callee is a mangled free function (`GetItemData__FUs`,
 * `get_lsp_data__FUsP10_mh_ivec2_`, ...), so the band's bodies are `extern "C"` and call the real
 * signature (rule 9).  The extab/extabindex runs the registered unit owns are
 * 0x80015F3C..0x80016074 / 0x80034D40..0x80034F14 (contiguous with the previous proposal's run,
 * which is why the seam at 0x8031A6C0 is a discovery size cap, not a TU edge).
 *
 * RESIDUALS (this pass).  The band is 59 functions / 17356 B; this pass reconstructs the 28 whose
 * bodies need no in-band helper that is not itself written here.  The remaining 31 - the two big
 * state machines (`fn_8031A6C0`, `fn_8031B774`), the three draw passes (`fn_8031AE38`,
 * `fn_8031B080`, `fn_8031BD7C`), the cursor's record machine (`fn_8031C0B8`, `fn_8031C5FC`,
 * `fn_8031CE08`, `fn_8031CAC4`) and the whole 0x8031D294.. effect tail - are transcribed from the
 * target's m2c skeletons in `.pi/notes` and are the follow-up round's work.  Every function's
 * `.text` is unaffected by the helpers, so the landed subset measures as its own functions.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked every `.text` row of
 * config/RMHE08/symbols.txt from 0x8031A6C0 to 0x8031EA8C - 59 functions, all `fn_` stems, and the
 * runtime dump answers `zz_031a6c0_`).
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

