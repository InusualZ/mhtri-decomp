/* BTE/sdp_db.h - the server database entry points `BTE/sdp_db.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_SDP_DB_H
#define MHTRI_BTE_SDP_DB_H

#include "BTE/sdp_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804A02B4 / 0x804A04C8 / 0x804A0524 - the record search, the record by handle and the attribute by id range. */
tSDP_RECORD* sdp_db_service_search(tSDP_RECORD* p_rec, tSDP_UUID_SEQ* p_seq);
tSDP_RECORD* sdp_db_find_record(u32 handle);
tSDP_ATTRIBUTE* sdp_db_find_attr_in_rec(tSDP_RECORD* p_rec, u16 start_attr, u16 end_attr);

/* 0x804A0564 - 0x804A0D44 - the local record API: create, delete, add and delete attributes. */
u32 SDP_CreateRecord(void);
u8 SDP_DeleteRecord(u32 handle);
u8 SDP_AddAttribute(u32 handle, u16 attr_id, u8 attr_type, u32 attr_len, u8* p_val);
u8 SDP_AddUuidSequence(u32 handle, u16 attr_id, u16 num_uuids, u16* p_uuids);
u8 SDP_AddServiceClassIdList(u32 handle, u16 num_services, u16* p_service_uuids);
u8 SDP_DeleteAttribute(u32 handle, u16 attr_id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_SDP_DB_H */
