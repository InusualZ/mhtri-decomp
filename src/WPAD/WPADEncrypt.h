/* WPAD/WPADEncrypt.h - the extension key generators and the decryption `WPAD/WPADEncrypt.cpp` owns (docs/plan.md 6.5 rule 2). */
#ifndef WPAD_WPADENCRYPT_H
#define WPAD_WPADENCRYPT_H

#include "types.h"
#include "WPAD/wpad.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804F72C0 / 0x804F79B0 - build the channel's extension key tables from a fresh OSGetTick-seeded key. */
void WPADiCreateKey(s32 chan);
void WPADiCreateKeyFor3rd(s32 chan);
/* 0x804F80B0 - decrypts extension data in place with the channel's key tables. */
void wpadDecryptExtension(s32 chan, u8* data, u16 length, s32 offset);

#ifdef __cplusplus
}
#endif

#endif /* WPAD_WPADENCRYPT_H */
