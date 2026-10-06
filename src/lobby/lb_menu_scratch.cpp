/* lobby/lb_menu_scratch.cpp - the lobby menu scratch block and its item-list page head: the candidate recorder, the
 *   entry filter and the scratch block's constructor.
 * RANGE. .text 0x8021E1EC-0x8021F020 (14 functions); .ctors 0x8056F360-0x8056F364, .data 0x805BA0C0-0x805BA540, .bss
 *   0x806AA8C8-0x806AAA88 (`lb_menu_scratch`), .sdata 0x80791EE0-0x80791EF0, .sdata2 0x80799C60-0x80799C78, extab,
 *   extabindex.  `tudiscover` reports the left edge as a strong cut (the `.sdata2` run `lbl_80799C58` ->
 *   `lbl_80799C60`), and the extabindex run starts with `fn_8021E1EC` (`fn_8021E1B4`'s record ends there).
 *   `lobby/lb_menu_pos_tbl.cpp` and `lobby/lb_equip_page.cpp` follow; the three share `lobby/fn_8021E1EC.h`.
 * FLAGS. `cflags_lobby` and file-scope `#pragma peephole off` (retail keeps the narrowing `clrlwi`s), back on around
 *   `fn_8021E304`, whose retail body has none to keep (docs/lobby.md).
 * NAMES. `lb_menu_scratch` is the `.bss` block the unit constructs (`fn_8021EFC8`); the map and the dump give only
 *   placeholders for the functions.
 * RESIDUALS. Unwritten: `fn_8021E538` (0x8021E538-0x8021EBF0), `fn_8021EC98` (0x8021EC98-0x8021EFBC).  `fn_8021EFBC`,
 *   which the `.ctors` word points at, is written as a plain function, so the object emits no `.ctors`.
 *  - `fn_8021E1EC`: retail copies the fourth integer argument (`mr r30,r6`) after the float one (`fmr f31,f1`), ours
 *    before; the parameter order, local order and `~limit` spellings do not move it;
 *  - `fn_8021EFC8`: the loop cursor and end pointer sit in r31/r30 where retail uses r30/r31.
 *   flipcheck: `.ctors`/`.sdata`/`.sdata2` claimed, not emitted; `.data` 0x40 against 0x480; `.text`/extab/extabindex
 *   short of the claim.
 * SHAPES. The `.bss` objects are defined at the foot of the file, after every use, so each keeps its own `lis`/`addi`.
 */
#include "types.h"
#include "nw4r/math.h"

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#include "lobby/fn_8021E1EC.h"
#include "lobby/fn_80208AC0.h" /* fn_80208AC0, owned by lobby/lb_npc.cpp (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* This range's record view of the user-data block; the owner's `lobby_world_block` is a plain `u8*`. */
static inline LbMenuBigBlock* lb_menu_block(void)
{
    return (LbMenuBigBlock*)lobby_world_block;
}

