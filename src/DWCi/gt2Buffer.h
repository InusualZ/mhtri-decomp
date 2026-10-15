/*
 * DWCi/gt2Buffer.h - the declarations `src/DWCi/gt2Buffer.c` owns (docs/plan.md 6.5 rule 2): the GT2 byte buffer
 *   helpers the socket and message files call.  `GTI2Buffer` comes from `DWCi/dwc_nasfunc.h`.
 */
#ifndef MHTRI_DWCI_GT2BUFFER_H
#define MHTRI_DWCI_GT2BUFFER_H

#include "types.h"
#include "DWCi/dwc_nasfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

int DWCi_bufferAlloc(struct GTI2Buffer* buffer, u32 size);
s32 gti2GetBufferFreeSpace(const GTI2Buffer* buffer);
void gti2BufferWriteByte(GTI2Buffer* buffer, u8 b);
void gti2BufferWriteUShort(GTI2Buffer* buffer, u16 s);
void DWCi_bufferFrame(struct GTI2Buffer* buffer, const u8* data, s32 len);
void gti2BufferShorten(GTI2Buffer* buffer, s32 start, s32 shortenBy);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_GT2BUFFER_H */
