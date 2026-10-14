/*
 * BTE/bta_sys_main.c - the BTA system manager: init, the event dispatcher, the timer update, the subsystem
 *   registration, message send, timer start and stop, disable and the trace level.
 *
 * RANGE. .text 0x8047AEEC-0x8047B130 (9 functions, 0x244 B); .data 0x80612ED0-0x80612F08; .bss 0x807391F8-0x80739288;
 *   .sbss 0x80795038-0x80795040. Strong start: the .data starts after an 8-byte pad (0x80612ECB -> 0x80612ED0) and its
 *   two strings are read only by bta_sys_event; bta_sys_init is Broadcom's first bta_sys_main.c function. Medium end:
 *   ptim_init follows (the ptim_* names); the next .data object (0x80612F08) is read first by
 *   bta_dm_disable_timer_cback.
 *   The control block 0x807391F8 (bta_sys_cb, 0x8C B) is read by BTE/bta_sys_conn.c and this range; the .sbss byte
 *   0x80795038 (appl_trace_level) is written by bta_sys_set_trace_level and read by every BTA function from
 *   bta_sys_init on. Both are Broadcom bta_sys_main.c globals; BTE/bd.c and BTE/bta_sys_conn.c carry no data, so the
 *   layout is the same with them in either file. The cfg pointer 0x80793D60 (p_bta_sys_cfg) that bta_sys_init and
 *   bta_sys_sendmsg read stays in BTE/gki_time.c (its header has the order evidence).
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, the compiler of every measured BTE unit (BTE/l2c_utils.c FLAGS line).
 * NAMES. the map's and the BTE source's: bta_sys_register (0x8047B05C, stores the registration in the control block's
 *   per-subsystem table), bta_sys_cb (0x807391F8), appl_trace_level (0x80795038).
 * RESIDUALS. the target's .bss/.sbss rows carry the link pad (0x90/8 B against 0x8C/1 B).
 * SHAPES. plain C.
 */

#include "BTE/bta_sys_main.h"
#include "BTE/bta_sys_cfg.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "BTE/bta_dm_act.h"   /* bta_dm_immediate_disable */

/* The period (ms) of the BTA protocol timer. */
#define BTA_SYS_TIMER_PERIOD 1000

/* The trace levels and the BTA layer's trace masks. */
#define BT_TRACE_LEVEL_WARNING 2
#define BT_TRACE_LEVEL_EVENT 4
#define TRACE_APPL_WARNING 0x00000501
#define TRACE_APPL_EVENT 0x00000503

tBTA_SYS_CB bta_sys_cb;
u8 appl_trace_level;

/* 0x8047AEEC (0x64): Clears the control block, starts the protocol timer and takes the task id and the trace level. */
void bta_sys_init(void) {
    memset(&bta_sys_cb, 0, sizeof(tBTA_SYS_CB));
    ptim_init(&bta_sys_cb.ptim_cb, BTA_SYS_TIMER_PERIOD, p_bta_sys_cfg->timer);
    bta_sys_cb.task_id = GKI_get_taskid();
    appl_trace_level = p_bta_sys_cfg->trace_level;
}

/* 0x8047AF50 (0xEC): Dispatches the BTA message `p_msg` to the subsystem its event names, then frees it if allowed. */
void bta_sys_event(BT_HDR* p_msg) {
    u8 id;
    u8 freebuf = TRUE;

    if (appl_trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_1(TRACE_APPL_EVENT, "BTA got event 0x%x", p_msg->event);
    }

    id = (u8)(p_msg->event >> 8);

    if (bta_sys_cb.events_disabled) {
        if (p_msg->event == BTA_DM_API_DISABLE_EVT) {
            bta_dm_immediate_disable();
        }
        GKI_freebuf(p_msg);
        return;
    }

    if (id < BTA_ID_MAX && bta_sys_cb.reg[id] != NULL) {
        freebuf = (*bta_sys_cb.reg[id]->evt_hdlr)(p_msg);
    } else if (appl_trace_level >= BT_TRACE_LEVEL_WARNING) {
        LogMsg_1(TRACE_APPL_WARNING, "BTA got unregistered event id %d", id);
    }

    if (freebuf) {
        GKI_freebuf(p_msg);
    }
}

/* 0x8047B03C (0x20): Ages the BTA timers unless they are disabled. */
void bta_sys_timer_update(void) {
    if (!bta_sys_cb.timers_disabled) {
        ptim_timer_update(&bta_sys_cb.ptim_cb);
    }
}

/* 0x8047B05C (0x14): Registers the subsystem `id`'s event handler and disable function. */
void bta_sys_register(u8 id, const tBTA_SYS_REG* p_reg) {
    bta_sys_cb.reg[id] = p_reg;
}

/* 0x8047B070 (0x1C): Posts `p_msg` to the BTA task's mailbox. */
/* untyped: a caller-owned payload - any BTA message behind its BT_HDR */
void bta_sys_sendmsg(void* p_msg) {
    GKI_send_msg(bta_sys_cb.task_id, p_bta_sys_cfg->mbox, p_msg);
}

/* 0x8047B08C (0x20): Starts the BTA timer entry `p_tle` with event `type` and `timeout` ms. */
void bta_sys_start_timer(TIMER_LIST_ENT* p_tle, u16 type, s32 timeout) {
    ptim_start_timer(&bta_sys_cb.ptim_cb, p_tle, type, timeout);
}

/* 0x8047B0AC (0x14): Stops the BTA timer entry `p_tle`. */
void bta_sys_stop_timer(TIMER_LIST_ENT* p_tle) {
    ptim_stop_timer(&bta_sys_cb.ptim_cb, p_tle);
}

/* 0x8047B0C0 (0x68): Calls every registered subsystem's disable function. */
void bta_sys_disable(void) {
    int i;

    for (i = 0; i < BTA_ID_MAX; i++) {
        if (bta_sys_cb.reg[i] != NULL && bta_sys_cb.reg[i]->disable != NULL) {
            (*bta_sys_cb.reg[i]->disable)();
        }
    }
}

/* 0x8047B128 (0x8): Sets the trace level of the BTA layer. */
void bta_sys_set_trace_level(u8 level) {
    appl_trace_level = level;
}
