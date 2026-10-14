/* BTE/bta_sys_cfg.h - the BTA system configuration type and pointer `BTE/bta_sys_cfg.c` owns (docs/plan.md 6.5
 *   rule 2). */
#ifndef MHTRI_BTE_BTA_SYS_CFG_H
#define MHTRI_BTE_BTA_SYS_CFG_H

#include "types.h"

/* size: 0x06 - the BTA system configuration: the BTU task mailbox and timer BTA uses and its initial trace level. */
typedef struct tBTA_SYS_CFG {
    /* +0x00 */ u16 mbox_evt;
    /* +0x02 */ u8 mbox;
    /* +0x03 */ u8 timer;
    /* +0x04 */ u8 trace_level;
    /* +0x05 */ u8 pad_0x05;
} tBTA_SYS_CFG; /* size: 0x06 */

#ifdef __cplusplus
extern "C" {
#endif

/* .sdata 0x80793D60 - the BTA system configuration in use. */
extern tBTA_SYS_CFG* p_bta_sys_cfg;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_SYS_CFG_H */
