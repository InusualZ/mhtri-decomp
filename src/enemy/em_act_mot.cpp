/* Monster Hunter Tri (RMHE08) - the enemy per-motion stepper band 0x8037F940-0x80382310 (`.text`,
 * 4 functions, 10704 B), reconstructed from the split target object.
 *
 * WHAT IT IS.  `em_act_mot_step` (0x8037F940, 10076 B) is the per-motion-number stepper: it calls
 * the em019 band's `em_parts_damage_ck` and `em_act_effect_ck`, reads `em_get_mot_no(self)` and
 * switches 216 ways (`cmplwi r0, 215` over the 216-entry jump table `.data` 0x805EF52C) - one arm
 * per motion the enemy can play, each arming the motion's own state and handing its damage windows
 * on.  The other three are the part-material steppers the neighbouring band calls: `em_part_reset`
 * resets one part slot's colour/meter and takes its RGB from the shared id table, `em_part_colour_lerp`
 * lerps a slot's RGB toward a target by its alpha, and `em_part_damage_meter` steps a slot's 0..1
 * damage meter by 1/damage and arms/disarms the slot.
 *
 * MODULE AND NAME (brief section 2, evidence order).  Class 1, a `__FILE__` string: none reachable
 * (this band's `.data`/`.sdata2` runs carry no printable byte).  Class 2, the runtime dump:
 * `dumpmap.py lookup` answers `zz_XXXXXXXX_` for all four addresses.  Class 3 decides the module: the
 * band drives the shared `_ENEMY_WORK` record, its link neighbours are `enemy/*`, and its three
 * steppers are called from the registered `enemy/fn_80382310.cpp`; the file name `em_act_mot` and
 * every symbol here are **derived names (GUESS)** from the bodies, on the module's `em_<noun>_<verb>`
 * scheme (`em_action.cpp`, `em_act_step.cpp`).
 *
 * SEAM (re-drawn, not the brief's `--max-bytes` cut).  This unit is the half of
 * `proposal/8037EA64_fn_8037EA64.cpp` that the brief's byte budget had merged with the em019
 * program band below; the cut is 0x8037F940 and the evidence is in the sibling half's header
 * (`src/enemy/em019_prog.cpp`): `tudiscover.py at 0x8037E0E8` reports it as the strong seam (the
 * `.data` run jump `jumptable_805EF4F4` -> `jumptable_805EF52C` and the `.sdata2` run jump
 * `lbl_8079BE88` -> `lbl_8079BE8C`, each side's labels referenced only by its own functions), the
 * registered `enemy/em019_ai.cpp` records 0x8037F940 as its file's right edge, and the bracketing
 * extab runs tile - this unit takes 0x80017E14..0x80017E2C and the next registered unit
 * (`enemy/fn_80382310.cpp`) starts its own record run at 0x80017E2C.
 *
 * rule 7 deferred: references only to other units' unrenamed `fn_XXXXXXXX` symbols; every symbol this
 * file *defines* is named (the three definitions below).  `em_act_mot_step`'s own row is **not yet
 * defined** - the 216 arm bodies are a unit of their own - so its map name stays a target-only name
 * and its row measures 0 % (see RESIDUALS).  Every definition is `extern "C"` so objdiff pairs it by
 * the map's name (playbook 42).
 *
 * SECTIONS.  `.text` 0x8037F940..0x80382310, `extab` 0x80017E14..0x80017E2C (3 x 8 B: one
 * record per function that carries one, `em_act_mot_step` included), `extabindex`
 * 0x80037B84..0x80037BA8 (3 x 12 B).  The `.data` jump table run 0x805EF4D0..0x805EF52C and the
 * shared part-colour id table 0x805EE5A0 are **not claimed**: the id table is referenced from
 * `enemy/fn_80382310.cpp` too (playbook 58's sole-referencer condition fails) and a jump table claim
 * without its emitting switch is a `target-extra` row.
 *
 * RESIDUALS.  `em_act_mot_step` (10076 B, 94 % of this unit's `.text`) is unwritten: a 216-case
 * switch whose arms must be read out of the target one at a time, which is a lane of its own.  It is
 * the unit's one 0 % row.
 */
