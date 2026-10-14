/* BTE/btm_main.h - the BTM control block `BTE/btm_main.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_MAIN_H
#define MHTRI_BTE_BTM_MAIN_H

#include "types.h"

/* size: 0x27C8 - the BTM control block; only the fields the L2CAP layer reads are named. */
typedef struct tBTM_CB {
    /* +0x0000 */ u8 unused_0x0000[0x640];
    /* +0x0640 */ u8 local_features[8];
    /* +0x0648 */ u8 unused_0x0648[0x2177];
    /* +0x27BF */ u8 acl_disc_reason;
    /* +0x27C0 */ u8 unused_0x27C0[8];
} tBTM_CB; /* size: 0x27C8 */

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x80739698 - the BTM control block. */
extern tBTM_CB btm_cb;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_MAIN_H */
