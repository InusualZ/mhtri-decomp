/*
 * DWCi/dwc_base64.c - the DWC base64 codec: the URL-safe alphabet ('.' and '-' for 62 and 63, '*' as the pad) the
 *   NAS request fields are encoded with and the response tokens decoded with.
 *
 * RANGE. `.text` 0x80507050..0x805073C0 (2 functions); `.data` 0x8062FCB8..0x8062FD00 (the alphabet); `.sdata`
 *   0x807941F0..0x807941F8 (the alphabet pointer). The left edge is a `.data` vtable->string seam: RomFont's vtable
 *   (nw4r/fn_8050661C.cpp) is followed by the alphabet; the pointer is read only by the encoder and the callers are
 *   the DWCi auth units only.
 * FLAGS. the `DWCi` lib block with `cflags_base` (both functions start 16-byte aligned). The decoder is compiled
 *   under `#pragma opt_strength_reduction off`: retail indexes the source and the 4-byte block with `lbzx`/`stbx`
 *   off one counter (with the pass on, MWCC walks both with pointers, a `nop`-aligned loop and one more saved GPR).
 * NAMES. `DWCi_Base64Encode`/`DWCi_Base64Decode` are the map's. GUESS: `DWCi_base64Alphabet` (lbl_8062FCB8, the 65-byte
 *   string) and `DWCi_base64AlphabetPtr` (lbl_807941F0, the pointer the encoder indexes); the file name is the SDK's
 *   `dwc_base64.c` (no `__FILE__` string).
 * RESIDUALS. none recorded.
 * SHAPES. the decoder's character classes are one `switch` (retail's binary compare tree); the encoder re-reads the
 *   alphabet pointer for every output character, declares the chunk size first and picks it with a ternary (an
 *   `if` after `take = 3` swaps r30/r31 with the hoisted /3 constant).
 */

#include "DWCi/dwc_base64.h"
#include "DWCi/dwc_auth_interface.h"     /* DWCi_Np_CPUCopyFast */
#include "Runtime.PPCEABI.H/memset.h"    /* memset */

static char DWCi_base64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-";
static char* DWCi_base64AlphabetPtr = DWCi_base64Alphabet;

/* 0x80507050 (0x190): encodes three bytes at a time into four characters, padding a short tail with '*'. */
s32 DWCi_Base64Encode(const char* src, u32 srcLength, char* dst, u32 dstCapacity) {
    s32 take;
    s32 remain;
    u32 encodedLength;
    u32 chars;
    u32 bits;
    const char* cur;
    const char* end;
    u8 block[3];
    char* out;

    encodedLength = srcLength / 3 * 4 + ((srcLength % 3) ? 4 : 0);
    if (dst == NULL) {
        return encodedLength;
    }
    if (dstCapacity < encodedLength) {
        return -1;
    }

    cur = src;
    out = dst;
    end = src + srcLength;
    while (cur != end) {
        remain = end - cur;
        bits = remain * 8;
        chars = bits / 6 + ((bits % 6) != 0);
        take = (remain < 3) ? remain : 3;
        memset(block, 0, 3);
        DWCi_Np_CPUCopyFast(block, (const u8*)cur, take);

        out[0] = DWCi_base64AlphabetPtr[block[0] >> 2];
        if (chars >= 2) {
            out[1] = DWCi_base64AlphabetPtr[((block[0] & 3) << 4) | (block[1] >> 4)];
        } else {
            out[1] = '*';
        }
        if (chars >= 3) {
            out[2] = DWCi_base64AlphabetPtr[((block[1] & 0xF) << 2) | (block[2] >> 6)];
        } else {
            out[2] = '*';
        }
        if (chars >= 4) {
            out[3] = DWCi_base64AlphabetPtr[block[2] & 0x3F];
        } else {
            out[3] = '*';
        }
        cur += take;
        out += 4;
    }
    return out - dst;
}

#pragma opt_strength_reduction off
/* 0x805071E0 (0x1D8): decodes four characters at a time into three bytes, stopping at the length the non-pad
 * characters carry. */
s32 DWCi_Base64Decode(const char* src, u32 srcLength, char* dst, u32 dstCapacity) {
    s8 block[4];
    s32 bits;
    s32 i;
    s32 j;
    s32 written;
    char* out;

    if (srcLength & 3) {
        return -1;
    }

    bits = 0;
    for (i = 0; i < (s32)srcLength; i++) {
        if (src[i] != '*') {
            bits += 6;
        }
    }
    bits /= 8;

    if (dst == NULL) {
        return bits;
    }
    if (dstCapacity < bits) {
        return -1;
    }
    if (srcLength == 0) {
        return 0;
    }

    out = dst;
    do {
        for (j = 0; j < 4; j++) {
            switch (src[j]) {
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
            case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
            case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
                block[j] = src[j] - 'A';
                break;
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
            case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
            case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
                block[j] = src[j] - 'a' + 26;
                break;
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8':
            case '9':
                block[j] = src[j] - '0' + 52;
                break;
            case '.':
                block[j] = 62;
                break;
            case '-':
                block[j] = 63;
                break;
            default:
                block[j] = 0;
                break;
            }
        }
        src += 4;
        out[0] = (block[0] << 2) | (block[1] >> 4);
        written = out + 1 - dst;
        if (written >= bits) {
            break;
        }
        out[1] = (block[1] << 4) | (block[2] >> 2);
        written = out + 2 - dst;
        if (written >= bits) {
            break;
        }
        out[2] = (block[2] << 6) | block[3];
        out += 3;
        written = out - dst;
    } while (written < bits);
    return written;
}

#pragma opt_strength_reduction reset
