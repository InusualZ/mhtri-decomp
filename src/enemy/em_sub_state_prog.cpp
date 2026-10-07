/* enemy/em_sub_state_prog.cpp - the tail of `enemy/fn_802F5138.cpp`'s action band: its last `state_sub` (+0x1E6)
 *   dispatchers and their handlers.
 * RANGE. .text 0x802F9994-0x802FA9A0 (11 functions); .data 0x805D8210-0x805D8B00, .bss 0x806BE0C0-0x806BE0D8,
 *   .sdata 0x807928D0-0x80792948, .sdata2 0x8079ABC0-0x8079AC20, extab, extabindex.
 * NAMES. `em_sub_state_prog` is a GUESS from the sub-state dispatchers the band holds.
 * RESIDUALS. 6 rows unwritten: 0x802F9994-0x802F9BF0, 0x802F9C2C-0x802FA7EC, 0x802FA804-0x802FA964.
 *  - `fn_802FA7EC` (written, 0 %): the 4-byte `b sinf` thunk; the callee's declaration,
 *    `f32 sinf(s16, f32)` (`unsplit/unknown.h`; `ef/eft022_fx.cpp` declares it the same way), makes ours
 *    narrow the argument with an `extsh`.
 *   flipcheck: `.bss`/`.data`/`.sdata`/`.sdata2`/extab/extabindex claimed, not emitted; `.text` short of the claim.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "ef.h"
#include "ef/effect.h"
#include "ef/eft_res.h"
#include "enemy/enemy_control.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "lobby/fn_802FA9A0.h"
#include "stage/stg_w.h"
extern "C" void fn_802F9BF0(_ENEMY_WORK* work);
extern "C" void fn_802F9C2C(_ENEMY_WORK* work);
extern "C" void fn_802FA33C(_ENEMY_WORK* work);
extern "C" f32 fn_802FA7EC(s16 a, f32 b);
extern "C" void fn_802FA7F0(_ENEMY_WORK* work);
extern "C" void fn_802FA800(_ENEMY_WORK* work);
extern "C" void fn_802FA964(_ENEMY_WORK* work);

#pragma peephole off

#pragma peephole on

/* Sub-state dispatcher of the band's model-placement action. */
extern "C" void fn_802F9BF0(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        fn_802F9C2C(work);
        break;
    case 1:
        fn_802FA33C(work);
        break;
    case 2:
        fn_802FA7F0(work);
        break;
    case 3:
        fn_802FA800(work);
        break;
    }
}

/* Forwards to the shared release helper (a tail call: the parameters pass straight through). */
extern "C" f32 fn_802FA7EC(s16 a, f32 b) {
    return sinf(a, b);
}

/* Advances a byte counter in a record above the work record. */
extern "C" void fn_802FA7F0(_ENEMY_WORK* work) {
    work->state = work->state + 1;
}

/* Releases the second shared resource. */
extern "C" void fn_802FA800(_ENEMY_WORK* work) {
    eft_res_slot_release(work);
}

/* Sub-state dispatcher of the band's last action. */
extern "C" void fn_802FA964(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        fn_802FA9A0(work);
        break;
    case 1:
        fn_802FAB98(work);
        break;
    case 2:
        fn_802FAFB4(work);
        break;
    case 3:
        fn_802FAFC4(work);
        break;
    }
}

