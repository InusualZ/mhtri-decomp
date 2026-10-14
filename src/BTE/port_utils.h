/* BTE/port_utils.h - the port utilities `BTE/port_utils.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_PORT_UTILS_H
#define MHTRI_BTE_PORT_UTILS_H

#include "BTE/rfc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8049A408 - 0x8049A9E0 - MTU selection, port release and lookup, and user and peer flow control. */
void port_select_mtu(tPORT* p_port);
void port_release_port(tPORT* p_port);
tPORT* port_find_mcb_dlci_port(tRFC_MCB* p_mcb, u8 dlci);
tPORT* port_find_dlci_port(u8 dlci);
u32 port_flow_control_user(tPORT* p_port);
u32 port_get_signal_changes(tPORT* p_port, u8 old_signals, u8 signal);
void port_flow_control_peer(tPORT* p_port, u8 enable, u16 count);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_PORT_UTILS_H */
