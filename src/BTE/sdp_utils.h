/* BTE/sdp_utils.h - the utility entry points `BTE/sdp_utils.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_SDP_UTILS_H
#define MHTRI_BTE_SDP_UTILS_H

#include "BTE/sdp_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804A3A74 / 0x804A3B04 / 0x804A3B78 - CCB lookup by channel, allocation and release. */
tCONN_CB* sdpu_find_ccb_by_cid(u16 cid);
tCONN_CB* sdpu_allocate_ccb(void);
void sdpu_release_ccb(tCONN_CB* p_ccb);

/* 0x804A3BB0 / 0x804A3D80 / 0x804A3ED4 - attribute list and attribute writers, and the error response. */
u8* sdpu_build_attrib_seq(u8* p_out, u16* p_attr, u16 num_attrs);
u8* sdpu_build_attrib_entry(u8* p_out, tSDP_ATTRIBUTE* p_attr);
void sdpu_build_n_send_error(tCONN_CB* p_ccb, u16 trans_num, u16 error_code, char* p_error_text);

/* 0x804A4010 / 0x804A4360 / 0x804A45B0 - request sequence parsers and the data element length reader. */
u8* sdpu_extract_uid_seq(u8* p, u16 param_len, tSDP_UUID_SEQ* p_seq);
u8* sdpu_extract_attr_seq(u8* p, u16 param_len, tSDP_ATTR_SEQ* p_seq);
u8* sdpu_get_len_from_type(u8* p, u8 type, u32* p_len);

/* 0x804A4680 - 0x804A4A64 - UUID comparisons and the attribute filter sort. */
u8 sdpu_is_base_uuid(u8* p_uuid);
u8 sdpu_compare_uuid_arrays(u8* p_uuid1, u32 len1, u8* p_uuid2, u16 len2);
u8 sdpu_compare_uuid_with_attr(tSDP_UUID* p_btuuid, tSDP_DISC_ATTR* p_attr);
void sdpu_sort_attr_list(u16 num_attr, tSDP_DISCOVERY_DB* p_db);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_SDP_UTILS_H */
