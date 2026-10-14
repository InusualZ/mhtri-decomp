/*
 * BTE/ptim.c - the BTA protocol timer service: init, the timer update and timer start and stop.
 *
 * RANGE. .text 0x8047B130-0x8047B310 (4 functions, 0x1E0 B); no data. Medium: the ptim_* run between
 *   bta_sys_set_trace_level (the last BTE/bta_sys_main.c function) and utl_freebuf; no data on either side pins it.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, the compiler of every measured BTE unit (BTE/l2c_utils.c FLAGS line).
 * NAMES. the map's.
 * RESIDUALS. none known.
 * SHAPES. plain C.
 */

#include "BTE/ptim.h"
#include "BTE/bta_sys_main.h"

/* Converts a period in ms to GKI ticks (10 ms each). */
#define GKI_MS_TO_TICKS(x) ((x) / 10)

/* 0x8047B130 (0x4C): Clears the timer list and sets the update period (ms) and the GKI timer. */
void ptim_init(tPTIM_CB* p_cb, u16 period, u8 timer_id) {
    GKI_init_timer_list(&p_cb->timer_queue);
    p_cb->period = period;
    p_cb->timer_id = timer_id;
}

/* 0x8047B17C (0xB4): Ages the list by one period and runs or posts every entry that expired. */
void ptim_timer_update(tPTIM_CB* p_cb) {
    TIMER_LIST_ENT* p_tle;
    BT_HDR* p_msg;

    GKI_update_timer_list(&p_cb->timer_queue, p_cb->period);

    while ((p_tle = p_cb->timer_queue.p_first) != NULL && p_tle->ticks <= 0) {
        GKI_remove_from_timer_list(&p_cb->timer_queue, p_tle);

        if (p_tle->p_cback) {
            (*p_tle->p_cback)(p_tle);
        } else if (p_tle->event) {
            if ((p_msg = GKI_getbuf(sizeof(BT_HDR))) != NULL) {
                p_msg->event = p_tle->event;
                bta_sys_sendmsg(p_msg);
            }
        }
    }

    if (p_cb->timer_queue.p_first == NULL) {
        GKI_stop_timer(p_cb->timer_id);
    }
}

/* 0x8047B230 (0xA0): Starts the entry `p_tle` with event `type` and `timeout` ms, starting the GKI timer for the first. */
void ptim_start_timer(tPTIM_CB* p_cb, TIMER_LIST_ENT* p_tle, u16 type, s32 timeout) {
    if (p_cb->timer_queue.p_first == NULL) {
        GKI_start_timer(p_cb->timer_id, GKI_MS_TO_TICKS(p_cb->period), TRUE);
    }

    GKI_remove_from_timer_list(&p_cb->timer_queue, p_tle);

    p_tle->event = type;
    p_tle->ticks = timeout;

    GKI_add_to_timer_list(&p_cb->timer_queue, p_tle);
}

/* 0x8047B2D0 (0x40): Stops the entry `p_tle`, stopping the GKI timer when the list empties. */
void ptim_stop_timer(tPTIM_CB* p_cb, TIMER_LIST_ENT* p_tle) {
    GKI_remove_from_timer_list(&p_cb->timer_queue, p_tle);

    if (p_cb->timer_queue.p_first == NULL) {
        GKI_stop_timer(p_cb->timer_id);
    }
}
