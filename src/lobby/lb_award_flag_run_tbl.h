/* `lb_award_flag_run_tbl`, defined by `lobby/lb_npc.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_AWARD_FLAG_RUN_TBL_H
#define MHTRI_LOBBY_LB_AWARD_FLAG_RUN_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The award flag-run tables: per award, a list of `{count, first flag}` halfword pairs ending in a count of -1. */
extern s16* lb_award_flag_run_tbl[10];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_AWARD_FLAG_RUN_TBL_H */
