/*
 * ENC/enc.c - the SDK encoding library (UTF-8 / UTF-16 conversion).
 *
 * RANGE. `.text` 0x804AE880-0x804AF420; `.data` 0x8061A4A8-0x8061A4F0; `.sdata` 0x80793E18-0x80793E20; `.sbss`
 *   0x80795180-0x80795188 (9 functions / 0xB50 B).
 *   - version string `<< RVL_SDK - ENC ... >>` at `.data` 0x8061A4A8 is registered by 0x804AE880 through `.sdata`
 *     0x80793E18; the run ends where the ESP library's `/dev/es` string starts
 *   - `ENCConvertStringUtf16ToUtf8` / `Utf8ToUtf16` are two-instruction wrappers over the converters; `.sbss`
 *     0x80795180 is the version-registered flag
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, -inline auto).
 * NAMES. all GUESSES: the two public wrappers `ENCConvertString*` (the map's rows, no dump or SDK header cited), the
 *   internal functions, `__ENCVersion` and `encVersionRegistered` in the SDK's `ENCi` scheme from what each one does,
 *   and the `ENC_ERR_*` values from where each result code is returned.
 * RESIDUALS.
 *   - `ENCiConvertUtf16ToUtf8`: register colouring only - retail keeps srcCount/dstCount/result/char+srcUnits/
 *     dstUnits in r31/r30/r29/r28/r27, ours gives srcCount, the char+srcUnits web and dstUnits other callee-saved
 *     registers; no declaration order of the locals reproduces it
 *   - `ENCiConvertUtf8ToUtf16`: register colouring only - retail puts srcUnits in the char's r31 and dstUnits in the
 *     line-break length's r27, ours the mirror; no declaration order, block scope or variable reuse moves it
 *   - `.sdata` `__ENCVersion` relocates against the compiler's anonymous string (`@36`) where the map names the
 *     `.data` string `lbl_8061A4A8`; naming only; flipcheck does not refuse it
 * SHAPES. the converters call the line-break helper 0x804AE980 out of line, which a one-TU `-inline auto` compile
 *   inlines; `#pragma dont_inline on` around its definition keeps the call. A seam between a conversion-utility TU and
 *   the converters (encutility/encunicode in the SDK's later layout) would explain the same call and is unpinned: no
 *   pool, string or unwind record separates the two. Assigning dstUnits before srcUnits in the unit-count branches
 *   reproduces retail's instruction order; `ENCiCheckUtf8Sequence` reads each trailing byte into a local and shifts
 *   the code point in place (`cp <<= 6`), which keeps retail's r4/r5 and its `cmplwi`.
 *   The UTF-16 surrogate join masks the low surrogate with retail's 0x3EF (`andi. r0,r0,1007`), not 0x3FF.
 */

#include "types.h"
#include "ENC/enc.h"
#include "OS/OS.h"
#include "Runtime.PPCEABI.H/memset.h"

#define ENC_OK 0
#define ENC_ERR_NO_BUF_LEFT -1
#define ENC_ERR_INVALID_PARAM -3
#define ENC_ERR_INVALID_FORMAT -4

s32 ENCiConvertUtf16ToUtf8(u8* dst, s32* dstLength, const u16* src, s32* srcLength, s32 lineBreak);
s32 ENCiConvertUtf8ToUtf16(u16* dst, s32* dstLength, const u8* src, s32* srcLength, s32 lineBreak);
u32 ENCiCheckUtf8Sequence(const u8* src, s32 length);
void ENCiEncodeUtf8(u8* dst, s32 length, u32 cp);

const char* __ENCVersion = "<< RVL_SDK - ENC \trelease build: Feb 27 2009 10:05:42 (0x4302_145) >>";
static BOOL encVersionRegistered;

/* 0x804AE880 (0xF8): Registers the library version once, then turns the in/out lengths into limits and reports a
 * missing source or length. */
s32 ENCiCheckParameters(BOOL hasDst, s32* dstLength, s32* dstLimit, BOOL* writeDst, BOOL hasSrc, s32* srcLength,
                        s32* srcLimit, BOOL* srcLimited)
{
    s32 result = ENC_OK;

    if (!encVersionRegistered) {
        OSRegisterVersion(__ENCVersion);
        encVersionRegistered = TRUE;
    }

    if (srcLength != NULL) {
        *srcLimit = *srcLength;
    } else {
        *srcLimit = -1;
    }

    if (dstLength != NULL) {
        *dstLimit = *dstLength;
    } else {
        *dstLimit = -1;
        result = ENC_ERR_INVALID_PARAM;
    }

    if (!hasSrc) {
        result = ENC_ERR_INVALID_PARAM;
    }

    if (!hasDst) {
        *writeDst = FALSE;
        *dstLimit = -1;
    }

    if (*srcLimit < 0) {
        *srcLimited = FALSE;
    }

    if (result != ENC_OK) {
        *dstLength = 0;
        *srcLength = 0;
    }

    return result;
}

