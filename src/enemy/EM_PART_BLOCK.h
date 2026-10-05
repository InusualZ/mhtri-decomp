/* The enemy work record's per-part block at +0x328, as the two bands that own it read it.
 *
 * `enemy/ENEMY_WORK.h` carries a union over +0x328 with a dozen other bands' views but not
 * this one, and the header is shared, so the layout lives here (docs/plan.md 6.5 rule 1: two units
 * use it - `enemy/em019_prog.cpp` clears it and `enemy/em_act_mot.cpp`'s steppers write it).  The
 * offsets are the target's own, read out of the four functions that touch it:
 *   * `em_part_reset` writes +0x32C+idx (byte), +0x330+idx*2 (halfword), +0x338+idx*4 (float) and
 *     +0x328/+0x348..+0x354+idx (`stb r5, 4(r7)` / `sth r5, 8(r4)` / `stfs f0, 16(r4)` with
 *     r7 = base + idx and r4 = base + idx*2 / base + idx*4);
 *   * `em_part_colour_lerp` reads +0x32C+idx as the slot's alpha and the RGB at +0x348;
 *   * `em_part_damage_meter` steps +0x338+idx*4 between 0 and 1 and arms +0x328+idx;
 *   * `em_act_prog_1` clears all 32 bytes of it.
 * The four loops' index is the enemy work record's part slot (0..3); `enemy/fn_80382310.cpp` reads
 * the same bytes as its own per-slot view.
 * size: 0x30 */
#ifndef MHTRI_ENEMY_EM_PART_BLOCK_H
#define MHTRI_ENEMY_EM_PART_BLOCK_H

#include "types.h"

struct EmPartBlock {
    /* +0x000 (+0x328) */ u8 flag[4];      /* arm byte, set when the slot's meter reaches 1.0 */
    /* +0x004 (+0x32C) */ u8 colour_id[4]; /* the GX K-colour index `em_part_reset` is handed */
    /* +0x008 (+0x330) */ s16 timer[4];    /* the per-slot timer `em_part_reset` clears */
    /* +0x010 (+0x338) */ f32 meter[4];    /* the 0..1 damage meter `em_part_damage_meter` steps */
    /* +0x020 (+0x348) */ u8 r[4];
    /* +0x024 (+0x34C) */ u8 g[4];
    /* +0x028 (+0x350) */ u8 b[4];
    /* +0x02C (+0x354) */ u8 a[4];         /* the alpha `em_part_colour_lerp` scales by */
};

#endif /* MHTRI_ENEMY_EM_PART_BLOCK_H */
