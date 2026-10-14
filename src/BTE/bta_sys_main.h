/* BTE/bta_sys_main.h - the BTA system manager entry points and data `BTE/bta_sys_main.c` owns that other units use
 *   (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_SYS_MAIN_H
#define MHTRI_BTE_BTA_SYS_MAIN_H

#include "types.h"
#include "BTE/gki_buffer.h"

/* size: 0x08 - the event handler and the disable function a BTA subsystem registers with bta_sys_register. */
typedef struct tBTA_SYS_REG {
    /* +0x00 */ u8 (*evt_hdlr)(BT_HDR* p_msg);
    /* +0x04 */ void (*disable)(void);
} tBTA_SYS_REG; /* size: 0x08 */

#ifdef __cplusplus
extern "C" {
#endif

/* .sbss 0x80795038 - the application layer's trace level. */
extern u8 appl_trace_level;

/* 0x8047B05C - registers the handlers `p_reg` of the subsystem `id`. */
void bta_sys_register(u8 id, const tBTA_SYS_REG* p_reg);

/* 0x8047B070 - posts the message `p_msg` to the BTA task. */
/* untyped: a caller-owned payload - any BTA message behind its BT_HDR */
void bta_sys_sendmsg(void* p_msg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_SYS_MAIN_H */
