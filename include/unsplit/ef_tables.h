/*
 * The `ef` band's data tables with no registered owner (docs/plan.md 6.5 rule 2: the band is the home for a symbol no unit owns).
 *
 * 0x80594840 - the strategy class table `ef/ef_effectsystem.cpp`'s out-of-line constructor stores (ef_drawstrategyimpl.cpp's `fn_800C5DB8` class);
 * the data run is an unowned ambiguous one in the reconciled candidate (its reader is the effect system, its neighbours the draw strategies').
 */
#ifndef MHTRI_UNSPLIT_EF_TABLES_H
#define MHTRI_UNSPLIT_EF_TABLES_H

#include "types.h"

extern void* lbl_80594840[];

/* 0x8059DCF8 / 0x8059DE00 / 0x8059DE48 - the enemy-effect part tables `ef/eft007.cpp`'s emitter setup indexes: the joint/motion id per
 * emitter type, the effect count per part and the joint offset per part (an engine `Vec`, completed by the including unit). */
extern u32 lbl_8059DCF8[];
/* 0x8059DBF0 / 0x8059DC74 - the effect id and the resource id per `eft009` type (`ef/eft009.cpp`'s spawners). */
extern "C" u16 lbl_8059DBF0[];
extern "C" u16 lbl_8059DC74[];
/* 0x806A2D34 - the 256-entry slot table that follows `eft_control` in `.bss`: a 4-byte header, then 0x18-byte records (`ef/eft_res.cpp`). */
extern "C" u8 lbl_806A2D34[];
extern u8 lbl_8059DE00[];
extern struct Vec lbl_8059DE48[];

#endif /* MHTRI_UNSPLIT_EF_TABLES_H */
