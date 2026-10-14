/* BTE/ptim.h - the BTA protocol timer service `BTE/ptim.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_PTIM_H
#define MHTRI_BTE_PTIM_H

#include "types.h"
#include "BTE/gki_time.h"

/* size: 0x14 - a protocol timer: a list of timer entries aged every `period` ms by the GKI timer `timer_id`. */
typedef struct tPTIM_CB {
    /* +0x00 */ TIMER_LIST_Q timer_queue;
    /* +0x0C */ s32 period;
    /* +0x10 */ u8 timer_id;
    /* +0x11 */ u8 pad_0x11[3];
} tPTIM_CB; /* size: 0x14 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047B130 - clears the timer list and sets the update period (ms) and the GKI timer. */
void ptim_init(tPTIM_CB* p_cb, u16 period, u8 timer_id);

/* 0x8047B17C - ages the list by one period and runs or posts every entry that expired. */
void ptim_timer_update(tPTIM_CB* p_cb);

/* 0x8047B230 / 0x8047B2D0 - start the entry `p_tle` with event `type` and `timeout` ms, or stop it. */
void ptim_start_timer(tPTIM_CB* p_cb, TIMER_LIST_ENT* p_tle, u16 type, s32 timeout);
void ptim_stop_timer(tPTIM_CB* p_cb, TIMER_LIST_ENT* p_tle);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_PTIM_H */
