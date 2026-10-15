/*
 * DWCi/gt2Auth.h - the declarations `src/DWCi/gt2Auth.c` owns (docs/plan.md 6.5 rule 2): the GT2 challenge /
 *   response helpers the connection and message files call.
 */
#ifndef MHTRI_DWCI_GT2AUTH_H
#define MHTRI_DWCI_GT2AUTH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80630880 - the GT2 challenge secret `gti2GetResponse` mixes into every response. GUESS on the name. */
extern char DWCi_gt2SecretKey[];

u8* gti2GetChallenge(u8* buffer);
u8* gti2GetResponse(u8* buffer, const u8* challenge);
int gti2CheckResponse(const u8* response1, const u8* response2);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_GT2AUTH_H */
