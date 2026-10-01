/*
 * The `ef` band's data tables with no registered owner (docs/plan.md 6.5 rule 2: the band is the home for a symbol no unit owns).
 *
 * 0x80594840 - the strategy class table `ef/ef_effectsystem.cpp`'s out-of-line constructor stores (ef_drawstrategyimpl.cpp's `fn_800C5DB8` class);
 * the data run is an unowned ambiguous one in the reconciled candidate (its reader is the effect system, its neighbours the draw strategies').
 */
#ifndef MHTRI_UNSPLIT_EF_TABLES_H
#define MHTRI_UNSPLIT_EF_TABLES_H

extern void* lbl_80594840[];

#endif /* MHTRI_UNSPLIT_EF_TABLES_H */
