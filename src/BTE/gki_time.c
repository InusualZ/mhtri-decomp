/*
 * BTE/gki_time.c - the GKI timer API of the Broadcom BTE Bluetooth stack: the tick count, the task timers and the
 *   timer lists the BTU and the protocol layers link their entries into.
 *
 * RANGE. .text 0x80478790-0x80478D20 (8 functions); no data. Strong start: 0x80478790 (gki_timers_init) follows
 *   GKI_delete_pool, the last function of BTE/gki_buffer.c, whose .bss holds gki_cb, the control block these
 *   functions read. Medium end: GKI_init (0x80478D20) opens BTE/gki_ppc.c - Broadcom's gki_time.c ends with the timer
 *   list functions and the OS glue of gki_ppc.c follows; no data on either side.
 * FLAGS. Wii/1.3 with -O4,p: the loop of GKI_update_timer_list is entered over a `nop` that aligns its head, which
 *   Wii/1.3 emits and GC/3.0a5.2 + cflags_nw4r does not (the probe matches 36 of 36 instructions under Wii/1.3, 3 of
 *   36 under GC). Every function start of 0x80478790-0x8047ABC4 is 16-aligned (61 of 61), as in BTE/gki_buffer.c
 *   (`cflags_base`); the lib default `cflags_os` packs to 4 bytes, which is why the unit builds with `cflags_base`.
 * NAMES. the map's; GKI_init_timer_list (0x80478A50) is the shared dump's name. GUESS BTA_DisableBluetooth
 *   (0x8047D718 in BTE/bta_dm_api.c, declared in BTE/gki_time.h: the bta_dm_api entry after BTA_EnableBluetooth,
 *   called from the WUD shutdown fiber).
 * RESIDUALS. none known.
 * SHAPES. plain C; gki_adjust_timer_count is a static helper inlined into GKI_start_timer (the target carries no
 *   copy of it). BTE/gki_time.h holds the timer list types and includes the headers of the files cut from this head.
 */

#include "BTE/gki_time.h"

#define GKI_TIMER_0 0
#define GKI_TIMER_1 1

#define GKI_INT32_MAX 0x7FFFFFFF
#define GKI_UNUSED_LIST_ENTRY ((s32)0x80000000)

/* 0x80478790 (0xBC): Clears the task timers and the tick bookkeeping. */
void gki_timers_init(void) {
    u8 tt;

    gki_cb.com.OSTicksTilExp = 0;
    gki_cb.com.OSNumOrigTicks = 0;

    for (tt = 0; tt < GKI_MAX_TASKS; tt++) {
        gki_cb.com.OSWaitTmr[tt] = 0;
        gki_cb.com.OSTaskTmr0[tt] = 0;
        gki_cb.com.OSTaskTmr0R[tt] = 0;
        gki_cb.com.OSTaskTmr1[tt] = 0;
        gki_cb.com.OSTaskTmr1R[tt] = 0;
    }
}

/* 0x80478850 (0x14): Returns the tick count since GKI started. */
u32 GKI_get_tick_count(void) {
    return gki_cb.com.OSTicks;
}

/* Moves the first expiry forward when a newly started timer of `ticks` ticks expires before it. */
static inline void gki_adjust_timer_count(s32 ticks) {
    if (ticks > 0) {
        if (gki_cb.com.OSNumOrigTicks == 0 || (ticks < gki_cb.com.OSTicksTilExp && gki_cb.com.OSTicksTilExp > 0)) {
            gki_cb.com.OSNumOrigTicks = (gki_cb.com.OSNumOrigTicks - gki_cb.com.OSTicksTilExp) + ticks;
            gki_cb.com.OSTicksTilExp = ticks;
        }
    }
}

/* 0x80478870 (0x14C): Starts task timer `tnum` of the calling task, reloading it when `is_continuous`. */
void GKI_start_timer(u8 tnum, s32 ticks, u8 is_continuous) {
    s32 reload;
    s32 orig_ticks;
    u8 task_id = GKI_get_taskid();
    u8 bad_timer = FALSE;

    if (ticks <= 0) {
        ticks = 1;
    }

    orig_ticks = ticks;

    if (is_continuous) {
        reload = ticks;
    } else {
        reload = 0;
    }

    GKI_disable();

    if (GKI_INT32_MAX - (gki_cb.com.OSNumOrigTicks - gki_cb.com.OSTicksTilExp) > ticks) {
        ticks += gki_cb.com.OSNumOrigTicks - gki_cb.com.OSTicksTilExp;
    } else {
        ticks = GKI_INT32_MAX;
    }

    switch (tnum) {
    case GKI_TIMER_0:
        gki_cb.com.OSTaskTmr0R[task_id] = reload;
        gki_cb.com.OSTaskTmr0[task_id] = ticks;
        break;
    case GKI_TIMER_1:
        gki_cb.com.OSTaskTmr1R[task_id] = reload;
        gki_cb.com.OSTaskTmr1[task_id] = ticks;
        break;
    default:
        bad_timer = TRUE;
    }

    if (!bad_timer) {
        gki_adjust_timer_count(orig_ticks);
    }

    GKI_enable();
}

