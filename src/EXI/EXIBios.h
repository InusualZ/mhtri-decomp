/*
 * EXI/EXIBios.h - the EXI bus driver's public interface (the symbols `EXI/EXIBios.c` owns that other units call).
 */
#ifndef EXI_EXIBIOS_H
#define EXI_EXIBIOS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { EXI_CHAN_0, EXI_CHAN_1, EXI_CHAN_2, EXI_MAX_CHAN } EXIChannel;
typedef enum { EXI_READ, EXI_WRITE, EXI_TYPE_2, EXI_MAX_TYPE } EXIType;
typedef enum { EXI_DEV_EXT, EXI_DEV_INT, EXI_DEV_NET, EXI_MAX_DEV } EXIDev;
typedef enum {
    EXI_FREQ_1MHZ,
    EXI_FREQ_2MHZ,
    EXI_FREQ_4MHZ,
    EXI_FREQ_8MHZ,
    EXI_FREQ_16MHZ,
    EXI_FREQ_32HZ,
    EXI_MAX_FREQ
} EXIFreq;

struct OSContext;
typedef void (*EXICallback)(EXIChannel chan, struct OSContext* context);

/* untyped: the byte range of a transfer */
BOOL EXIImm(EXIChannel chan, void* buf, s32 len, u32 type, EXICallback callback);
/* untyped: the byte range of a transfer */
BOOL EXIImmEx(EXIChannel chan, void* buf, s32 len, u32 type);
/* untyped: the byte range of a transfer */
BOOL EXIDma(EXIChannel chan, void* buf, s32 len, u32 type, EXICallback callback);
BOOL EXISync(EXIChannel chan);
EXICallback EXISetExiCallback(EXIChannel chan, EXICallback callback);
BOOL EXIAttach(EXIChannel chan, EXICallback callback);
BOOL EXIDetach(EXIChannel chan);
BOOL EXISelect(EXIChannel chan, u32 dev, u32 freq);
BOOL EXIDeselect(EXIChannel chan);
void EXIInit(void);
BOOL EXILock(EXIChannel chan, u32 dev, EXICallback callback);
BOOL EXIUnlock(EXIChannel chan);
BOOL EXIGetID(EXIChannel chan, u32 dev, u32* id);

#ifdef __cplusplus
}
#endif

#endif
