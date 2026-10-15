/* DWCi/dwc_base64.h - the DWC base64 pair (src/DWCi/dwc_base64.c): the NAS request fields are encoded with it and
 *   the response tokens decoded.  Callers: DWCi/dwc_auth_interface.c, DWCi/dwc_nasfunc.c. */
#ifndef MHTRI_DWCI_DWC_BASE64_H
#define MHTRI_DWCI_DWC_BASE64_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80507050 (0x190): base64-encodes `srcLength` bytes into `dst` (at most `dstCapacity`); returns the encoded length,
 * the needed length when `dst` is NULL, or -1 when it does not fit. */
s32 DWCi_Base64Encode(const char* src, u32 srcLength, char* dst, u32 dstCapacity);

/* 0x805071E0 (0x1D8): base64-decodes `srcLength` characters into `dst` (at most `dstCapacity`); returns the decoded
 * length, the needed length when `dst` is NULL, or -1 on a bad length or too small a buffer. */
s32 DWCi_Base64Decode(const char* src, u32 srcLength, char* dst, u32 dstCapacity);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWC_BASE64_H */