/* 0x804789C0 (0x84): Stops task timer `tnum` of the calling task. */
void GKI_stop_timer(u8 tnum) {
    u8 task_id = GKI_get_taskid();

    switch (tnum) {
    case GKI_TIMER_0:
        gki_cb.com.OSTaskTmr0R[task_id] = 0;
        gki_cb.com.OSTaskTmr0[task_id] = 0;
        break;
    case GKI_TIMER_1:
        gki_cb.com.OSTaskTmr1R[task_id] = 0;
        gki_cb.com.OSTaskTmr1[task_id] = 0;
        break;
    }
}

/* 0x80478A50 (0x14): Empties a timer list. */
void GKI_init_timer_list(TIMER_LIST_Q* p_timer_listq) {
    p_timer_listq->p_first = NULL;
    p_timer_listq->p_last = NULL;
    p_timer_listq->last_ticks = 0;
}

/* 0x80478A70 (0x90): Counts ticks off a timer list; returns the number of entries that have expired. */
s32 GKI_update_timer_list(TIMER_LIST_Q* p_timer_listq, s32 num_units_since_last_update) {
    TIMER_LIST_ENT* p_tle;
    s32 num_time_out = 0;
    s32 rem_ticks;
    s32 temp_ticks;

    p_tle = p_timer_listq->p_first;

    while (p_tle != NULL && p_tle->ticks <= 0) {
        num_time_out++;
        p_tle = p_tle->p_next;
    }

    rem_ticks = num_units_since_last_update;

    while (p_tle != NULL && rem_ticks > 0) {
        temp_ticks = p_tle->ticks;
        p_tle->ticks -= rem_ticks;

        if (p_tle->ticks <= 0) {
            p_tle->ticks = 0;
            num_time_out++;
        }

        rem_ticks -= temp_ticks;
        p_tle = p_tle->p_next;
    }

    if (p_timer_listq->last_ticks > 0) {
        p_timer_listq->last_ticks -= num_units_since_last_update;

        if (p_timer_listq->last_ticks < 0) {
            p_timer_listq->last_ticks = 0;
        }
    }

    return num_time_out;
}

/* 0x80478B00 (0xEC): Links an entry into a timer list in expiry order, keeping each entry's ticks relative. */
void GKI_add_to_timer_list(TIMER_LIST_Q* p_timer_listq, TIMER_LIST_ENT* p_tle) {
    s32 nr_ticks_total;
    TIMER_LIST_ENT* p_temp;

    if (p_tle->ticks >= 0) {
        if (p_tle->ticks >= p_timer_listq->last_ticks) {
            if (p_timer_listq->p_first == NULL) {
                p_timer_listq->p_first = p_tle;
            } else {
                if (p_timer_listq->p_last != NULL) {
                    p_timer_listq->p_last->p_next = p_tle;
                }
                p_tle->p_prev = p_timer_listq->p_last;
            }

            p_tle->p_next = NULL;
            p_timer_listq->p_last = p_tle;
            nr_ticks_total = p_tle->ticks;
            p_tle->ticks -= p_timer_listq->last_ticks;
            p_timer_listq->last_ticks = nr_ticks_total;
        } else {
            p_temp = p_timer_listq->p_first;
            while (p_tle->ticks > p_temp->ticks) {
                if (p_temp->ticks > 0) {
                    p_tle->ticks -= p_temp->ticks;
                }
                p_temp = p_temp->p_next;
            }

            if (p_temp == p_timer_listq->p_first) {
                p_tle->p_next = p_timer_listq->p_first;
                p_timer_listq->p_first->p_prev = p_tle;
                p_timer_listq->p_first = p_tle;
            } else {
                p_temp->p_prev->p_next = p_tle;
                p_tle->p_prev = p_temp->p_prev;
                p_temp->p_prev = p_tle;
                p_tle->p_next = p_temp;
            }
            p_temp->ticks -= p_tle->ticks;
        }

        p_tle->in_use = TRUE;
    }
}

/* 0x80478BF0 (0x124): Unlinks an entry from a timer list and gives its remaining ticks to the entry after it. */
void GKI_remove_from_timer_list(TIMER_LIST_Q* p_timer_listq, TIMER_LIST_ENT* p_tle) {
    if (p_tle == NULL || p_tle->in_use == FALSE || p_timer_listq->p_first == NULL) {
        return;
    }

    if (p_tle->p_next != NULL) {
        p_tle->p_next->ticks += p_tle->ticks;
    } else {
        p_timer_listq->last_ticks -= p_tle->ticks;
    }

    if (p_timer_listq->p_first == p_tle) {
        p_timer_listq->p_first = p_tle->p_next;

        if (p_timer_listq->p_first != NULL) {
            p_timer_listq->p_first->p_prev = NULL;
        }

        if (p_timer_listq->p_last == p_tle) {
            p_timer_listq->p_last = NULL;
        }
    } else {
        if (p_timer_listq->p_last == p_tle) {
            p_timer_listq->p_last = p_tle->p_prev;

            if (p_timer_listq->p_last != NULL) {
                p_timer_listq->p_last->p_next = NULL;
            }
        } else {
            if (p_tle->p_next != NULL && p_tle->p_next->p_prev == p_tle) {
                p_tle->p_next->p_prev = p_tle->p_prev;
            } else {
                return;
            }

            if (p_tle->p_prev != NULL && p_tle->p_prev->p_next == p_tle) {
                p_tle->p_prev->p_next = p_tle->p_next;
            } else {
                return;
            }
        }
    }

    p_tle->p_next = p_tle->p_prev = NULL;
    p_tle->ticks = GKI_UNUSED_LIST_ENTRY;
    p_tle->in_use = FALSE;
}
