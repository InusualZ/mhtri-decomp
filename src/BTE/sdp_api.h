/* BTE/sdp_api.h - the SDP client API entry points `BTE/sdp_api.c` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_SDP_API_H
#define MHTRI_BTE_SDP_API_H

#include "types.h"

/* The SDP types are BTE/sdp_int.h's, which reaches this header through BTE/gki_time.h: only the tags are named here. */
struct tSDP_UUID;
struct tSDP_DISC_ATTR;
struct tSDP_DISC_REC;
struct tSDP_DISCOVERY_DB;
struct tSDP_DI_RECORD;
struct tSDP_DI_GET_RECORD;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8049F44C - clears the discovery database `p_db` of `len` bytes and stores its UUID and attribute filters. */
u8 SDP_InitDiscoveryDb(struct tSDP_DISCOVERY_DB* p_db, u32 len, u16 num_uuid, struct tSDP_UUID* p_uuid_list,
                       u16 num_attr, u16* p_attr_list);

/* 0x8049F7C4 - starts a service search of the device at `p_bd_addr` into `p_db`; `p_cb` reports the result. */
u8 SDP_ServiceSearchRequest(u8* p_bd_addr, struct tSDP_DISCOVERY_DB* p_db, void (*p_cb)(u16 result));

/* 0x8049F884 - the attribute `attr_id` of the discovered record `p_rec`, or NULL. */
struct tSDP_DISC_ATTR* SDP_FindAttributeInRec(struct tSDP_DISC_REC* p_rec, u16 attr_id);

/* 0x8049F978 - the next record after `p_start_rec` in `p_db` carrying the service UUID `p_uuid`, or NULL. */
struct tSDP_DISC_REC* SDP_FindServiceUUIDInDb(struct tSDP_DISCOVERY_DB* p_db, struct tSDP_UUID* p_uuid,
                                              struct tSDP_DISC_REC* p_start_rec);

/* 0x8049FA80 / 0x8049FDE0 - register the local device identification record, or read it back. */
u16 SDP_SetLocalDiRecord(struct tSDP_DI_RECORD* p_device_info, u32* p_handle);
u16 SDP_GetLocalDiRecord(struct tSDP_DI_GET_RECORD* p_device_info, u32* p_handle);

/* 0x804A0290 - sets the SDP trace level (0xFF only reads it); returns the level. */
u8 SDP_SetTraceLevel(u8 new_level);

/* 0x8049F44C / 0x8049F820 / 0x8049F884 / 0x8049F8AC - the SDP discovery API (BTE/sdp_int.h defines the types). */
struct tSDP_DISCOVERY_DB;
struct tSDP_UUID;
struct tSDP_DISC_REC;
struct tSDP_DISC_ATTR;
u8 SDP_InitDiscoveryDb(struct tSDP_DISCOVERY_DB* p_db, u32 len, u16 num_uuid, struct tSDP_UUID* p_uuid_list,
                       u16 num_attr, u16* p_attr_list);
u8 SDP_ServiceSearchAttributeRequest(u8* p_bd_addr, struct tSDP_DISCOVERY_DB* p_db, void (*p_cb)(u16 result));
struct tSDP_DISC_ATTR* SDP_FindAttributeInRec(struct tSDP_DISC_REC* p_rec, u16 attr_id);
struct tSDP_DISC_REC* SDP_FindServiceInDb(struct tSDP_DISCOVERY_DB* p_db, u16 service_uuid,
                                         struct tSDP_DISC_REC* p_start_rec);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_SDP_API_H */
