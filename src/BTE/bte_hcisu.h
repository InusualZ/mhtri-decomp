/* BTE/bte_hcisu.h - the HCI transport entry points `BTE/bte_hcisu.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTE_HCISU_H
#define MHTRI_BTE_BTE_HCISU_H

#include "types.h"
#include "BTE/gki_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047A370 - hands the message `p_msg` with the event `event` to the HCI transport. */
void bte_hcisu_send(BT_HDR* p_msg, u16 event);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTE_HCISU_H */
