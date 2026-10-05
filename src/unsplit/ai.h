/*
 * The `ai` band's unowned addresses: the `.sdata` table and the `.sdata2` float pool the AI-NPC
 * sub-state machines load.  Nothing owns them (no registered unit's `splits.txt` range covers the
 * addresses), so this is the band's fallback header (docs/plan.md 6.5 rule 2).
 *
 * The pool labels are used as load operands, never defined (playbook 29): defining one would rebuild
 * the section and move every later constant.
 */
#ifndef MHTRI_UNSPLIT_AI_H
#define MHTRI_UNSPLIT_AI_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80792598 - the four sub-state frame counts (180, 270, 285, 300) the AI-NPC step machines index
 * with their `+0x005` entry byte. */
extern s16 lbl_80792598[];

/* The `.sdata2` pool run 0x8079A670-0x8079A77C the AI-NPC band loads its geometry/angle constants
 * from (the loads' own displacements resolve here). */
extern const f32 lbl_8079A670; /* 0.0 */
extern const f32 lbl_8079A674; /* 0.75 */
extern const f32 lbl_8079A678; /* 1.0 */
extern const f32 lbl_8079A680; /* 1.5 */
extern const f32 lbl_8079A684; /* 1.4 */
extern const f32 lbl_8079A688; /* 60.0 */
extern const f32 lbl_8079A68C; /* 50.0 */
extern const f32 lbl_8079A694; /* 2000.0 */
extern const f32 lbl_8079A6BC; /* 220.0 */
extern const f32 lbl_8079A6C0; /* 65.0 */
extern const f32 lbl_8079A6C4; /* 40.0 */
extern const f32 lbl_8079A6C8; /* 143.0 */
extern const f32 lbl_8079A6CC; /* 150.0 */
extern const f32 lbl_8079A6D0; /* 400.0 */
extern const f32 lbl_8079A6D4; /* 200.0 */
extern const f32 lbl_8079A6D8; /* 500.0 */
extern const f32 lbl_8079A6DC; /* 700.0 */
extern const f32 lbl_8079A6E0; /* 10000.0 */
extern const f32 lbl_8079A6E4; /* 4.0 */
extern const f32 lbl_8079A6E8; /* 1.3 */
extern const f32 lbl_8079A6EC; /* 12.0 */
extern const f32 lbl_8079A6F0; /* 2500.0 */
extern const f32 lbl_8079A6F4; /* 10.0 */
extern const f32 lbl_8079A6F8; /* 1.2 */
extern const f32 lbl_8079A6FC; /* 15.0 */
extern const f32 lbl_8079A700; /* 7.0 */
extern const f32 lbl_8079A704; /* -2.5 */
extern const f32 lbl_8079A708; /* 6.0 */
extern const f32 lbl_8079A70C; /* 160.0 */
extern const f32 lbl_8079A710; /* 80.0 */
extern const f32 lbl_8079A714; /* 8.0 */
extern const f32 lbl_8079A718; /* 64.0 */
extern const f32 lbl_8079A71C; /* 194.0 */
extern const f32 lbl_8079A720; /* 94.0 */
extern const f32 lbl_8079A724; /* 88.0 */
extern const f32 lbl_8079A728; /* 0.5 */
extern const f32 lbl_8079A72C; /* 124.0 */
extern const f32 lbl_8079A730; /* 2.0 */
extern const f32 lbl_8079A734; /* -5.0 */
extern const f32 lbl_8079A738; /* -1.0 */
extern const f32 lbl_8079A73C; /* 0.6 */
extern const f32 lbl_8079A740; /* 22.0 */
extern const f32 lbl_8079A744; /* -2.0 */
extern const f32 lbl_8079A748; /* 176.0 */
extern const f32 lbl_8079A750; /* 5.57 */
extern const f32 lbl_8079A754; /* 20.0 */
extern const f32 lbl_8079A758; /* -306.9 */
extern const f32 lbl_8079A75C; /* -279.5 */
extern const f32 lbl_8079A760; /* -56.9 */
extern const f32 lbl_8079A764; /* 0.2 */
extern const f32 lbl_8079A768; /* 0.8 */
extern const f32 lbl_8079A76C; /* 800.0 */
extern const f32 lbl_8079A770; /* 1000.0 */
extern const f32 lbl_8079A774; /* 1500.0 */
extern const f32 lbl_8079A778; /* 4000000.0 */

#ifdef __cplusplus
}
#endif

extern const f32 lbl_8079A870; /* 5000.0 - the AI NPC's own attack reach */

/* The hold-item tuning tables `src/ai/fn_802D44F4.cpp` reads (0x805D5354-0x805D5390): none of them is
 * covered by a registered unit's range, so they are the band's fallback declarations too. */
extern s16 lbl_805D5354[6];        /* the release countdown, x 0x1E frames */
extern s16 lbl_805D5378[12];       /* the +0x422/+0x424 timer pair, x 0x1E frames */
extern s16 lbl_805D5390[12];       /* the second +0x422/+0x424 timer pair */

/* The per-record tuning table at 0x805D5360: 6 records of 4 bytes (size 0x18); +0x0 and +0x1 are the
 * two thresholds `fn_802D66B8` rolls against and +0x3 the frame count `fn_802D6690` scales.
 * size: 0x4 */
struct AINPCTuning {
    /* +0x0 */ u8 field_0x0;
    /* +0x1 */ u8 field_0x1;
    /* +0x2 */ u8 unused_0x2;
    /* +0x3 */ u8 field_0x3;        /* the per-record frame count (x 0x708) */
};
extern struct AINPCTuning lbl_805D5360[6];

#endif /* MHTRI_UNSPLIT_AI_H */
