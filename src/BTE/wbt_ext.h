/* BTE/wbt_ext.h - the WBT extension entry point `BTE/wbt_ext.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_WBT_EXT_H
#define MHTRI_BTE_WBT_EXT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048C1B4 - creates the local DI record when there is none and adds the WBT attributes; returns whether it could. */
u8 WBT_ExtCreateRecord(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_WBT_EXT_H */