/* Retail keeps the narrowing `clrlwi` the peephole pass folds away; `fn_8021E304` (none to keep) keeps the pass on. */
#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * Foreign callees.
 *
 * The plain `fn_XXXXXXXX` names are this unit's own spelling of unsplit addresses whose bracketing
 * registered units name different modules (rule 2's named gap), the same way
 * `src/lobby/fn_80212810.cpp` records them; the C++-linkage lobby ABI lives in this unit's header.
 */
extern "C" {
void* fn_8021EFC8(LbMenuScratch* work);
u32 fn_8021E1B4(LbMenuActor* self, VEC3* pos);
f32 fabsf(f32 v);

u32 fn_8021DF50(u32 idx, u32 limit);
void fn_8021DDA8(LbMenuCandidate* candidate, LbMenuRow* row);
f32 fn_8021E300(f32 v);
s32 game_ready_ck(void);
u8 fn_8004DD74(void);









}

extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * Definitions, in address order.
 */

/* The candidate recorder: copies the entry's position (or the fallback work record's), rejects it when
 * it is outside the y window or its slot is, and otherwise appends it to the scratch row. */
s32 fn_8021E1EC(LbMenuActor* self, LbMenuActor* rec, LbMenuFallback* work, u32 limit, f32 radius)
{
    LbMenuCandidate candidate;
    u32 idx;
    f32 dist_sq;
    VEC3 pos;

    VEC3_ctor(&pos);
    if (rec != 0) {
        copyVec3(&pos, &rec->pos_0x03C);
    } else {
        copyVec3(&pos, &work->pos_0x10);
    }
    if (fn_8021E300(pos.y - self->pos_0x03C.y) < lbl_80799C60) {
        idx = fn_8021E1B4(self, &pos);
        dist_sq = calcDistanceSqXZ(&self->pos_0x03C, &pos);
        if ((idx < limit || idx > (u32)(-1 - (s32)limit)) && dist_sq < radius) {
            candidate.rec_0x00 = rec;
            candidate.work_0x04 = work;
            candidate.dist_sq_0x08 = dist_sq;
            candidate.slot_0x0C = (s16)fn_8021DF50(idx, limit);
            fn_8021DDA8(&candidate, &lb_menu_scratch.rows_0x00C[0]);
            return 1;
        }
    }
    return 0;
}

/* The y-window filter's absolute value helper (retail: `b <fabsf>`). */
f32 fn_8021E300(f32 v)
{
    return fabsf(v);
}

/* The entry filter: the entry's kind byte must not be 0x49 on a busy actor, and its mode must not be
 * the "special" one.  (peephole on: this body has no narrowing store the pass could fold.) */
#pragma peephole on
s32 fn_8021E304(LbMenuActor* self, LbMenuEntryRec* entry)
{
    if (entry->kind_0x002 == 0x49 && self->busy_0xB03) {
        return 0;
    }
    if (entry->mode_0x256 == 1) {
        return 0;
    }
    return 1;
}

#pragma peephole off

/* The page's per-kind gate: whether the entry kind may be opened from the current lobby state. */
s32 fn_8021E340(LbMenuActor* self, s16* kind)
{
    s32 ok = 1;

    if (fn_80208AC0() != 0) {
        ok = 0;
    }
    switch (*kind) {
    case 10:
    case 11:
    case 14:
    case 15:
        if (game_ready_ck() == 0) {
            ok = 0;
        }
        if (lobby_w.state_0x000) {
            ok = 0;
        }
        break;
    case 1:
    case 6:
        if (self->busy_0xB03) {
            ok = 0;
        }
        break;
    case 3:
        if (lobby_w.state_0x000) {
            ok = 0;
        }
        if (!lobby_w.page_0x052) {
            ok = 0;
        }
        break;
    case 4:
        if (lobby_w.page_0x052 != 6) {
            ok = 0;
        }
        break;
    default:
        if (lobby_w.state_0x000) {
            ok = 0;
        }
        break;
    }
    return ok;
}

/* The item-list page's "still idle" gate. */
s32 fn_8021E454(void)
{
    if (lobby_w.state_0x000 == 0 && lobby_w.flag_0x027 == 0) {
        return 1;
    }
    return 0;
}

/* The page's availability gate: the item list must be open and its kind must match. */
s32 fn_8021E484(s32 kind)
{
    if (fn_8004DD74() == 0) {
        return 0;
    }
    switch ((u8)kind) {
    case 0:
        if (lb_menu_block()->list_kind_0x484D) {
            return 0;
        }
        break;
    case 1:
        if (lb_menu_block()->list_kind_0x484D != 1) {
            return 0;
        }
        break;
    case 2:
        if (lb_menu_block()->list_kind_0x484D != 2) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    return 1;
}

/* The current row's model pointer, or 0 when there is none. */
s32 fn_8021EBF0(void)
{
    if (lb_menu_scratch.index_0x14C >= 0 && lb_menu_scratch.rows_0x00C[lb_menu_scratch.index_0x14C].model_0x04 != 0) {
        return lb_menu_scratch.rows_0x00C[lb_menu_scratch.index_0x14C].model_0x04;
    }
    return 0;
}

/* The current row's table pointer, or 0 when there is none. */
s32 fn_8021EC20(void)
{
    if (lb_menu_scratch.index_0x14C >= 0 && lb_menu_scratch.rows_0x00C[lb_menu_scratch.index_0x14C].table_0x00 != 0) {
        return lb_menu_scratch.rows_0x00C[lb_menu_scratch.index_0x14C].table_0x00;
    }
    return 0;
}

/* The special (fixed) entry's two pointers, one per accessor; only valid in the -2 mode. */
s16* fn_8021EC50(void)
{
    if (lb_menu_scratch.index_0x14C == -2) {
        return lb_menu_scratch.fixed_a_0x004;
    }
    return 0;
}

u8* fn_8021EC74(void)
{
    if (lb_menu_scratch.index_0x14C == -2) {
        return lb_menu_scratch.fixed_b_0x008;
    }
    return 0;
}

/* The no-argument form the constructor table calls.  It is defined before the callee so the retail
 * out-of-line tail call survives. */
void fn_8021EFBC(void)
{
    fn_8021EFC8(&lb_menu_scratch);
}

/* The scratch block's constructor: clears its 8-entry vector run. */
void* fn_8021EFC8(LbMenuScratch* work)
{
    VEC3* p = work->slots_0x160;
    VEC3* end = work->slots_0x160 + 8;

    do {
        VEC3_ctor(p);
        p++;
    } while (p < end);
    return work;
}


/* This unit's `.bss`, defined after every use so each access keeps its own `lis`/`addi` like the target. */
LbMenuScratch lb_menu_scratch;             /* .bss 0x806AA8C8 */

}  /* extern "C" */
