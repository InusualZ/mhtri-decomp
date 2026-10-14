/* BTE/btm_dev.h - the device record entry points `BTE/btm_dev.c` owns that other units call, and the head of the
 *   security device record they return (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_DEV_H
#define MHTRI_BTE_BTM_DEV_H

#include "types.h"

/* size: 0x36 - the head of a security device record; only the class and name the L2CAP layer passes on are named
 * (approximation: the record continues past the name). */
typedef struct tBTM_SEC_DEV_REC {
    /* +0x00 */ u8 unused_0x00[0x22];
    /* +0x22 */ u8 dev_class[3];
    /* +0x25 */ u8 unused_0x25[0x10];
    /* +0x35 */ u8 sec_bd_name[1];
} tBTM_SEC_DEV_REC; /* size: 0x36 - approximation: the head only */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80482C80 - the security record of the device at `bd_addr`, or NULL. */
tBTM_SEC_DEV_REC* btm_find_dev(u8* bd_addr);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_DEV_H */
