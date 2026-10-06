/* The declarations `enemy/em_act_step.cpp` owns: the class vtable `lbl_805E04E0` its `.data` ends with (the
 * `lbl_` stem is kept: renaming the data symbol moves the unit's matched data rows).
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
