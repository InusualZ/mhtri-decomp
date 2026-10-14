/* BTE/bta_sys_main.h - the BTA system manager entry points and data `BTE/bta_sys_main.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_SYS_MAIN_H
#define MHTRI_BTE_BTA_SYS_MAIN_H

#include "types.h"
#include "BTE/bta_sys_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x807391F8 - the BTA system control block. */
extern tBTA_SYS_CB bta_sys_cb;

/* .sbss 0x80795038 - the trace level of the BTA layer. */
extern u8 appl_trace_level;

/* 0x8047AEEC / 0x8047AF50 - initialise the system manager, or dispatch one BTA message to its subsystem. */
void bta_sys_init(void);
void bta_sys_event(BT_HDR* p_msg);

/* 0x8047B03C - ages the BTA timers unless they are disabled. */
void bta_sys_timer_update(void);

/* 0x8047B05C - registers the subsystem `id`'s event handler and disable function. */
void bta_sys_register(u8 id, const tBTA_SYS_REG* p_reg);

/* 0x8047B070 - posts `p_msg` to the BTA task's mailbox. */
/* untyped: a caller-owned payload - any BTA message behind its BT_HDR */
void bta_sys_sendmsg(void* p_msg);

/* 0x8047B08C / 0x8047B0AC - start the BTA timer entry `p_tle` with event `type` and `timeout` ms, or stop it. */
void bta_sys_start_timer(TIMER_LIST_ENT* p_tle, u16 type, s32 timeout);
void bta_sys_stop_timer(TIMER_LIST_ENT* p_tle);

/* 0x8047B0C0 - calls every registered subsystem's disable function. */
void bta_sys_disable(void);

/* 0x8047B128 - sets the trace level of the BTA layer. */
void bta_sys_set_trace_level(u8 level);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_SYS_MAIN_H */