#pragma dont_inline on
/* 0x804AE980 (0x34): Returns how many source units a line break at `c` spans (CR LF counts two). */
s32 ENCiGetLineBreakLength(u32 c, u32 next)
{
    if (c == 10) {
        return 1;
    }
    if (c == 13) {
        return (next == 10) + 1;
    }
    return 0;
}
#pragma dont_inline reset

/* 0x804AE9C0 (0x100): Writes the line break `type` selects in `unitSize`-byte units and returns the units used. */
s32 ENCiWriteLineBreak(u8* dst, s32 unitSize, s32 type, BOOL write)
{
    if (write) {
        memset(dst, 0, unitSize - 1);
        switch (type) {
        case 1:
            dst[unitSize - 1] = 13;
            memset(dst + unitSize, 0, unitSize - 1);
            dst[unitSize * 2 - 1] = 10;
            return 2;
        case 2:
            dst[unitSize - 1] = 13;
            return 1;
        case 3:
            dst[unitSize - 1] = 10;
            return 1;
        }
    } else {
        switch (type) {
        case 1:
            return 2;
        case 2:
            return 1;
        case 3:
            return 1;
        }
    }
    return 0;
}

/* 0x804AEAC0 (0x8): Converts a UTF-16 string to UTF-8, keeping its line breaks. */
s32 ENCConvertStringUtf16ToUtf8(u8* dst, s32* dstLength, const u16* src, s32* srcLength)
{
    return ENCiConvertUtf16ToUtf8(dst, dstLength, src, srcLength, 0);
}

/* 0x804AEAD0 (0x8): Converts a UTF-8 string to UTF-16, keeping its line breaks. */
s32 ENCConvertStringUtf8ToUtf16(u16* dst, s32* dstLength, const u8* src, s32* srcLength)
{
    return ENCiConvertUtf8ToUtf16(dst, dstLength, src, srcLength, 0);
}

/* 0x804AEAE0 (0x304): Converts UTF-16 to UTF-8, honouring a byte-order mark and rewriting line breaks. */
s32 ENCiConvertUtf16ToUtf8(u8* dst, s32* dstLength, const u16* src, s32* srcLength, s32 lineBreak)
{
    s32 srcLimit = -1;
    s32 dstLimit = -1;
    BOOL writeDst = TRUE;
    BOOL srcLimited = TRUE;
    s32 dstUnits;
    s32 dstCount = 0;
    s32 result;
    s32 srcCount = 0;
    u32 c;
    s32 breakLength;
    s32 srcUnits;
    u32 cp;

    result = ENCiCheckParameters(dst != NULL, dstLength, &dstLimit, &writeDst, src != NULL, srcLength, &srcLimit,
                                 &srcLimited);
    if (result != ENC_OK) {
        return result;
    }

    if (srcLimit > 0 || !srcLimited) {
        if (*src == 0xFEFF) {
            srcCount = 1;
            src++;
        } else if (*src == 0xFFFE) {
            if (dstLength != NULL) {
                *dstLength = 0;
            }
            *srcLength = 0;
            return ENC_ERR_INVALID_FORMAT;
        }
    }

    while ((c = *src) != 0 && (srcCount < srcLimit || !srcLimited)) {
        if (dstCount >= dstLimit && writeDst) {
            result = ENC_ERR_NO_BUF_LEFT;
            break;
        }

        if (lineBreak > 0) {
            breakLength = ENCiGetLineBreakLength(c, (srcLimit - srcCount > 1 || !srcLimited) ? src[1] : 0);
            if (breakLength > 0) {
                dstUnits = ENCiWriteLineBreak(dst, 1, lineBreak, writeDst);
                if (dstLimit - dstCount < dstUnits && writeDst) {
                    result = ENC_ERR_NO_BUF_LEFT;
                    break;
                }
                src += breakLength;
                srcCount += breakLength;
                dstCount += dstUnits;
                if (writeDst) {
                    dst += dstUnits;
                }
                continue;
            }
        }

        if (c < 0x80) {
            if (writeDst) {
                *dst++ = c;
            }
            dstCount++;
            src++;
            srcCount++;
            continue;
        }

        if (c < 0x800) {
            dstUnits = 2;
            srcUnits = 1;
        } else if ((c & 0xF800) != 0xD800) {
            dstUnits = 3;
            srcUnits = 1;
        } else {
            dstUnits = 4;
            srcUnits = 2;
        }

        if (srcLimit - srcCount < srcUnits && srcLimited) {
            break;
        }

        if (writeDst) {
            if (dstLimit - dstCount < dstUnits) {
                result = ENC_ERR_NO_BUF_LEFT;
                break;
            }
            if (srcUnits == 1) {
                cp = src[0];
            } else if (srcUnits == 2) {
                cp = ((src[0] - 0xD7C0) << 10) + (src[1] & 0x3EF);
            } else {
                cp = 0;
            }
            ENCiEncodeUtf8(dst, dstUnits, cp);
            dst += dstUnits;
        }
        src += srcUnits;
        srcCount += srcUnits;
        dstCount += dstUnits;
    }

    if (srcLength != NULL) {
        *srcLength = srcCount;
    }
    if (dstLength != NULL) {
        *dstLength = dstCount;
    }
    return result;
}

