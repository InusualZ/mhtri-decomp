/* BTE/l2c_main.h - the L2CAP main module entry points and the control block `BTE/l2c_main.c` owns: l2c_init, the ACL
 *   receive path, the timeout dispatcher and the held-packet queue (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_L2C_MAIN_H
#define MHTRI_BTE_L2C_MAIN_H

#include "BTE/l2c_int.h"

/* .bss 0x8073C760 - the L2CAP control block. */
extern tL2C_CB l2cb;

/* 0x804960C8 - clears the control block and chains the free channel list. */
void l2c_init(void);

/* 0x804961DC - dispatches one received ACL packet to the signalling code or its channel. */
void l2c_rcv_acl_data(BT_HDR* p_msg);

/* 0x80496F7C - dispatches an expired L2CAP timer. */
void l2c_process_timeout(TIMER_LIST_ENT* p_tle);

/* 0x80496FCC - replays the packets held for links that were not up yet. */
void l2c_process_held_packets(u8 timed_out);

#endif /* MHTRI_BTE_L2C_MAIN_H */
