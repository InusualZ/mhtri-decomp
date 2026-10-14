/* BTE/port_api.h - the RFCOMM control block and the entry point `BTE/port_api.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_PORT_API_H
#define MHTRI_BTE_PORT_API_H

#include "BTE/rfc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x8073CF48 - the RFCOMM control block (GUESS name rfc_cb, the BTE source's). */
extern tRFC_CB rfc_cb;

/* 0x80498FBC - clears the control block, sets its defaults and registers RFCOMM with L2CAP. */
void RFCOMM_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_PORT_API_H */
