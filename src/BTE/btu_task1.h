/* BTE/btu_task1.h - the BTU control block and the BTU timer entry points `BTE/btu_task1.c` owns (docs/plan.md 6.5
 *   rule 2). */
#ifndef MHTRI_BTE_BTU_TASK1_H
#define MHTRI_BTE_BTU_TASK1_H

#include "types.h"

/* The timer list entry; BTE/gki_time.h defines it. */
struct TIMER_LIST_ENT;

/* size: 0x88 - the BTU control block; only the ACL sizes the L2CAP layer reads are named. */
typedef struct tBTU_CB {
    /* +0x00 */ u8 unused_0x00[0x7C];
    /* +0x7C */ u16 hcit_acl_data_size;
    /* +0x7E */ u16 hcit_acl_pkt_size;
    /* +0x80 */ u8 unused_0x80[8];
} tBTU_CB; /* size: 0x88 */

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x80739170 - the BTU control block. */
extern tBTU_CB btu_cb;

/* 0x8047AB40 / 0x8047ABB0 - start the timer entry with `timeout` seconds of type `type`, or stop it. */
void btu_start_timer(struct TIMER_LIST_ENT* p_tle, u16 type, u32 timeout);
void btu_stop_timer(struct TIMER_LIST_ENT* p_tle);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTU_TASK1_H */
