/* nw4r/DWCi_Base64Encode.h - the DWC base64 pair the `nw4r/fn_8050661C.cpp` range ends with (docs/plan.md 6.5 rule 2,
 *   leaf header): `DWCi_Base64Encode` (0x80507050) and `DWCi_Base64Decode` (0x805071E0).  Their only callers are the
 *   DWC auth units; the seam between that range and the DWC band is undecided. */
#ifndef MHTRI_NW4R_DWCI_BASE64ENCODE_H
#define MHTRI_NW4R_DWCI_BASE64ENCODE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80507050 - base64-encodes `srcLength` bytes into `dst` (at most `dstCapacity`); answers the encoded length. */
s32 DWCi_Base64Encode(const char* src, u32 srcLength, char* dst, u32 dstCapacity);

/* 0x805071E0 - base64-decodes `srcLength` characters into `dst` (at most `dstCapacity`); answers the decoded length. */
s32 DWCi_Base64Decode(const char* src, u32 srcLength, char* dst, u32 dstCapacity);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NW4R_DWCI_BASE64ENCODE_H */
