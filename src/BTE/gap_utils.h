/* BTE/gap_utils.h - the GAP utilities `BTE/gap_utils.c` owns: the BTM completion callbacks, the find-address-by-name
 *   steps and the BTM status conversion (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_GAP_UTILS_H
#define MHTRI_BTE_GAP_UTILS_H

#include "BTE/gap_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048CE30 - reports the BTM completion `p_data` of the request block `index` to its callback and frees the block. */
/* untyped: a caller-owned payload - the BTM completion record of the request's command */
void btm_cback(u16 index, void* p_data);

/* 0x8048D008 / 0x8048D014 - the BTM completion callbacks of request blocks 0 and 1. */
/* untyped: a caller-owned payload - the BTM completion record of the request's command */
void gap_btm_cback0(void* p1);
/* untyped: a caller-owned payload - the BTM completion record of the request's command */
void gap_btm_cback1(void* p1);

/* Traces an inquiry result and passes it to the request blocks running an inquiry (dead-stripped in retail; its two
 * trace strings stay in the unit's .data). */
void gap_inq_results_cb(tBTM_INQ_INFO* p_results);

/* 0x8048D020 - takes a remote name: ends the find-address-by-name request on a match, else names the next inquiry
 * result. */
void gap_find_addr_name_cb(tBTM_REMOTE_DEV_NAME* p);

/* 0x8048D238 - takes the end of the find-address-by-name request's inquiry and names its first result. */
void gap_find_addr_inq_cb(tBTM_INQUIRY_CMPL* p);

/* 0x8048D3EC - converts a BTM status to a GAP result. */
u16 gap_convert_btm_status(u8 btm_status);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_GAP_UTILS_H */
