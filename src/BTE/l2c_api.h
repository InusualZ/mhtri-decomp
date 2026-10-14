/* BTE/l2c_api.h - the L2CAP client API entry points `BTE/l2c_api.c` owns and the configuration and callback types they
 *   take (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_L2C_API_H
#define MHTRI_BTE_L2C_API_H

#include "types.h"
#include "BTE/gki_buffer.h"

/* size: 0x18 - the HCI flow specification of an L2CAP configuration. */
typedef struct FLOW_SPEC {
    /* +0x00 */ u8 qos_flags;
    /* +0x01 */ u8 service_type;
    /* +0x04 */ u32 token_rate;
    /* +0x08 */ u32 token_bucket_size;
    /* +0x0C */ u32 peak_bandwidth;
    /* +0x10 */ u32 latency;
    /* +0x14 */ u32 delay_variation;
} FLOW_SPEC; /* size: 0x18 */

/* size: 0x0A - the retransmission and flow control option of an L2CAP configuration. */
typedef struct tL2CAP_FCR_OPTS {
    /* +0x00 */ u8 mode;
    /* +0x01 */ u8 tx_win_sz;
    /* +0x02 */ u8 max_transmit;
    /* +0x04 */ u16 rtrans_tout;
    /* +0x06 */ u16 mon_tout;
    /* +0x08 */ u16 mps;
} tL2CAP_FCR_OPTS; /* size: 0x0A */

/* size: 0x3C - an L2CAP channel configuration and the continuation flags of its command. */
typedef struct tL2CAP_CFG_INFO {
    /* +0x00 */ u16 result;
    /* +0x02 */ u8 mtu_present;
    /* +0x04 */ u16 mtu;
    /* +0x06 */ u8 qos_present;
    /* +0x08 */ FLOW_SPEC qos;
    /* +0x20 */ u8 flush_to_present;
    /* +0x22 */ u16 flush_to;
    /* +0x24 */ u8 fcr_present;
    /* +0x26 */ tL2CAP_FCR_OPTS fcr;
    /* +0x30 */ u8 unused_0x30[8];
    /* +0x38 */ u16 flags;
    /* +0x3A */ u8 pad_0x3A[2];
} tL2CAP_CFG_INFO; /* size: 0x3C */

/* The callbacks an L2CAP client registers for its PSM. */
typedef void (tL2CA_CONNECT_IND_CB)(u8* bd_addr, u16 l2cap_cid, u16 psm, u8 l2cap_id);
typedef void (tL2CA_CONNECT_CFM_CB)(u16 l2cap_cid, u16 result);
typedef void (tL2CA_CONNECT_PND_CB)(u16 l2cap_cid);
typedef void (tL2CA_CONFIG_IND_CB)(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg);
typedef void (tL2CA_CONFIG_CFM_CB)(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg);
typedef void (tL2CA_DISCONNECT_IND_CB)(u16 l2cap_cid, u8 ack_needed);
typedef void (tL2CA_DISCONNECT_CFM_CB)(u16 l2cap_cid, u16 result);
typedef void (tL2CA_QOS_VIOLATION_IND_CB)(u8* bd_addr);
typedef void (tL2CA_DATA_IND_CB)(u16 l2cap_cid, BT_HDR* p_buf);
typedef void (tL2CA_CONGESTION_STATUS_CB)(u16 l2cap_cid, u8 congested);

/* size: 0x28 - the callback set an L2CAP client registers. */
typedef struct tL2CAP_APPL_INFO {
    /* +0x00 */ tL2CA_CONNECT_IND_CB* pL2CA_ConnectInd_Cb;
    /* +0x04 */ tL2CA_CONNECT_CFM_CB* pL2CA_ConnectCfm_Cb;
    /* +0x08 */ tL2CA_CONNECT_PND_CB* pL2CA_ConnectPnd_Cb;
    /* +0x0C */ tL2CA_CONFIG_IND_CB* pL2CA_ConfigInd_Cb;
    /* +0x10 */ tL2CA_CONFIG_CFM_CB* pL2CA_ConfigCfm_Cb;
    /* +0x14 */ tL2CA_DISCONNECT_IND_CB* pL2CA_DisconnectInd_Cb;
    /* +0x18 */ tL2CA_DISCONNECT_CFM_CB* pL2CA_DisconnectCfm_Cb;
    /* +0x1C */ tL2CA_QOS_VIOLATION_IND_CB* pL2CA_QoSViolationInd_Cb;
    /* +0x20 */ tL2CA_DATA_IND_CB* pL2CA_DataInd_Cb;
    /* +0x24 */ tL2CA_CONGESTION_STATUS_CB* pL2CA_CongestionStatus_Cb;
} tL2CAP_APPL_INFO; /* size: 0x28 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80492E94 - 0x80493754 - the L2CAP client API: register a PSM, open, configure and close a channel, send data. */
u16 L2CA_Register(u16 psm, tL2CAP_APPL_INFO* p_cb_info);
u16 L2CA_ConnectReq(u16 psm, u8* p_bd_addr);
u8 L2CA_ConnectRsp(u8* p_bd_addr, u8 id, u16 lcid, u16 result, u16 status);
u8 L2CA_ConfigReq(u16 cid, tL2CAP_CFG_INFO* p_cfg);
u8 L2CA_ConfigRsp(u16 cid, tL2CAP_CFG_INFO* p_cfg);
u8 L2CA_DisconnectReq(u16 cid);
u8 L2CA_DisconnectRsp(u16 cid);
u8 L2CA_DataWrite(u16 cid, BT_HDR* p_data);

/* 0x80493A40 - sets the L2CAP trace level (0xFF keeps it); returns the level. */
u8 L2CA_SetTraceLevel(u8 new_level);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_L2C_API_H */