#include "types.h"
#include "enemy/EM_PART_BLOCK.h"
#include "enemy/ENEMY_WORK.h"
#include "gx.h"
#include "sound/mhchar.h"

/* Retail keeps the unfused `clrlwi rN, r4, 24` that narrows each stepper's slot index before the
 * first array access (`-O3`'s peephole drops it as redundant for a `u8` parameter).  `stage/
 * fn_802B2AA0.h` used to carry this pragma into the band by accident (it opens a `#pragma peephole
 * off` for its own band); this unit states it, so the three steppers keep the form (playbook 39:
 * measured 86.4/90.5/89.5 with the peephole on, ~100 with it off). */
#pragma peephole off

/* The shared part-colour id table the steppers index by slot (`lwzx r4, r4, r0` over 0x805EE5A0,
 * `{3, 0, 1, 4}`).  Referenced from `enemy/fn_80382310.cpp` too, so it is declared, never defined
 * (playbook 29/58). */
extern "C" u32 lbl_805EE5A0[];

/* The band's pooled floats, declared never defined: they are shared with the neighbouring bands'
 * pools, so none is claimable (playbook 58). */
extern "C" f32 lbl_8079BC88;   /* 0.0 */
extern "C" f32 lbl_8079BCA8;   /* 1.0 */

extern "C" {

/* The per-motion stepper of the enemy work record - 0x8037F940, 10076 B, NOT reconstructed: its 216
 * arms would have to be read out of the target one at a time.  See the file header's RESIDUALS. */

/* Resets one part slot: stores the caller's K-colour index, clears the slot's timer, meter, arm byte
 * and alpha, and takes the slot's RGB either from the caller's colour or from the shared id table. */
extern "C" void em_part_reset(_ENEMY_WORK* self, u8 idx, u8 colour_id, const u8* colour) {
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;

    part->colour_id[idx] = colour_id;
    part->timer[idx] = 0;
    part->meter[idx] = lbl_8079BC88;
    part->flag[idx] = 0;
    if (colour == NULL) {
        GXColor tmp;

        ((MHchar*)self->char_0x024)->getTevKColor(lbl_805EE5A0[idx], GX_KCOLOR3, &tmp);
        part->r[idx] = tmp.r;
        part->g[idx] = tmp.g;
        part->b[idx] = tmp.b;
    } else {
        part->r[idx] = colour[0];
        part->g[idx] = colour[1];
        part->b[idx] = colour[2];
    }
    part->a[idx] = 0;
}

/* Lerps one part slot's RGB toward a target colour by the slot's alpha over the caller's ceiling,
 * and stamps the caller's alpha on the result. */
extern "C" void em_part_colour_lerp(_ENEMY_WORK* self, u8* out, u8 idx, const u8* colour, u8 ceiling) {
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;
    f32 t = (f32)part->a[idx] / (f32)ceiling;

    out[0] = part->r[idx] + (s32)((f32)(colour[0] - part->r[idx]) * t);
    out[1] = part->g[idx] + (s32)((f32)(colour[1] - part->g[idx]) * t);
    out[2] = part->b[idx] + (s32)((f32)(colour[2] - part->b[idx]) * t);
    out[3] = 255;
}

/* Steps one part slot's damage meter by 1/damage, arming the slot when it reaches 1.0 and disarming
 * it when it falls back to 0.0. */
extern "C" void em_part_damage_meter(_ENEMY_WORK* self, u8 idx, f32 damage) {
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;

    if (damage <= lbl_8079BC88)
        return;
    if (part->flag[idx] == 0) {
        part->meter[idx] += lbl_8079BCA8 / damage;
        if (part->meter[idx] >= lbl_8079BCA8) {
            part->meter[idx] = lbl_8079BCA8;
            part->flag[idx] = 1;
        }
    } else {
        part->meter[idx] -= lbl_8079BCA8 / damage;
        if (part->meter[idx] <= lbl_8079BC88) {
            part->meter[idx] = lbl_8079BC88;
            part->flag[idx] = 0;
        }
    }
}
}
