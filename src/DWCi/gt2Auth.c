/*
 * DWCi/gt2Auth.c - the GameSpy GT2 connection challenge: a fresh challenge, the response to a challenge (keyed with
 *   the secret) and the response check.
 *
 * RANGE. `.text` 0x8050C950..0x8050CD20 (3 functions); `.data` 0x80630880..0x806308A8 (the secret key). The left edge
 *   follows the availability probe's last function, the right edge is gt2Buffer's first (`DWCi/gt2Buffer.c`); the
 *   probe's "%s.available..." string ends at 0x80630879 and the key starts on the next 8-byte boundary (0x80630880,
 *   past the 4-byte one), the object seam.
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned (`gti2CheckResponse` keeps a
 *   loop-alignment `nop`).
 * NAMES. GameSpy SDK names (`gti2GetChallenge`, `gti2GetResponse`, `gti2CheckResponse`); GUESS `DWCi_gt2SecretKey`.
 *   The file name is the GameSpy SDK's `gt2Auth.c` (no `__FILE__` string).
 * RESIDUALS. `gti2GetChallenge` and `gti2GetResponse` colour the parity step's scratch registers differently
 *   (the xor chain's operands land in r3/r4/r0 where retail has r4/r0/r3); `gti2GetResponse`'s absolute value
 *   swaps its two temporaries.
 * SHAPES. The challenge check is a static inline returning 0/1 (retail expands it ahead of the response loop); the
 *   key index is `c * (i * 17991)` with `c` a `char` (retail strength-reduces `i * 17991` and sign-extends `c`);
 *   `gti2GetChallenge` clears `odd` after drawing the first byte.
 */
#include "DWCi/gt2Auth.h"
#include "DWCi/dwc_nasfunc.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "MSL_C/alloc.h"

/* The GT2 challenge secret `gti2GetResponse` mixes into every response. */
char DWCi_gt2SecretKey[] = "3b8dd8995f7c40a9a5c5b7dd5b481341";

#define GTI2_CHALLENGE_LEN 32
#define GTI2_RANDOM_CHAR() (u8)(rand() % 93 + 33)

/* 0x8050C950 (0x110): fills `buffer` with a fresh 32-byte challenge (each byte's parity is a function of the
 * ones before it). */
u8* gti2GetChallenge(u8* buffer) {
    int i;
    int odd;

    srand(current_time());
    buffer[0] = GTI2_RANDOM_CHAR();
    odd = 0;
    for (i = 1; i < GTI2_CHALLENGE_LEN; i++) {
        odd = (buffer[0] & 1) ^ odd ^ ((i ^ buffer[i - 1]) & 1) ^ (buffer[0] < 79) ^ (buffer[i - 1] < buffer[0]);
        buffer[i] = GTI2_RANDOM_CHAR();
        if ((odd && !(buffer[i] & 1)) || (!odd && (buffer[i] & 1) == 1)) {
            buffer[i]++;
        }
    }
    return buffer;
}

/* 0x8050CA60 (0x1F0): the response to `challenge` (random bytes when the challenge is not a valid one). */
/* Non-zero when `challenge` is a well-formed challenge (each byte's parity follows from the ones before it). */
static inline int gti2VerifyChallengeInline(const u8* challenge) {
    int odd;
    int i;

    odd = 0;
    for (i = 1; i < GTI2_CHALLENGE_LEN; i++) {
        odd = (challenge[0] & 1) ^ odd ^ ((i ^ challenge[i - 1]) & 1) ^ (challenge[0] < 79) ^ (challenge[i - 1] < challenge[0]);
        if ((odd && !(challenge[i] & 1)) || (!odd && (challenge[i] & 1) == 1)) {
            return 0;
        }
    }
    return 1;
}

u8* gti2GetResponse(u8* buffer, const u8* challenge) {
    int keyLength = strlen(DWCi_gt2SecretKey);
    int i;
    int valid;
    char c;
    int value;

    valid = gti2VerifyChallengeInline(challenge);
    for (i = 0; i < GTI2_CHALLENGE_LEN; i++) {
        if (!valid || i == 0 || i == 13) {
            buffer[i] = GTI2_RANDOM_CHAR();
        } else {
            if (i == 1 || i == 14) {
                c = challenge[i];
            } else {
                c = challenge[i - 1];
            }
            value = challenge[(DWCi_gt2SecretKey[(i + challenge[i]) % keyLength] + i * challenge[i]) % GTI2_CHALLENGE_LEN] ^
                    DWCi_gt2SecretKey[(c * (i * 17991)) % keyLength];
            if (value < 0) {
                value = -value;
            }
            buffer[i] = value % 93 + 33;
        }
    }
    return buffer;
}

/* 0x8050CC50 (0xC8): non-zero when the two responses agree (bytes 0 and 13 are random). */
int gti2CheckResponse(const u8* response1, const u8* response2) {
    int i;
    for (i = 0; i < GTI2_CHALLENGE_LEN; i++) {
        if (i != 0 && i != 13 && response1[i] != response2[i]) {
            return 0;
        }
    }
    return 1;
}
