/*
 * The `ef` band's data tables with no registered owner (docs/plan.md 6.5 rule 2: the band is the home for a symbol no unit owns).
 */
#ifndef MHTRI_UNSPLIT_EF_TABLES_H
#define MHTRI_UNSPLIT_EF_TABLES_H

#include "types.h"

/* 0x806A2D34 - the 256-entry slot table that follows `eft_control` in `.bss`: a 4-byte header, then 0x18-byte records (`ef/eft_res.cpp`). */
extern "C" u8 lbl_806A2D34[];

#endif /* MHTRI_UNSPLIT_EF_TABLES_H */
