/* BTE/l2c_csm.h - the L2CAP channel state machine entry point `BTE/l2c_csm.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_L2C_CSM_H
#define MHTRI_BTE_L2C_CSM_H

#include "BTE/l2c_int.h"

/* 0x80493A60 - runs `event` through the state handler of the channel's current state; `p_data` is the event's
 * connection info, configuration or buffer. */
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
void l2c_csm_execute(tL2C_CCB* p_ccb, u8 event, void* p_data);

#endif /* MHTRI_BTE_L2C_CSM_H */