/* 0x804AEDF0 (0x314): Converts UTF-8 to UTF-16, skipping a byte-order mark, validating each sequence and rewriting
 * line breaks. */
s32 ENCiConvertUtf8ToUtf16(u16* dst, s32* dstLength, const u8* src, s32* srcLength, s32 lineBreak)
{
    s32 srcLimit = -1;
    s32 dstLimit = -1;
    BOOL writeDst = TRUE;
    BOOL srcLimited = TRUE;
    s32 srcCount = 0;
    s32 dstCount = 0;
    s32 result;
    u32 c;
    s32 dstUnits;
    s32 breakLength;
    s32 srcUnits;
    u32 cp;

    result = ENCiCheckParameters(dst != NULL, dstLength, &dstLimit, &writeDst, src != NULL, srcLength, &srcLimit,
                                 &srcLimited);
    if (result != ENC_OK) {
        return result;
    }

    if (srcLimit >= 3 || !srcLimited) {
        if (src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
            srcCount = 3;
            src += 3;
        }
    }

    while ((c = *src) != 0 && (srcCount < srcLimit || !srcLimited)) {
        if (dstCount >= dstLimit && writeDst) {
            result = ENC_ERR_NO_BUF_LEFT;
            break;
        }

        if (lineBreak > 0) {
            breakLength = ENCiGetLineBreakLength(c, (srcLimit - srcCount > 1 || !srcLimited) ? src[1] : 0);
            if (breakLength > 0) {
                dstUnits = ENCiWriteLineBreak((u8*)dst, 2, lineBreak, writeDst);
                if (dstLimit - dstCount < dstUnits && writeDst) {
                    result = ENC_ERR_NO_BUF_LEFT;
                    break;
                }
                src += breakLength;
                srcCount += breakLength;
                dstCount += dstUnits;
                if (writeDst) {
                    dst += dstUnits;
                }
                continue;
            }
        }

        if (c < 0x80) {
            if (writeDst) {
                *dst++ = c;
            }
            dstCount++;
            src++;
            srcCount++;
            continue;
        }

        if ((c & 0xE0) == 0xC0) {
            dstUnits = 1;
            srcUnits = 2;
        } else if ((c & 0xF0) == 0xE0) {
            dstUnits = 1;
            srcUnits = 3;
        } else if ((c & 0xF8) == 0xF0) {
            dstUnits = 2;
            srcUnits = 4;
        } else {
            result = ENC_ERR_INVALID_FORMAT;
            break;
        }

        if (srcLimit - srcCount < srcUnits && srcLimited) {
            break;
        }

        cp = ENCiCheckUtf8Sequence(src, srcUnits);
        if (cp == 0) {
            result = ENC_ERR_INVALID_FORMAT;
            break;
        }

        if (writeDst) {
            if (dstLimit - dstCount < dstUnits) {
                result = ENC_ERR_NO_BUF_LEFT;
                break;
            }
            if (dstUnits == 1) {
                dst[0] = cp;
            }
            if (dstUnits == 2) {
                dst[0] = (cp >> 10) + 0xD7C0;
                dst[1] = (cp & 0x3FF) + 0xDC00;
            }
            dst += dstUnits;
        }
        dstCount += dstUnits;
        src += srcUnits;
        srcCount += srcUnits;
    }

    if (srcLength != NULL) {
        *srcLength = srcCount;
    }
    if (dstLength != NULL) {
        *dstLength = dstCount;
    }
    return result;
}

/* 0x804AF110 (0xA4): Decodes one `length`-byte UTF-8 sequence and returns its code point, or 0 when it is malformed. */
u32 ENCiCheckUtf8Sequence(const u8* src, s32 length)
{
    u32 cp;
    s32 i;
    u32 c;

    switch (length) {
    case 1:
        cp = src[0];
        break;
    case 2:
        cp = src[0] & 0x1F;
        break;
    case 3:
        cp = src[0] & 0x0F;
        break;
    case 4:
        cp = src[0] & 0x07;
        break;
    default:
        return 0;
    }

    for (i = 1; i < length; i++) {
        c = src[i];
        if ((c & 0xC0) != 0x80) {
            return 0;
        }
        cp <<= 6;
        cp += c & 0x3F;
    }
    return cp;
}

/* 0x804AF1C0 (0x258): Encodes code point `cp` as a `length`-byte UTF-8 sequence. */
void ENCiEncodeUtf8(u8* dst, s32 length, u32 cp)
{
    s32 i;

    switch (length) {
    case 1:
        dst[0] = cp;
        break;
    case 2:
        dst[0] = (cp >> 6) + 0xC0;
        break;
    case 3:
        dst[0] = (cp >> 12) + 0xE0;
        break;
    case 4:
        dst[0] = (cp >> 18) + 0xF0;
        break;
    default:
        return;
    }

    for (i = 1; i < length; i++) {
        dst[i] = ((cp >> ((length - i - 1) * 6)) & 0x3F) + 0x80;
    }
}
