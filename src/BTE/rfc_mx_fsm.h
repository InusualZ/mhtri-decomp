/* BTE/rfc_mx_fsm.h - the multiplexer state machine entry point `BTE/rfc_mx_fsm.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_RFC_MX_FSM_H
#define MHTRI_BTE_RFC_MX_FSM_H

#include "BTE/rfc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8049B314 - runs the multiplexer state machine for `event`; `p_data` is the event's payload. */
/* untyped: a caller-owned payload - its type depends on the event */
void rfc_mx_sm_execute(tRFC_MCB* p_mcb, u16 event, void* p_data);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_RFC_MX_FSM_H */
