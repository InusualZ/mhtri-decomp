/*
 * `lb_param_w` - the lobby/option parameter block (.bss 0x806590B4, 0x9C B), defined by `src/mh3_pad.cpp`
 * (its `.bss` 0x806585B8-0x806694E8 is that unit's own).  The one home of the type (rule 1; the
 * object is declared in `lb_param_w.h`); it merges the three per-consumer views the tree carried before:
 * the lobby band's (`flag_0x0C`/`value_0x10`/`value_0x16..`, `unsplit/lobby.h`), the companion page's
 * (`entry_0x08`, `sub_0x26..sub_0x30`) and `Pl/fn_80273B14.cpp`'s act-id/bonus rows (the same bytes as
 * `flag_0x0C[0..2]` / `value_0x10[0..2]`).
 */
#ifndef MHTRI_MH3_PAD_LB_PARAM_WORK_H
#define MHTRI_MH3_PAD_LB_PARAM_WORK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LbParamWork {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 entry_0x08;     /* the selected entry id */
    /* +0x09 */ u8 pad_0x09[2];
    /* +0x0B */ u8 flag_0x0b;      /* == 1 with +0x00 set is `quest_select_ready_ck` */
    /* +0x0C */ u8 flag_0x0C[3];   /* the three act ids the per-act bonus rows key on */
    /* +0x0F */ u8 pad_0x0F;
    /* +0x10 */ s16 value_0x10[3]; /* the bonus rows matching `flag_0x0C` */
    /* +0x16 */ u16 value_0x16;
    /* +0x18 */ u16 value_0x18;
    /* +0x1A */ u16 value_0x1A;
    /* +0x1C */ u16 value_0x1C;
    /* +0x1E */ u8 pad_0x1E[8];
    /* +0x26 */ u8 sub_0x26;
    /* +0x27 */ u8 sub_0x27;
    /* +0x28 */ u8 sub_0x28;
    /* +0x29 */ u8 sub_0x29;
    /* +0x2A */ u8 sub_0x2A;
    /* +0x2B */ u8 pad_0x2B;
    /* +0x2C */ u16 sub_0x2C;
    /* +0x2E */ u16 sub_0x2E;
    /* +0x30 */ u16 sub_0x30;
    /* +0x32 */ u8 pad_0x32[0x6A];
} LbParamWork; /* size: 0x9C */


#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_LB_PARAM_WORK_H */
