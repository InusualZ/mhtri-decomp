/* Declarations the enemy action band `enemy/em009_act.cpp` needs that its owner headers cannot
 * carry without breaking the C consumers: the two move-work accessors the band walks by their 0xB20
 * stride.  `ef/fn_800CDB2C.cpp` owns the addresses (0x800CFA90/0x800CFAD0), but its own header cannot
 * hold the C++-scope spelling beside `include/unsplit/ef.h`'s C-scope one (MWCC 10505 illegal
 * overloading, hit by `Pl/fn_80273B14.cpp`), so the C++ consumers that already carry it - `include/
 * enemy/fn_80165FC8.h`, `include/Pl/fn_8025F088.h`, `include/ai/fn_802D0F34.h` - keep their own copy
 * and so does this unit.  The map names are manglings, so the declarations sit at C++ scope (rule 9).
 */
#ifndef MHTRI_ENEMY_FN_80387844_H
#define MHTRI_ENEMY_FN_80387844_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
/* The action-table head 0x803874C8..0x80387844 (inside this unit's range): the first four `state_sub` handlers
 * `fn_80388144` dispatches to.  Each drives the shared `_ENEMY_WORK`; `fn_803874C8` takes the work record, the
 * other three take none (they forward r3). */
void fn_803874C8(struct _ENEMY_WORK* self);
void fn_80387528(void);
void fn_803875A4(void);
void fn_80387620(void);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
void* get_move_work_adrs(u8 kind);
u32 get_move_work_max(u8 kind);
#endif

#endif /* MHTRI_ENEMY_FN_80387844_H */
