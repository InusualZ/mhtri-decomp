/* The declarations `src/enemy/em_act_step.cpp` owns (docs/plan.md 6.5, rules 1-5).
 *
 * The unit is the first of the two halves of enemy/em_act_step.cpp's range: the enemy work
 * record's action-step band, `.text` 0x8032C920..0x8033041C (74 functions / 15100 B).  It owns the table
 * run `.data` 0x805DFC9C..0x805E0510, whose last object is the class vtable below, plus extab
 * 0x80016464..0x8001662C, extabindex 0x800354FC..0x800357A8 and the `.ctors` word 0x8056F3A0 ->
 * `fn_80330128` (see the unit header for the seam evidence that split the proposal's range).
 *
 * Why only one declaration is here.  Rule 2 sends a declaration to the header of the unit that owns the
 * symbol, and ownership is `symbols.txt` + `splits.txt`: the `.data` claim above makes `lbl_805E04E0`
 * this unit's, so it moved out of `unsplit/enemy.h` (which is the band for a symbol *no* unit
 * owns).  This unit's `.sdata2` pool half 0x8079B108..0x8079B210 stays declared in that band on
 * purpose: the run is deliberately **unclaimed** - a `.sdata2` claim links only while the object emits
 * no pool of its own, and this one emits the compiler's 8-byte u32->f32 magic (playbook 23/58) - so
 * those labels have no owning unit, and the band is where they belong (playbook 29: declared, never
 * defined).
 *
 * The vtable keeps its `lbl_` stem on purpose: `lbl_XXXXXXXX` is not rule 7's class, and renaming a
 * data symbol moves the unit's `matched_data` rows for no gain (the naming pass left it alone).
 */
#ifndef MHTRI_ENEMY_EM_ACT_STEP_H
#define MHTRI_ENEMY_EM_ACT_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The class vtable this unit's constructor `em_work_ctor` stores into its record (`.data` 0x805E04E0,
 * 0x30 B = twelve slots).  It derives from the class whose vtable is `lbl_805A1368` (eleven slots,
 * enemy band) and this unit overrides three of them: the destructor `fn_803300CC`, `fn_8032FFEC` and
 * `fn_8032FFF4` - the range's own tail, still unwritten, which is why those keep the map's stems. */
extern void* lbl_805E04E0[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_ACT_STEP_H */
