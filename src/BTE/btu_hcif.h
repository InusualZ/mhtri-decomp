/* BTE/btu_hcif.h - the BTU HCI interface entry points `BTE/btu_hcif.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTU_HCIF_H
#define MHTRI_BTE_BTU_HCIF_H

#include "types.h"
#include "BTE/gki_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048AEE0 - dispatches the HCI event in `p_msg` to the layer that handles it. */
void btu_hcif_process_event(BT_HDR* p_msg);

/* 0x8048B498 - sends the HCI command `p_buf` while the controller has credits, queueing it otherwise (NULL drains the
 * queue). */
void btu_hcif_send_cmd(BT_HDR* p_buf);

/* 0x8048BF04 - handles the expiry of the command response timer. */
void btu_hcif_cmd_timeout(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTU_HCIF_H */
