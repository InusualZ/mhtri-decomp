/*
 * THP/THPDec.c - the THP movie decoder: the Huffman and IDCT decode of a frame and `THPInit`.
 *
 * RANGE. .text 0x804E1120-0x804E45B0 (16 functions, 0x3490 B); .rodata 0x80573BB0-0x80573C40; .data
 *    0x8062AB20-0x8062AB68; .bss 0x8075AFC0-0x8075B110; .sdata 0x80794140-0x80794148; .sbss
 *    0x807954A0-0x807955C8; .sdata2 0x8079D3A0-0x8079D3C0.  Cut from the old SC block at 0x804E1120; the right
 *    edge is `TPLBind` (0x804E45B0, `TPL/tpl.cpp`).  COARSE: the library's source files are not separated.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the library name `THP` is known (the build string); the file name `THPDec.c` and every function, table and
 *    global name are GUESSES read off the bodies (a baseline JPEG decoder: marker parser, Huffman lookup tables,
 *    inverse DCT); `THPVideoDecode` and `THPInit` are named for their callers in `menu/movie.cpp`.
 * EVIDENCE. `.data` 0x8062AB20 is the build string `<< RVL_SDK - THP release build ... >>` read through `.sdata`
 *    0x80794140 by `THPInit`; `.rodata` 0x80573BB0 (0x50 B) is the zig-zag order and 0x80573C00 (0x40 B) the eight
 *    AAN scale factors read as doubles; `.sdata2` 0x8079D3A8..0x8079D3BC are the inverse-DCT constants of the
 *    paired-single routines; the decoder uses the locked cache (`LCStoreData`, `LCQueueWait`, `PPCMfhid2`).
 *    `.bss` 0x8075AFC0 .. 0x8075B110 and `.sbss` 0x807954A0..0x807955C8 are read here only.
 * RESIDUALS. `THPBuildLookupTables` 0x804E1DC0 (62 %): register numbering of the nested lookup loops (the target keeps the
 *    prefix in r30 and the length in r31); `THPVideoDecode` 0x804E1120 (77 %): the marker switch is compiled with signed
 *    compares and a different tree where the target uses unsigned compares; `THPParseHuffmanTable` 0x804E19F0 (82 %),
 *    `THPDecodeFrameAnySize` 0x804E2EF0 (90 %), `THPDecodeFrame` 0x804E2010 (94 %): register numbering, and the GQR
 *    reads/writes (the target interleaves the constant set-up with the saves); `THPDecodeFrame512x448`,
 *    `THPParseFrameHeader`, `THPParseQuantTable`, `THPDecodeFrame640x480` (96-99 %): register numbering only.
 *    flipcheck: .text 0x346C vs 0x3490.
 */

#include "types.h"

#include "OS/LCEnable.h"
#include "OS/OS.h"
#include "OS/OSCache.h"
#include "OS/PPCArch.h"
#include "THP/THPDec.h"

#define THP_READ_U16(p) (((p)[0] << 8) | (p)[1])

const u8 THPZigzagTable[80] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
    63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
};

const f64 THPAanScale[8] = {
    1.0, 1.3870398998260498, 1.3065630197525024, 1.1758755445480347,
    1.0, 0.78569495677948, 0.5411961078643799, 0.27589938044548035,
};

const char* THPVersion = "<< RVL_SDK - THP 	release build: Feb 27 2009 09:52:24 (0x4302_145) >>";

s16* THPBlocks[6];
u32 THPPlane512[3];
f32 THPIdctBuffer[64] __attribute__((aligned(32)));
u32 THPPlane640[3];

THPHuffTable* THPDcTable0 __attribute__((aligned(32)));
THPHuffTable* THPDcTable1 __attribute__((aligned(32)));
THPHuffTable* THPDcTable2 __attribute__((aligned(32)));
THPHuffTable* THPAcTable0 __attribute__((aligned(32)));
THPHuffTable* THPAcTable1 __attribute__((aligned(32)));
THPHuffTable* THPAcTable2 __attribute__((aligned(32)));
u8* THPHuffCounts;
u8* THPHuffSizes;
u8* THPHuffCodes;
u8* THPOutputPlane __attribute__((aligned(32)));
u32 THPOutputStride __attribute__((aligned(32)));
f32* THPQuantTable __attribute__((aligned(32)));
u32 THPSavedGqr5;
u32 THPSavedGqr6;
u8* THPWorkEnd;
THPDecoder* THPState;
s32 THPInitialized;

u8 THPParseFrameHeader(void);
u8 THPParseScanHeader(void);
u8 THPParseQuantTable(void);
u8 THPParseHuffmanTable(void);
void THPDecodeFrame(u8* planeY, u8* planeU, u8* planeV);
void THPBuildLookupTables(void);
void THPDecodeFrame512x448(void);
void THPDecodeFrame640x480(void);
void THPDecodeFrameAnySize(void);
void THPIdctUpper(s16* block, u32 x);
void THPIdctLower(s16* block, u32 x);
void THPDecodeBlockY(THPDecoder* state, s16* block);
void THPDecodeBlockU(THPDecoder* state, s16* block);
void THPDecodeBlockV(THPDecoder* state, s16* block);

u8 THPVideoDecode(u8* data, u8* planeY, u8* planeU, u8* planeV, u8* work)
{
    u8 result;
    u32 marker;
    s32 done;
    s16* base;

    if (data != NULL) {
        if (planeY != NULL && planeU != NULL && planeV != NULL) {
            if (work != NULL) {
                if ((PPCMfhid2() & 0x10000000) != 0) {
                    if (THPInitialized != 0) {
                        THPState = (THPDecoder*)(((u32)work + 31) & ~31);
                        THPWorkEnd = (u8*)THPState + sizeof(THPDecoder);
                        DCZeroRange((u8*)THPState, sizeof(THPDecoder));
                        done = 0;
                        THPState->bitPosition = 0x21;
                        THPState->rowY = 0;
                        THPState->cursor = data;
                        do {
                            if (*THPState->cursor++ == 0xFF) {
                                while (*THPState->cursor == 0xFF) {
                                    THPState->cursor++;
                                }
                                marker = *THPState->cursor++;
                                switch (marker) {
                                case 0xC0:
                                    result = THPParseFrameHeader();
                                    if (result != 0) {
                                        return result;
                                    }
                                    break;
                                case 0xC4:
                                    result = THPParseHuffmanTable();
                                    if (result != 0) {
                                        return result;
                                    }
                                    break;
                                case 0xD8:
                                    break;
                                case 0xDA:
                                    result = THPParseScanHeader();
                                    if (result != 0) {
                                        return result;
                                    }
                                    done = 1;
                                    break;
                                case 0xDB:
                                    result = THPParseQuantTable();
                                    if (result != 0) {
                                        return result;
                                    }
                                    break;
                                case 0xDD:
                                    THPState->restartEnabled = 1;
                                    THPState->cursor += 2;
                                    THPState->restartInterval = THP_READ_U16(THPState->cursor);
                                    THPState->cursor += 2;
                                    THPState->restartCounter = THPState->restartInterval;
                                    break;
                                case 0xE0:
                                case 0xE1:
                                case 0xE2:
                                case 0xE3:
                                case 0xE4:
                                case 0xE5:
                                case 0xE6:
                                case 0xE7:
                                case 0xE8:
                                case 0xE9:
                                case 0xEA:
                                case 0xEB:
                                case 0xEC:
                                case 0xED:
                                case 0xEE:
                                case 0xEF:
                                case 0xFE:
                                    THPState->cursor += THP_READ_U16(THPState->cursor);
                                    break;
                                default:
                                    return 0xB;
                                }
                            } else {
                                return 3;
                            }
                        } while (done == 0);
                        base = (s16*)(((u32)THPWorkEnd + 31) & ~31);
                        THPBlocks[0] = base;
                        THPBlocks[1] = base + 64;
                        THPBlocks[2] = base + 128;
                        THPBlocks[3] = base + 192;
                        THPBlocks[4] = base + 256;
                        THPBlocks[5] = base + 320;
                        THPDecodeFrame(planeY, planeU, planeV);
                        return 0;
                    }
                    return 0x1D;
                }
                return 0x1C;
            }
            return 0x1A;
        }
        return 0x1B;
    }
    return 0x19;
}

u8 THPParseFrameHeader(void)
{
    u8 index;
    u8 sampling;
    s32 count;

    THPState->cursor += 2;
    if (*THPState->cursor++ != 8) {
        return 0xA;
    }
    THPState->height = THP_READ_U16(THPState->cursor);
    THPState->cursor += 2;
    THPState->width = THP_READ_U16(THPState->cursor);
    THPState->cursor += 2;
    if (*THPState->cursor++ != 3) {
        return 0xC;
    }
    index = 0;
    for (count = 3; count != 0; count--) {
        THPState->cursor += 1;
        sampling = *THPState->cursor++;
        if ((index == 0 && sampling != 0x22) || (index != 0 && sampling != 0x11)) {
            return 0x13;
        }
        THPState->component[index].quantTable = *THPState->cursor;
        index++;
        THPState->cursor++;
    }
    return 0;
}

u8 THPParseScanHeader(void)
{
    u8 index;
    u8 selector;
    s32 count;

    THPState->cursor += 2;
    if (*THPState->cursor++ != 3) {
        return 0xC;
    }
    index = 0;
    for (count = 3; count != 0; count--) {
        THPState->cursor += 1;
        selector = *THPState->cursor++;
        THPState->component[index].dcTable = selector >> 4;
        THPState->component[index].acTable = selector & 0xF;
        if ((THPState->tableMask & (1 << (selector >> 4))) == 0) {
            return 0xF;
        }
        if ((THPState->tableMask & (1 << ((selector & 0xF) + 1))) == 0) {
            return 0xF;
        }
        index++;
    }
    THPState->cursor += 3;
    THPState->mcuColumns = (THPState->width + 15) / 16;
    THPState->component[0].dcPredictor = 0;
    THPState->component[1].dcPredictor = 0;
    THPState->component[2].dcPredictor = 0;
    return 0;
}

u8 THPParseQuantTable(void)
{
    f32 raw[64];
    u16 remaining;
    u16 i;
    u16 j;
    u16 row;
    u16 table;
    s32 count;
    f64 rowScale;

    remaining = THP_READ_U16(THPState->cursor) - 2;
    THPState->cursor += 2;
    do {
        j = 0;
        table = *THPState->cursor++;
        for (count = 8; count != 0; count--) {
            raw[THPZigzagTable[j + 0]] = (f32)*THPState->cursor++;
            raw[THPZigzagTable[j + 1]] = (f32)*THPState->cursor++;
            raw[THPZigzagTable[j + 2]] = (f32)*THPState->cursor++;
            raw[THPZigzagTable[j + 3]] = (f32)*THPState->cursor++;
            raw[THPZigzagTable[j + 4]] = (f32)*THPState->cursor++;
            raw[THPZigzagTable[j + 5]] = (f32)*THPState->cursor++;
            raw[THPZigzagTable[j + 6]] = (f32)*THPState->cursor++;
            raw[THPZigzagTable[j + 7]] = (f32)*THPState->cursor++;
            j += 8;
        }
        i = 0;
        for (row = 0; row < 8; row++) {
            rowScale = THPAanScale[row];
            THPState->quant[table][i] = THPAanScale[0] * ((f64)raw[i] * rowScale);
            i++;
            THPState->quant[table][i] = THPAanScale[1] * ((f64)raw[i] * rowScale);
            i++;
            THPState->quant[table][i] = THPAanScale[2] * ((f64)raw[i] * rowScale);
            i++;
            THPState->quant[table][i] = THPAanScale[3] * ((f64)raw[i] * rowScale);
            i++;
            THPState->quant[table][i] = THPAanScale[4] * ((f64)raw[i] * rowScale);
            i++;
            THPState->quant[table][i] = THPAanScale[5] * ((f64)raw[i] * rowScale);
            i++;
            THPState->quant[table][i] = THPAanScale[6] * ((f64)raw[i] * rowScale);
            i++;
            THPState->quant[table][i] = THPAanScale[7] * ((f64)raw[i] * rowScale);
            i++;
        }
        remaining -= 0x41;
    } while (remaining != 0);
    return 0;
}

u8 THPParseHuffmanTable(void)
{
    u16 remaining;
    u8 selector;
    u8 table;
    u16 total;
    u8 length;
    u8 count;
    s32 k;
    s32 codeIndex;
    u8 sizeValue;
    u16 code;
    s32 j;
    s32 l;
    THPHuffTable* huffman;
    s32 i;

    THPHuffSizes = THPWorkEnd;
    THPHuffCodes = THPWorkEnd + 0x101;
    remaining = THP_READ_U16(THPState->cursor) - 2;
    THPState->cursor += 2;
    do {
        selector = *THPState->cursor++;
        table = ((selector * 2) & 0x1E) + (selector >> 4);
        THPHuffCounts = THPState->cursor;
        total = 0;
        for (i = 0; i < 16; i++) {
            total += *THPState->cursor++;
        }
        huffman = &THPState->huffman[table];
        huffman->symbols = THPState->cursor;
        THPState->cursor += total;
        k = 0;
        for (length = 1; length <= 16; length++) {
            count = THPHuffCounts[length - 1];
            for (; count != 0; count--) {
                THPHuffSizes[k++] = length;
            }
        }
        THPHuffSizes[k] = 0;
        codeIndex = 0;
        code = 0;
        sizeValue = THPHuffSizes[0];
        while (THPHuffSizes[codeIndex] != 0) {
            while (sizeValue == THPHuffSizes[codeIndex]) {
                ((u16*)THPHuffCodes)[codeIndex] = code;
                codeIndex++;
                code++;
            }
            code = (code * 2) & 0xFFFE;
            sizeValue++;
        }
        j = 0;
        l = 1;
        for (i = 0; i < 8; i++) {
            if (THPHuffCounts[l - 1] != 0) {
                huffman->valueOffset[l] = j - ((u16*)THPHuffCodes)[j];
                j += THPHuffCounts[l - 1];
                huffman->maxCode[l] = ((u16*)THPHuffCodes)[j - 1];
            } else {
                huffman->maxCode[l] = -1;
                huffman->valueOffset[l] = -1;
            }
            if (THPHuffCounts[l] != 0) {
                huffman->valueOffset[l + 1] = j - ((u16*)THPHuffCodes)[j];
                j += THPHuffCounts[l];
                huffman->maxCode[l + 1] = ((u16*)THPHuffCodes)[j - 1];
            } else {
                huffman->maxCode[l + 1] = -1;
                huffman->valueOffset[l + 1] = -1;
            }
            l += 2;
        }
        huffman->maxCode[17] = 0xFFFFF;
        remaining -= total + 0x11;
        THPState->tableMask |= 1 << table;
    } while (remaining != 0);
    return 0;
}

void THPBuildLookupTables(void)
{
    u32 bitPosition = THPState->bitPosition;
    u32 address = (u32)THPState->cursor;
    u32 aligned = address & ~3;
    u32 offset = address & 3;
    u32 table;
    u32 code;
    u32 length;

    if (bitPosition != 0x21) {
        THPState->bitPosition = bitPosition - (3 - offset) * 8;
    } else {
        THPState->bitPosition = offset * 8 + 1;
    }
    THPState->cursor = (u8*)aligned;
    THPState->bitBuffer = *(u32*)aligned;
    for (table = 0; table < 4; table++) {
        if (THPState->tableMask & (1 << table)) {
            for (code = 0; code < 32; code++) {
                THPState->huffman[table].fastSymbol[code] = 0xFF;
                for (length = 0; length < 5; length++) {
                    s32 prefix = code >> (4 - length);

                    if (prefix <= THPState->huffman[table].maxCode[length + 1]) {
                        THPState->huffman[table].fastSymbol[code] =
                            *(THPState->huffman[table].symbols + prefix + THPState->huffman[table].valueOffset[length + 1]);
                        THPState->huffman[table].fastLength[code] = length + 1;
                        length = 99;
                    }
                }
            }
        }
    }
    THPDcTable0 = &THPState->huffman[THPState->component[0].dcTable * 2];
    THPDcTable1 = &THPState->huffman[THPState->component[1].dcTable * 2];
    THPDcTable2 = &THPState->huffman[THPState->component[2].dcTable * 2];
    THPAcTable0 = &THPState->huffman[THPState->component[0].acTable * 2 + 1];
    THPAcTable1 = &THPState->huffman[THPState->component[1].acTable * 2 + 1];
    THPAcTable2 = &THPState->huffman[THPState->component[2].acTable * 2 + 1];
}

/* The inverse-DCT constants the paired-single routines load through the small-data base. */
const f32 THPIdctSqrt2 = 1.4142135f;
const f32 THPIdctCos2 = 1.847759f;
const f32 THPIdctCos3 = 1.0823922f;
const f32 THPIdctNegCos4 = -2.613126f;
const f32 THPIdctRound = 1024.0f;

void THPDecodeFrame(u8* planeY, u8* planeU, u8* planeV)
{
    u16 width;
    u16 row;
    u16 height;
    register u32 savedGqr5;
    register u32 savedGqr6;

    THPState->outputY = planeY;
    THPState->outputU = planeU;
    THPState->outputV = planeV;
    row = THPState->rowY;
    height = THPState->height;
    asm {
        mfspr savedGqr5, GQR5
        mfspr savedGqr6, GQR6
    }
    THPSavedGqr5 = savedGqr5;
    THPSavedGqr6 = savedGqr6;
    asm {
        li savedGqr5, 7
        oris savedGqr5, savedGqr5, 7
        mtspr GQR5, savedGqr5
        li savedGqr5, 15620
        oris savedGqr5, savedGqr5, 15620
        mtspr GQR6, savedGqr5
    }
    THPBuildLookupTables();
    width = THPState->width;
    if (width == 512 && height == 448) {
        while (row < height) {
            THPDecodeFrame512x448();
            row += 16;
        }
    } else if (width == 640 && height == 480) {
        while (row < height) {
            THPDecodeFrame640x480();
            row += 16;
        }
    } else {
        while (row < height) {
            THPDecodeFrameAnySize();
            row += 16;
        }
    }
    savedGqr5 = THPSavedGqr5;
    savedGqr6 = THPSavedGqr6;
    asm {
        mtspr GQR5, savedGqr5
        mtspr GQR6, savedGqr6
    }
}

void THPDecodeFrame512x448(void)
{
    u8 mcu;
    u32 x;

    LCQueueWait(3);
    for (mcu = 0; mcu < THPState->mcuColumns; mcu++) {
        THPDecodeBlockY(THPState, THPBlocks[0]);
        THPDecodeBlockY(THPState, THPBlocks[1]);
        THPDecodeBlockY(THPState, THPBlocks[2]);
        THPDecodeBlockY(THPState, THPBlocks[3]);
        THPDecodeBlockU(THPState, THPBlocks[4]);
        THPDecodeBlockV(THPState, THPBlocks[5]);
        x = (mcu * 16) & 0xFF0;
        THPOutputPlane = (u8*)THPPlane512[0];
        THPOutputStride = 512;
        THPQuantTable = THPState->quant[THPState->component[0].quantTable];
        THPIdctUpper(THPBlocks[0], x);
        THPIdctUpper(THPBlocks[1], x + 8);
        THPIdctLower(THPBlocks[2], x);
        THPIdctLower(THPBlocks[3], x + 8);
        x >>= 1;
        THPOutputPlane = (u8*)THPPlane512[1];
        THPOutputStride = 256;
        THPQuantTable = THPState->quant[THPState->component[1].quantTable];
        THPIdctUpper(THPBlocks[4], x);
        THPOutputPlane = (u8*)THPPlane512[2];
        THPQuantTable = THPState->quant[THPState->component[2].quantTable];
        THPIdctUpper(THPBlocks[5], x);
        if (THPState->restartEnabled != 0) {
            THPState->restartCounter--;
            if (THPState->restartCounter == 0) {
                THPState->restartCounter = THPState->restartInterval;
                THPState->bitPosition = ((THPState->bitPosition + 6) & ~7) + 1;
                if (THPState->bitPosition > 0x21) {
                    THPState->bitPosition = 0x21;
                }
                THPState->component[0].dcPredictor = 0;
                THPState->component[1].dcPredictor = 0;
                THPState->component[2].dcPredictor = 0;
            }
        }
    }
    LCStoreData(THPState->outputY, (u8*)THPPlane512[0], 0x2000);
    LCStoreData(THPState->outputU, (u8*)THPPlane512[1], 0x800);
    LCStoreData(THPState->outputV, (u8*)THPPlane512[2], 0x800);
    THPState->outputY += 0x2000;
    THPState->outputU += 0x800;
    THPState->outputV += 0x800;
}

/* Runs the 8x8 inverse DCT of a block and stores the pixels at the upper row of the output plane (paired singles). */
asm void THPIdctUpper(s16* block, u32 x)
{
    nofralloc
    stwu r1, -128(r1)
    stfd f31, 112(r1)
    psq_st f31, 120(r1), 0, 0
    stfd f30, 96(r1)
    psq_st f30, 104(r1), 0, 0
    stfd f29, 80(r1)
    psq_st f29, 88(r1), 0, 0
    stfd f28, 64(r1)
    psq_st f28, 72(r1), 0, 0
    stfd f27, 48(r1)
    psq_st f27, 56(r1), 0, 0
    stfd f26, 32(r1)
    psq_st f26, 40(r1), 0, 0
    stfd f25, 16(r1)
    psq_st f25, 24(r1), 0, 0
    lis r5, THPIdctBuffer@ha
    li r7, 8
    addi r5, r5, THPIdctBuffer@l
    lfs f29, THPIdctSqrt2(r0)
    addi r10, r5, -8
    lfs f28, THPIdctCos2(r0)
    lfs f27, THPIdctCos3(r0)
    lfs f26, THPIdctNegCos4(r0)
    lfs f25, THPIdctRound(r0)
    lwz r5, THPQuantTable(r0)
    mtctr r7
L_12b8:
    psq_l f10, 0(r3), 0, 5
    psq_l f11, 0(r5), 0, 0
    lwz r0, 12(r3)
    lwz r8, 8(r3)
    ps_mul f10, f10, f11
    lwz r6, 4(r3)
    or. r0, r0, r8
    lhz r7, 2(r3)
L_12d8:
    cmpwi r0, 0
    bne L_1418
    ps_merge00 f0, f10, f10
    cmpwi r6, 0
    psq_st f0, 8(r10), 0, 0
    bne L_1384
    psq_st f0, 16(r10), 0, 0
    cmpwi r7, 0
    psq_st f0, 24(r10), 0, 0
    bne L_1314
    psq_stu f0, 32(r10), 0, 0
    addi r3, r3, 16
    addi r5, r5, 32
    bdnz L_12b8
    b L_14d8
L_1314:
    ps_msub f2, f10, f28, f10
    psq_lu f11, 32(r5), 0, 0
    ps_sub f1, f28, f27
    lwz r6, 20(r3)
    ps_merge00 f9, f10, f10
    lhz r7, 18(r3)
    ps_msub f3, f10, f29, f2
    ps_merge11 f5, f10, f2
    ps_nmsub f4, f10, f1, f3
    ps_add f7, f9, f5
    psq_l f10, 16(r3), 0, 5
    lwz r0, 28(r3)
    ps_sub f5, f9, f5
    ps_merge11 f6, f3, f4
    lwz r8, 24(r3)
    ps_add f8, f9, f6
    ps_sub f6, f9, f6
    psq_stu f7, 8(r10), 0, 0
    ps_merge10 f6, f6, f6
    psq_stu f8, 8(r10), 0, 0
    ps_merge10 f5, f5, f5
    or r0, r0, r8
    psq_stu f6, 8(r10), 0, 0
    ps_mul f10, f10, f11
    psq_stu f5, 8(r10), 0, 0
    addi r3, r3, 16
    bdnz L_12d8
    b L_14d8
L_1384:
    psq_l f1, 4(r3), 0, 5
    psq_l f9, 8(r5), 0, 0
    lwz r0, 28(r3)
    ps_mul f1, f1, f9
    lwz r8, 24(r3)
    lwz r6, 20(r3)
    lhz r7, 18(r3)
    ps_sub f3, f10, f1
    ps_add f2, f10, f1
    ps_mul f8, f3, f28
    ps_madd f4, f1, f29, f3
    ps_nmsub f5, f1, f29, f2
    ps_nmsub f6, f1, f26, f8
    ps_nmsub f7, f10, f27, f8
    ps_merge00 f4, f2, f4
    ps_sub f6, f6, f2
    ps_merge00 f5, f5, f3
    ps_msub f8, f3, f29, f6
    ps_merge11 f2, f2, f6
    psq_lu f10, 16(r3), 0, 5
    psq_lu f11, 32(r5), 0, 0
    ps_sub f7, f7, f8
    ps_add f9, f4, f2
    ps_sub f4, f4, f2
    ps_merge11 f3, f8, f7
    psq_stu f9, 8(r10), 0, 0
    or r0, r0, r8
    ps_sub f1, f5, f3
    ps_add f0, f5, f3
    psq_stu f0, 8(r10), 0, 0
    ps_merge10 f1, f1, f1
    ps_merge10 f4, f4, f4
    psq_stu f1, 8(r10), 0, 0
    ps_mul f10, f10, f11
    psq_stu f4, 8(r10), 0, 0
    bdnz L_12d8
    b L_14d8
L_1418:
    psq_l f9, 4(r3), 0, 5
    psq_l f5, 8(r5), 0, 0
    ps_mul f9, f9, f5
    psq_l f2, 8(r3), 0, 5
    psq_l f6, 16(r5), 0, 0
    ps_merge01 f0, f10, f9
    psq_l f3, 12(r3), 0, 5
    ps_merge01 f1, f9, f10
    psq_l f7, 24(r5), 0, 0
    lwz r0, 28(r3)
    ps_madd f4, f2, f6, f0
    ps_nmsub f5, f2, f6, f0
    lwz r8, 24(r3)
    ps_madd f6, f3, f7, f1
    lwz r6, 20(r3)
    ps_nmsub f7, f3, f7, f1
    lhz r7, 18(r3)
    ps_add f0, f4, f6
    ps_sub f8, f7, f5
    ps_msub f2, f7, f29, f6
    ps_sub f3, f4, f6
    ps_mul f8, f8, f28
    ps_add f1, f5, f2
    ps_sub f2, f5, f2
    ps_nmsub f6, f5, f26, f8
    ps_msub f4, f7, f27, f8
    ps_merge00 f1, f0, f1
    ps_sub f6, f6, f0
    ps_merge00 f2, f2, f3
    ps_madd f5, f3, f29, f6
    ps_merge11 f7, f0, f6
    psq_lu f10, 16(r3), 0, 5
    psq_lu f11, 32(r5), 0, 0
    ps_sub f4, f4, f5
    ps_add f3, f1, f7
    ps_sub f0, f1, f7
    ps_merge11 f4, f5, f4
    ps_mul f10, f10, f11
    ps_add f5, f2, f4
    ps_sub f6, f2, f4
    ps_merge10 f5, f5, f5
    psq_stu f3, 8(r10), 0, 0
    ps_merge10 f0, f0, f0
    psq_stu f6, 8(r10), 0, 0
    psq_stu f5, 8(r10), 0, 0
    or r0, r0, r8
    psq_stu f0, 8(r10), 0, 0
    bdnz L_12d8
L_14d8:
    lis r10, THPIdctBuffer@ha
    lwz r0, THPOutputStride(r0)
    addi r10, r10, THPIdctBuffer@l
    slwi r4, r4, 2
    psq_l f10, 0(r10), 0, 0
    slwi r5, r0, 2
    psq_l f11, 128(r10), 0, 0
    add r5, r4, r5
    lwz r0, THPOutputPlane(r0)
    li r3, 3
    ps_add f6, f10, f11
    psq_l f12, 64(r10), 0, 0
    psq_l f13, 192(r10), 0, 0
    ps_sub f8, f10, f11
    add r6, r0, r4
    add r7, r0, r5
    ps_add f6, f6, f25
    ps_add f7, f12, f13
    ps_sub f9, f12, f13
    ps_add f8, f8, f25
    ps_add f0, f6, f7
    mtctr r3
L_1530:
    ps_msub f9, f9, f29, f7
    psq_l f4, 32(r10), 0, 0
    ps_sub f3, f6, f7
    psq_l f5, 96(r10), 0, 0
    psq_l f6, 160(r10), 0, 0
    psq_l f7, 224(r10), 0, 0
    ps_add f1, f8, f9
    psq_l f10, 8(r10), 0, 0
    ps_sub f2, f8, f9
    psq_l f11, 136(r10), 0, 0
    ps_add f8, f6, f5
    psq_l f12, 72(r10), 0, 0
    ps_add f9, f4, f7
    psq_l f13, 200(r10), 0, 0
    ps_sub f6, f6, f5
    addi r10, r10, 8
    ps_sub f4, f4, f7
    ps_add f7, f9, f8
    ps_sub f5, f9, f8
    ps_add f8, f6, f4
    ps_add f9, f0, f7
    ps_sub f30, f0, f7
    ps_mul f8, f8, f28
    ps_madd f6, f6, f26, f8
    ps_sub f6, f6, f7
    psq_st f9, 0(r6), 0, 6
    ps_msub f4, f4, f27, f8
    ps_msub f5, f5, f29, f6
    ps_add f9, f1, f6
    ps_sub f31, f1, f6
    psq_st f9, 8(r6), 0, 6
    ps_add f4, f4, f5
    ps_add f8, f2, f5
    psq_st f8, 16(r6), 0, 6
    ps_sub f9, f3, f4
    ps_add f0, f3, f4
    psq_st f9, 24(r6), 0, 6
    ps_add f6, f10, f11
    ps_sub f1, f2, f5
    psq_st f0, 0(r7), 0, 6
    ps_sub f8, f10, f11
    ps_add f6, f6, f25
    psq_st f1, 8(r7), 0, 6
    ps_add f7, f12, f13
    ps_sub f9, f12, f13
    psq_st f31, 16(r7), 0, 6
    addi r4, r4, 2
    add r6, r0, r4
    ps_add f0, f6, f7
    psq_st f30, 24(r7), 0, 6
    addi r5, r5, 2
    ps_add f8, f8, f25
    add r7, r0, r5
    bdnz L_1530
    ps_msub f9, f9, f29, f7
    psq_l f4, 32(r10), 0, 0
    ps_sub f3, f6, f7
    psq_l f5, 96(r10), 0, 0
    psq_l f6, 160(r10), 0, 0
    psq_l f7, 224(r10), 0, 0
    ps_add f1, f8, f9
    ps_sub f2, f8, f9
    ps_add f8, f6, f5
    ps_add f9, f4, f7
    ps_sub f6, f6, f5
    ps_sub f4, f4, f7
    ps_add f7, f9, f8
    ps_sub f5, f9, f8
    ps_add f8, f6, f4
    ps_add f9, f0, f7
    ps_sub f30, f0, f7
    ps_mul f8, f8, f28
    ps_madd f6, f6, f26, f8
    psq_st f9, 0(r6), 0, 6
    ps_msub f4, f4, f27, f8
    ps_sub f6, f6, f7
    psq_st f30, 24(r7), 0, 6
    ps_add f9, f1, f6
    ps_msub f5, f5, f29, f6
    ps_sub f31, f1, f6
    psq_st f9, 8(r6), 0, 6
    ps_add f4, f4, f5
    ps_add f8, f2, f5
    psq_st f31, 16(r7), 0, 6
    psq_st f8, 16(r6), 0, 6
    ps_sub f9, f3, f4
    ps_add f0, f3, f4
    psq_st f9, 24(r6), 0, 6
    ps_sub f1, f2, f5
    psq_st f0, 0(r7), 0, 6
    psq_st f1, 8(r7), 0, 6
    psq_l f31, 120(r1), 0, 0
    lfd f31, 112(r1)
    psq_l f30, 104(r1), 0, 0
    lfd f30, 96(r1)
    psq_l f29, 88(r1), 0, 0
    lfd f29, 80(r1)
    psq_l f28, 72(r1), 0, 0
    lfd f28, 64(r1)
    psq_l f27, 56(r1), 0, 0
    lfd f27, 48(r1)
    psq_l f26, 40(r1), 0, 0
    lfd f26, 32(r1)
    psq_l f25, 24(r1), 0, 0
    lfd f25, 16(r1)
    addi r1, r1, 128
    blr
}

/* Runs the 8x8 inverse DCT of a block and stores the pixels at the lower row of the output plane (paired singles). */
asm void THPIdctLower(s16* block, u32 x)
{
    nofralloc
    stwu r1, -128(r1)
    stfd f31, 112(r1)
    psq_st f31, 120(r1), 0, 0
    stfd f30, 96(r1)
    psq_st f30, 104(r1), 0, 0
    stfd f29, 80(r1)
    psq_st f29, 88(r1), 0, 0
    stfd f28, 64(r1)
    psq_st f28, 72(r1), 0, 0
    stfd f27, 48(r1)
    psq_st f27, 56(r1), 0, 0
    stfd f26, 32(r1)
    psq_st f26, 40(r1), 0, 0
    stfd f25, 16(r1)
    psq_st f25, 24(r1), 0, 0
    lis r5, THPIdctBuffer@ha
    li r7, 8
    addi r5, r5, THPIdctBuffer@l
    lfs f29, THPIdctSqrt2(r0)
    addi r10, r5, -8
    lfs f28, THPIdctCos2(r0)
    lfs f27, THPIdctCos3(r0)
    lfs f26, THPIdctNegCos4(r0)
    lfs f25, THPIdctRound(r0)
    lwz r5, THPQuantTable(r0)
    mtctr r7
L_1748:
    psq_l f10, 0(r3), 0, 5
    psq_l f11, 0(r5), 0, 0
    lwz r0, 12(r3)
    lwz r8, 8(r3)
    ps_mul f10, f10, f11
    lwz r6, 4(r3)
    lhz r7, 2(r3)
    or r0, r0, r8
L_1768:
    cmpwi r0, 0
    bne L_18a8
    ps_merge00 f0, f10, f10
    cmpwi r6, 0
    psq_st f0, 8(r10), 0, 0
    bne L_1814
    psq_st f0, 16(r10), 0, 0
    cmpwi r7, 0
    psq_st f0, 24(r10), 0, 0
    bne L_17a4
    psq_stu f0, 32(r10), 0, 0
    addi r3, r3, 16
    addi r5, r5, 32
    bdnz L_1748
    b L_1968
L_17a4:
    ps_msub f2, f10, f28, f10
    psq_lu f11, 32(r5), 0, 0
    ps_sub f1, f28, f27
    lwz r6, 20(r3)
    ps_merge00 f9, f10, f10
    lhz r7, 18(r3)
    ps_msub f3, f10, f29, f2
    ps_merge11 f5, f10, f2
    ps_nmsub f4, f10, f1, f3
    ps_add f7, f9, f5
    psq_l f10, 16(r3), 0, 5
    lwz r0, 28(r3)
    ps_sub f5, f9, f5
    ps_merge11 f6, f3, f4
    lwz r8, 24(r3)
    ps_add f8, f9, f6
    ps_sub f6, f9, f6
    psq_stu f7, 8(r10), 0, 0
    ps_merge10 f6, f6, f6
    psq_stu f8, 8(r10), 0, 0
    ps_merge10 f5, f5, f5
    or r0, r0, r8
    psq_stu f6, 8(r10), 0, 0
    ps_mul f10, f10, f11
    psq_stu f5, 8(r10), 0, 0
    addi r3, r3, 16
    bdnz L_1768
    b L_1968
L_1814:
    psq_l f1, 4(r3), 0, 5
    psq_l f9, 8(r5), 0, 0
    lwz r0, 28(r3)
    ps_mul f1, f1, f9
    lwz r8, 24(r3)
    lwz r6, 20(r3)
    lhz r7, 18(r3)
    ps_sub f3, f10, f1
    ps_add f2, f10, f1
    ps_mul f8, f3, f28
    ps_madd f4, f1, f29, f3
    ps_nmsub f5, f1, f29, f2
    ps_nmsub f6, f1, f26, f8
    ps_nmsub f7, f10, f27, f8
    ps_merge00 f4, f2, f4
    ps_sub f6, f6, f2
    ps_merge00 f5, f5, f3
    ps_msub f8, f3, f29, f6
    ps_merge11 f2, f2, f6
    psq_lu f10, 16(r3), 0, 5
    psq_lu f11, 32(r5), 0, 0
    ps_sub f7, f7, f8
    ps_add f9, f4, f2
    ps_sub f4, f4, f2
    ps_merge11 f3, f8, f7
    psq_stu f9, 8(r10), 0, 0
    or r0, r0, r8
    ps_sub f1, f5, f3
    ps_add f0, f5, f3
    psq_stu f0, 8(r10), 0, 0
    ps_merge10 f1, f1, f1
    ps_merge10 f4, f4, f4
    psq_stu f1, 8(r10), 0, 0
    ps_mul f10, f10, f11
    psq_stu f4, 8(r10), 0, 0
    bdnz L_1768
    b L_1968
L_18a8:
    psq_l f9, 4(r3), 0, 5
    psq_l f5, 8(r5), 0, 0
    ps_mul f9, f9, f5
    psq_l f2, 8(r3), 0, 5
    psq_l f6, 16(r5), 0, 0
    ps_merge01 f0, f10, f9
    psq_l f3, 12(r3), 0, 5
    ps_merge01 f1, f9, f10
    psq_l f7, 24(r5), 0, 0
    lwz r0, 28(r3)
    ps_madd f4, f2, f6, f0
    ps_nmsub f5, f2, f6, f0
    lwz r8, 24(r3)
    ps_madd f6, f3, f7, f1
    lwz r6, 20(r3)
    ps_nmsub f7, f3, f7, f1
    lhz r7, 18(r3)
    ps_add f0, f4, f6
    ps_sub f8, f7, f5
    ps_msub f2, f7, f29, f6
    ps_sub f3, f4, f6
    ps_mul f8, f8, f28
    ps_add f1, f5, f2
    ps_sub f2, f5, f2
    ps_nmsub f6, f5, f26, f8
    ps_msub f4, f7, f27, f8
    ps_merge00 f1, f0, f1
    ps_sub f6, f6, f0
    ps_merge00 f2, f2, f3
    ps_madd f5, f3, f29, f6
    ps_merge11 f7, f0, f6
    psq_lu f10, 16(r3), 0, 5
    psq_lu f11, 32(r5), 0, 0
    ps_sub f4, f4, f5
    ps_add f3, f1, f7
    ps_sub f0, f1, f7
    ps_merge11 f4, f5, f4
    ps_mul f10, f10, f11
    ps_add f5, f2, f4
    ps_sub f6, f2, f4
    ps_merge10 f5, f5, f5
    psq_stu f3, 8(r10), 0, 0
    ps_merge10 f0, f0, f0
    psq_stu f6, 8(r10), 0, 0
    psq_stu f5, 8(r10), 0, 0
    or r0, r0, r8
    psq_stu f0, 8(r10), 0, 0
    bdnz L_1768
L_1968:
    lis r10, THPIdctBuffer@ha
    lwz r0, THPOutputStride(r0)
    addi r10, r10, THPIdctBuffer@l
    slwi r3, r4, 2
    psq_l f10, 0(r10), 0, 0
    slwi r4, r0, 3
    psq_l f11, 128(r10), 0, 0
    slwi r5, r0, 2
    add r4, r4, r3
    lwz r0, THPOutputPlane(r0)
    ps_add f6, f10, f11
    psq_l f12, 64(r10), 0, 0
    psq_l f13, 192(r10), 0, 0
    ps_sub f8, f10, f11
    add r5, r4, r5
    li r3, 3
    ps_add f6, f6, f25
    add r6, r0, r4
    ps_add f7, f12, f13
    add r7, r0, r5
    ps_sub f9, f12, f13
    ps_add f8, f8, f25
    ps_add f0, f6, f7
    mtctr r3
L_19c8:
    ps_msub f9, f9, f29, f7
    psq_l f4, 32(r10), 0, 0
    ps_sub f3, f6, f7
    psq_l f5, 96(r10), 0, 0
    psq_l f6, 160(r10), 0, 0
    psq_l f7, 224(r10), 0, 0
    ps_add f1, f8, f9
    psq_l f10, 8(r10), 0, 0
    ps_sub f2, f8, f9
    psq_l f11, 136(r10), 0, 0
    ps_add f8, f6, f5
    psq_l f12, 72(r10), 0, 0
    ps_add f9, f4, f7
    psq_l f13, 200(r10), 0, 0
    ps_sub f6, f6, f5
    addi r10, r10, 8
    ps_sub f4, f4, f7
    ps_add f7, f9, f8
    ps_sub f5, f9, f8
    ps_add f8, f6, f4
    ps_add f9, f0, f7
    ps_sub f30, f0, f7
    ps_mul f8, f8, f28
    ps_madd f6, f6, f26, f8
    ps_sub f6, f6, f7
    psq_st f9, 0(r6), 0, 6
    ps_msub f4, f4, f27, f8
    ps_msub f5, f5, f29, f6
    ps_add f9, f1, f6
    ps_sub f31, f1, f6
    psq_st f9, 8(r6), 0, 6
    ps_add f4, f4, f5
    ps_add f8, f2, f5
    psq_st f8, 16(r6), 0, 6
    ps_sub f9, f3, f4
    ps_add f0, f3, f4
    psq_st f9, 24(r6), 0, 6
    ps_add f6, f10, f11
    ps_sub f1, f2, f5
    psq_st f0, 0(r7), 0, 6
    ps_sub f8, f10, f11
    ps_add f6, f6, f25
    psq_st f1, 8(r7), 0, 6
    ps_add f7, f12, f13
    ps_sub f9, f12, f13
    psq_st f31, 16(r7), 0, 6
    addi r4, r4, 2
    add r6, r0, r4
    ps_add f0, f6, f7
    psq_st f30, 24(r7), 0, 6
    addi r5, r5, 2
    ps_add f8, f8, f25
    add r7, r0, r5
    bdnz L_19c8
    ps_msub f9, f9, f29, f7
    psq_l f4, 32(r10), 0, 0
    ps_sub f3, f6, f7
    psq_l f5, 96(r10), 0, 0
    psq_l f6, 160(r10), 0, 0
    psq_l f7, 224(r10), 0, 0
    ps_add f1, f8, f9
    ps_sub f2, f8, f9
    ps_add f8, f6, f5
    ps_add f9, f4, f7
    ps_sub f6, f6, f5
    ps_sub f4, f4, f7
    ps_add f7, f9, f8
    ps_sub f5, f9, f8
    ps_add f8, f6, f4
    ps_add f9, f0, f7
    ps_sub f30, f0, f7
    ps_mul f8, f8, f28
    ps_madd f6, f6, f26, f8
    psq_st f9, 0(r6), 0, 6
    ps_msub f4, f4, f27, f8
    ps_sub f6, f6, f7
    psq_st f30, 24(r7), 0, 6
    ps_msub f5, f5, f29, f6
    ps_add f9, f1, f6
    ps_sub f31, f1, f6
    psq_st f9, 8(r6), 0, 6
    ps_add f8, f2, f5
    ps_add f4, f4, f5
    psq_st f8, 16(r6), 0, 6
    ps_sub f9, f3, f4
    psq_st f31, 16(r7), 0, 6
    ps_add f0, f3, f4
    psq_st f9, 24(r6), 0, 6
    ps_sub f1, f2, f5
    psq_st f0, 0(r7), 0, 6
    psq_st f1, 8(r7), 0, 6
    psq_l f31, 120(r1), 0, 0
    lfd f31, 112(r1)
    psq_l f30, 104(r1), 0, 0
    lfd f30, 96(r1)
    psq_l f29, 88(r1), 0, 0
    lfd f29, 80(r1)
    psq_l f28, 72(r1), 0, 0
    lfd f28, 64(r1)
    psq_l f27, 56(r1), 0, 0
    lfd f27, 48(r1)
    psq_l f26, 40(r1), 0, 0
    lfd f26, 32(r1)
    psq_l f25, 24(r1), 0, 0
    lfd f25, 16(r1)
    addi r1, r1, 128
    blr
}

void THPDecodeFrame640x480(void)
{
    u8 mcu;
    u32 x;

    LCQueueWait(3);
    for (mcu = 0; mcu < THPState->mcuColumns; mcu++) {
        THPDecodeBlockY(THPState, THPBlocks[0]);
        THPDecodeBlockY(THPState, THPBlocks[1]);
        THPDecodeBlockY(THPState, THPBlocks[2]);
        THPDecodeBlockY(THPState, THPBlocks[3]);
        THPDecodeBlockU(THPState, THPBlocks[4]);
        THPDecodeBlockV(THPState, THPBlocks[5]);
        x = (mcu * 16) & 0xFF0;
        THPOutputPlane = (u8*)THPPlane640[0];
        THPOutputStride = 640;
        THPQuantTable = THPState->quant[THPState->component[0].quantTable];
        THPIdctUpper(THPBlocks[0], x);
        THPIdctUpper(THPBlocks[1], x + 8);
        THPIdctLower(THPBlocks[2], x);
        THPIdctLower(THPBlocks[3], x + 8);
        x >>= 1;
        THPOutputPlane = (u8*)THPPlane640[1];
        THPOutputStride = 320;
        THPQuantTable = THPState->quant[THPState->component[1].quantTable];
        THPIdctUpper(THPBlocks[4], x);
        THPOutputPlane = (u8*)THPPlane640[2];
        THPQuantTable = THPState->quant[THPState->component[2].quantTable];
        THPIdctUpper(THPBlocks[5], x);
        if (THPState->restartEnabled != 0) {
            THPState->restartCounter--;
            if (THPState->restartCounter == 0) {
                THPState->restartCounter = THPState->restartInterval;
                THPState->bitPosition = ((THPState->bitPosition + 6) & ~7) + 1;
                if (THPState->bitPosition > 0x20) {
                    THPState->bitPosition = 0x21;
                }
                THPState->component[0].dcPredictor = 0;
                THPState->component[1].dcPredictor = 0;
                THPState->component[2].dcPredictor = 0;
            }
        }
    }
    LCStoreData(THPState->outputY, (u8*)THPPlane640[0], 0x2800);
    LCStoreData(THPState->outputU, (u8*)THPPlane640[1], 0xA00);
    LCStoreData(THPState->outputV, (u8*)THPPlane640[2], 0xA00);
    THPState->outputY += 0x2800;
    THPState->outputU += 0xA00;
    THPState->outputV += 0xA00;
}

void THPDecodeFrameAnySize(void)
{
    u16 width = THPState->width;
    u8 mcu;
    u32 x;

    LCQueueWait(3);
    for (mcu = 0; mcu < THPState->mcuColumns; mcu++) {
        THPDecodeBlockY(THPState, THPBlocks[0]);
        THPDecodeBlockY(THPState, THPBlocks[1]);
        THPDecodeBlockY(THPState, THPBlocks[2]);
        THPDecodeBlockY(THPState, THPBlocks[3]);
        THPDecodeBlockU(THPState, THPBlocks[4]);
        THPDecodeBlockV(THPState, THPBlocks[5]);
        x = (mcu * 16) & 0xFF0;
        THPOutputPlane = (u8*)THPPlane640[0];
        THPOutputStride = width;
        THPQuantTable = THPState->quant[THPState->component[0].quantTable];
        THPIdctUpper(THPBlocks[0], x);
        THPIdctUpper(THPBlocks[1], x + 8);
        THPIdctLower(THPBlocks[2], x);
        THPIdctLower(THPBlocks[3], x + 8);
        x >>= 1;
        THPOutputPlane = (u8*)THPPlane640[1];
        THPOutputStride = width >> 1;
        THPQuantTable = THPState->quant[THPState->component[1].quantTable];
        THPIdctUpper(THPBlocks[4], x);
        THPOutputPlane = (u8*)THPPlane640[2];
        THPQuantTable = THPState->quant[THPState->component[2].quantTable];
        THPIdctUpper(THPBlocks[5], x);
        if (THPState->restartEnabled != 0) {
            THPState->restartCounter--;
            if (THPState->restartCounter == 0) {
                THPState->restartCounter = THPState->restartInterval;
                THPState->bitPosition = ((THPState->bitPosition + 6) & ~7) + 1;
                if (THPState->bitPosition > 0x20) {
                    THPState->bitPosition = 0x21;
                }
                THPState->component[0].dcPredictor = 0;
                THPState->component[1].dcPredictor = 0;
                THPState->component[2].dcPredictor = 0;
            }
        }
    }
    LCStoreData(THPState->outputY, (u8*)THPPlane640[0], (width * 16) & ~0xFF);
    LCStoreData(THPState->outputU, (u8*)THPPlane640[1], (width * 4) & ~0x3F);
    LCStoreData(THPState->outputV, (u8*)THPPlane640[2], (width * 4) & ~0x3F);
    THPState->outputY += (width * 16) & ~0xFF;
    THPState->outputU += (width * 4) & ~0x3F;
    THPState->outputV += (width * 4) & ~0x3F;
}

/* Huffman-decodes one luma block with the component 0 tables into the block buffer (paired-single stores). */
asm void THPDecodeBlockY(THPDecoder* state, s16* block)
{
    nofralloc
    stwu r1, -32(r1)
    stw r31, 28(r1)
    stw r30, 24(r1)
    stw r29, 20(r1)
    stw r28, 16(r1)
    dcbz 0, r4
    lwz r12, 1700(r3)
    lwz r8, THPDcTable0(r0)
    cmpwi r12, 28
    lwz r11, 1696(r3)
    addi r5, r12, 4
    addi r10, r8, 32
    rlwnm r9, r11, r5, 27, 31
    bgt L_2120
    lbzx r5, r8, r9
    lbzx r10, r10, r9
    cmpwi r5, 255
    beq L_2084
    add r12, r12, r10
    stw r12, 1700(r3)
    b L_22c4
L_2084:
    addi r6, r8, 88
    li r5, 5
    addi r12, r12, 5
L_2090:
    cmpwi r12, 33
    slwi r9, r9, 1
    beq L_20b0
    rlwnm r10, r11, r12, 31, 31
    lwzu r0, 4(r6)
    or r9, r9, r10
    addi r12, r12, 1
    b L_20f4
L_20b0:
    lwz r10, 1692(r3)
    li r12, 1
    lwzu r11, 4(r10)
    lwzu r0, 4(r6)
    rlwimi r9, r11, 1, 31, 31
    stw r10, 1692(r3)
    stw r11, 1696(r3)
    b L_20e0
L_20d0:
    slwi r9, r9, 1
    rlwnm r10, r11, r12, 31, 31
    lwzu r0, 4(r6)
    or r9, r9, r10
L_20e0:
    cmpw r9, r0
    addi r12, r12, 1
    addi r5, r5, 1
    bgt L_20d0
    b L_2100
L_20f4:
    cmpw r9, r0
    addi r5, r5, 1
    bgt L_2090
L_2100:
    stw r12, 1700(r3)
    slwi r0, r5, 2
    add r5, r8, r0
    lwz r0, 64(r8)
    lwz r5, 140(r5)
    add r0, r0, r9
    lbzx r5, r5, r0
    b L_22c4
L_2120:
    cmpwi r12, 33
    lwz r9, 1692(r3)
    beq L_21d8
    cmpwi r12, 32
    rlwnm r5, r11, r5, 27, 31
    beq L_2160
    lbzx r9, r8, r5
    lbzx r10, r10, r5
    cmpwi r9, 255
    add r5, r12, r10
    beq L_223c
    cmpwi r5, 33
    stw r5, 1700(r3)
    bgt L_223c
    mr r5, r9
    b L_22c4
L_2160:
    lwzu r11, 4(r9)
    stw r9, 1692(r3)
    rlwimi r5, r11, 4, 28, 31
    lbzx r9, r8, r5
    lbzx r10, r10, r5
    cmpwi r9, 255
    stw r10, 1700(r3)
    stw r11, 1696(r3)
    beq L_218c
    mr r5, r9
    b L_22c4
L_218c:
    slwi r9, r5, 27
    addi r6, r8, 88
    rlwimi r9, r11, 31, 1, 31
    li r12, 5
    nop
L_21a0:
    subfic r11, r12, 31
    lwzu r0, 4(r6)
    srw r5, r9, r11
    addi r12, r12, 1
    cmpw r5, r0
    bgt L_21a0
    stw r12, 1700(r3)
L_21bc:
    slwi r0, r12, 2
    lwz r7, 64(r8)
    add r6, r8, r0
    lwz r6, 140(r6)
    add r0, r7, r5
    lbzx r5, r6, r0
    b L_22c4
L_21d8:
    lwzu r11, 4(r9)
    stw r9, 1692(r3)
    srwi r5, r11, 27
    lbzx r12, r8, r5
    lbzx r10, r10, r5
    cmpwi r12, 255
    stw r11, 1696(r3)
    addi r10, r10, 1
    beq L_2208
    stw r10, 1700(r3)
    mr r5, r12
    b L_22c4
L_2208:
    li r12, 5
    li r6, 20
L_2210:
    subfic r9, r12, 31
    addi r6, r6, 4
    add r5, r8, r6
    addi r12, r12, 1
    lwz r0, 68(r5)
    srw r5, r11, r9
    cmpw cr1, r5, r0
    bgt cr1, L_2210
    addi r0, r12, 1
    stw r0, 1700(r3)
    b L_21bc
L_223c:
    subfic r0, r12, 33
    li r5, -1
    slw r7, r5, r0
    lwz r9, 1692(r3)
    andc r5, r11, r7
    addi r7, r8, 68
    subfic r6, r12, 33
    lwzu r11, 4(r9)
    addi r12, r6, 1
    slwi r6, r6, 2
    stw r11, 1696(r3)
    add r7, r7, r6
    slwi r5, r5, 1
    stw r9, 1692(r3)
    rlwimi r5, r11, 1, 31, 31
    li r9, 2
    lwzu r6, 4(r7)
    b L_229c
    nop
L_2288:
    slwi r5, r5, 1
    lwzu r6, 4(r7)
    add r5, r5, r10
    addi r9, r9, 1
    addi r12, r12, 1
L_229c:
    cmpw r5, r6
    rlwnm r10, r11, r9, 31, 31
    bgt L_2288
    stw r9, 1700(r3)
    slwi r0, r12, 2
    add r6, r8, r0
    lwz r0, 64(r8)
    lwz r6, 140(r6)
    add r0, r0, r5
    lbzx r5, r6, r0
L_22c4:
    li r0, 32
    dcbz r4, r0
    li r0, 64
    li r7, 0
    dcbz r4, r0
    cmpwi cr1, r5, 0
    beq cr1, L_2364
    lwz r7, 1700(r3)
    subfic r8, r7, 33
    lwz r6, 1696(r3)
    subfc. r9, r8, r5
    addi r10, r7, -1
    bgt L_2310
    add r0, r7, r5
    stw r0, 1700(r3)
    slw r7, r6, r10
    subfic r0, r5, 32
    srw r7, r7, r0
    b L_233c
L_2310:
    slw r0, r6, r10
    lwz r7, 1692(r3)
    lwzu r6, 4(r7)
    addi r9, r9, 1
    stw r6, 1696(r3)
    srw r6, r6, r8
    stw r7, 1692(r3)
    add r0, r6, r0
    stw r9, 1700(r3)
    subfic r9, r5, 32
    srw r7, r0, r9
L_233c:
    extsh r6, r7
    subfic r0, r5, 32
    cntlzw r6, r6
    cmpw cr1, r6, r0
    ble cr1, L_2364
    li r0, -1
    slw r0, r0, r5
    add r5, r7, r0
    addi r0, r5, 1
    extsh r7, r0
L_2364:
    li r0, 96
    dcbz r4, r0
    lis r10, THPZigzagTable@ha
    lha r0, 1668(r3)
    addi r10, r10, THPZigzagTable@l
    li r5, 1
    li r11, -1
    add r0, r0, r7
    sth r0, 1668(r3)
    sth r0, 0(r4)
    lwz r8, THPAcTable0(r0)
    lwz r6, 1700(r3)
    lwz r0, 1696(r3)
    addi r7, r8, 32
    b L_267c
L_23a0:
    cmpwi r6, 28
    addi r30, r6, 4
    rlwnm r29, r0, r30, 27, 31
    bgt L_2464
    lbzx r31, r8, r29
    lbzx r30, r7, r29
    cmpwi r31, 255
    beq L_23c8
    add r6, r6, r30
    b L_25e8
L_23c8:
    addi r9, r8, 88
    li r30, 5
    addi r6, r6, 5
    nop
L_23d8:
    cmpwi r6, 33
    slwi r29, r29, 1
    beq L_23f8
    rlwnm r31, r0, r6, 31, 31
    lwzu r12, 4(r9)
    or r29, r29, r31
    addi r6, r6, 1
    b L_243c
L_23f8:
    lwz r31, 1692(r3)
    li r6, 1
    lwzu r0, 4(r31)
    lwzu r12, 4(r9)
    rlwimi r29, r0, 1, 31, 31
    stw r31, 1692(r3)
    b L_2428
    nop
L_2418:
    slwi r29, r29, 1
    rlwnm r31, r0, r6, 31, 31
    lwzu r12, 4(r9)
    or r29, r29, r31
L_2428:
    cmpw r29, r12
    addi r6, r6, 1
    addi r30, r30, 1
    bgt L_2418
    b L_2448
L_243c:
    cmpw r29, r12
    addi r30, r30, 1
    bgt L_23d8
L_2448:
    slwi r9, r30, 2
    lwz r31, 64(r8)
    add r9, r8, r9
    lwz r12, 140(r9)
    add r9, r31, r29
    lbzx r31, r12, r9
    b L_25e8
L_2464:
    cmpwi r6, 33
    lwz r29, 1692(r3)
    beq L_24a0
    cmpwi r6, 32
    rlwnm r30, r0, r30, 27, 31
    beq L_2508
    lbzx r31, r8, r30
    lbzx r28, r7, r30
    cmpwi r31, 255
    add r30, r6, r28
    beq L_256c
    cmpwi r30, 33
    bgt L_256c
    mr r6, r30
    b L_25e8
L_24a0:
    lwzu r0, 4(r29)
    stw r29, 1692(r3)
    srwi r30, r0, 27
    lbzx r31, r8, r30
    lbzx r29, r7, r30
    cmpwi r31, 255
    addi r6, r29, 1
    beq L_24c4
    b L_25e8
L_24c4:
    li r31, 5
    li r6, 20
    nop
L_24d0:
    subfic r29, r31, 31
    addi r6, r6, 4
    add r12, r8, r6
    addi r31, r31, 1
    lwz r9, 68(r12)
    srw r30, r0, r29
    cmpw cr1, r30, r9
    bgt cr1, L_24d0
    lwz r9, 64(r8)
    addi r6, r31, 1
    lwz r12, 140(r12)
    add r9, r9, r30
    lbzx r31, r12, r9
    b L_25e8
L_2508:
    lwzu r0, 4(r29)
    stw r29, 1692(r3)
    rlwimi r30, r0, 4, 28, 31
    lbzx r31, r8, r30
    lbzx r6, r7, r30
    cmpwi r31, 255
    beq L_2528
    b L_25e8
L_2528:
    slwi r29, r30, 27
    addi r9, r8, 88
    rlwimi r29, r0, 31, 1, 31
    li r6, 5
L_2538:
    subfic r31, r6, 31
    lwzu r12, 4(r9)
    srw r30, r29, r31
    addi r6, r6, 1
    cmpw r30, r12
    bgt L_2538
    slwi r9, r6, 2
    lwz r31, 64(r8)
    add r9, r8, r9
    lwz r12, 140(r9)
    add r9, r31, r30
    lbzx r31, r12, r9
    b L_25e8
L_256c:
    subfic r9, r6, 33
    lwz r29, 1692(r3)
    slw r9, r11, r9
    andc r30, r0, r9
    addi r9, r8, 68
    subfic r12, r6, 33
    lwzu r0, 4(r29)
    addi r31, r12, 1
    slwi r12, r12, 2
    slwi r30, r30, 1
    stw r29, 1692(r3)
    add r9, r9, r12
    rlwimi r30, r0, 1, 31, 31
    li r6, 2
    lwzu r12, 4(r9)
    b L_25c4
    nop
L_25b0:
    slwi r30, r30, 1
    lwzu r12, 4(r9)
    add r30, r30, r28
    addi r6, r6, 1
    addi r31, r31, 1
L_25c4:
    cmpw r30, r12
    rlwnm r28, r0, r6, 31, 31
    bgt L_25b0
    slwi r9, r31, 2
    lwz r31, 64(r8)
    add r9, r8, r9
    lwz r12, 140(r9)
    add r9, r31, r30
    lbzx r31, r12, r9
L_25e8:
    andi. r28, r31, 15
    srawi r31, r31, 4
    beq L_266c
    add r5, r5, r31
    subfic r30, r6, 33
    subfc. r29, r30, r28
    addi r9, r6, -1
    bgt L_261c
    add r6, r6, r28
    slw r12, r0, r9
    subfic r9, r28, 32
    srw r31, r12, r9
    b L_2640
L_261c:
    slw r9, r0, r9
    lwz r12, 1692(r3)
    lwzu r0, 4(r12)
    addi r6, r29, 1
    stw r12, 1692(r3)
    srw r12, r0, r30
    add r9, r12, r9
    subfic r29, r28, 32
    srw r31, r9, r29
L_2640:
    cntlzw r12, r31
    subfic r9, r28, 32
    cmpw cr1, r12, r9
    ble cr1, L_265c
    slw r9, r11, r28
    add r9, r9, r31
    addi r31, r9, 1
L_265c:
    lbzx r9, r10, r5
    slwi r9, r9, 1
    sthx r31, r4, r9
    b L_2678
L_266c:
    cmpwi cr1, r31, 15
    bne cr1, L_2684
    addi r5, r5, 15
L_2678:
    addi r5, r5, 1
L_267c:
    cmpwi cr1, r5, 64
    blt cr1, L_23a0
L_2684:
    stw r6, 1700(r3)
    stw r0, 1696(r3)
    lwz r31, 28(r1)
    lwz r30, 24(r1)
    lwz r29, 20(r1)
    lwz r28, 16(r1)
    addi r1, r1, 32
    blr
}

/* Huffman-decodes one blue-difference chroma block with the component 1 tables (paired-single stores). */
asm void THPDecodeBlockU(THPDecoder* state, s16* block)
{
    nofralloc
    stwu r1, -32(r1)
    stw r31, 28(r1)
    stw r30, 24(r1)
    stw r29, 20(r1)
    dcbz 0, r4
    lwz r12, 1700(r3)
    lwz r8, THPDcTable1(r0)
    cmpwi r12, 28
    lwz r11, 1696(r3)
    addi r5, r12, 4
    addi r10, r8, 32
    rlwnm r9, r11, r5, 27, 31
    bgt L_27a0
    lbzx r5, r8, r9
    lbzx r10, r10, r9
    cmpwi r5, 255
    beq L_2700
    add r12, r12, r10
    stw r12, 1700(r3)
    b L_2944
L_2700:
    addi r6, r8, 88
    li r5, 5
    addi r12, r12, 5
    nop
L_2710:
    cmpwi r12, 33
    slwi r9, r9, 1
    beq L_2730
    rlwnm r10, r11, r12, 31, 31
    lwzu r0, 4(r6)
    or r9, r9, r10
    addi r12, r12, 1
    b L_2774
L_2730:
    lwz r10, 1692(r3)
    li r12, 1
    lwzu r11, 4(r10)
    lwzu r0, 4(r6)
    rlwimi r9, r11, 1, 31, 31
    stw r10, 1692(r3)
    stw r11, 1696(r3)
    b L_2760
L_2750:
    slwi r9, r9, 1
    rlwnm r10, r11, r12, 31, 31
    lwzu r0, 4(r6)
    or r9, r9, r10
L_2760:
    cmpw r9, r0
    addi r12, r12, 1
    addi r5, r5, 1
    bgt L_2750
    b L_2780
L_2774:
    cmpw r9, r0
    addi r5, r5, 1
    bgt L_2710
L_2780:
    stw r12, 1700(r3)
    slwi r0, r5, 2
    add r5, r8, r0
    lwz r0, 64(r8)
    lwz r5, 140(r5)
    add r0, r0, r9
    lbzx r5, r5, r0
    b L_2944
L_27a0:
    cmpwi r12, 33
    lwz r9, 1692(r3)
    beq L_2858
    cmpwi r12, 32
    rlwnm r5, r11, r5, 27, 31
    beq L_27e0
    lbzx r9, r8, r5
    lbzx r10, r10, r5
    cmpwi r9, 255
    add r5, r12, r10
    beq L_28bc
    cmpwi r5, 33
    stw r5, 1700(r3)
    bgt L_28bc
    mr r5, r9
    b L_2944
L_27e0:
    lwzu r11, 4(r9)
    stw r9, 1692(r3)
    rlwimi r5, r11, 4, 28, 31
    lbzx r9, r8, r5
    lbzx r10, r10, r5
    cmpwi r9, 255
    stw r10, 1700(r3)
    stw r11, 1696(r3)
    beq L_280c
    mr r5, r9
    b L_2944
L_280c:
    slwi r9, r5, 27
    addi r6, r8, 88
    rlwimi r9, r11, 31, 1, 31
    li r12, 5
    nop
L_2820:
    subfic r11, r12, 31
    lwzu r0, 4(r6)
    srw r5, r9, r11
    addi r12, r12, 1
    cmpw r5, r0
    bgt L_2820
    stw r12, 1700(r3)
L_283c:
    slwi r0, r12, 2
    lwz r7, 64(r8)
    add r6, r8, r0
    lwz r6, 140(r6)
    add r0, r7, r5
    lbzx r5, r6, r0
    b L_2944
L_2858:
    lwzu r11, 4(r9)
    stw r9, 1692(r3)
    srwi r5, r11, 27
    lbzx r12, r8, r5
    lbzx r10, r10, r5
    cmpwi r12, 255
    stw r11, 1696(r3)
    addi r10, r10, 1
    beq L_2888
    stw r10, 1700(r3)
    mr r5, r12
    b L_2944
L_2888:
    li r12, 5
    li r6, 20
L_2890:
    subfic r9, r12, 31
    addi r6, r6, 4
    add r5, r8, r6
    addi r12, r12, 1
    lwz r0, 68(r5)
    srw r5, r11, r9
    cmpw cr1, r5, r0
    bgt cr1, L_2890
    addi r0, r12, 1
    stw r0, 1700(r3)
    b L_283c
L_28bc:
    subfic r0, r12, 33
    li r5, -1
    slw r7, r5, r0
    lwz r9, 1692(r3)
    andc r5, r11, r7
    addi r7, r8, 68
    subfic r6, r12, 33
    lwzu r11, 4(r9)
    addi r12, r6, 1
    slwi r6, r6, 2
    stw r11, 1696(r3)
    add r7, r7, r6
    slwi r5, r5, 1
    stw r9, 1692(r3)
    rlwimi r5, r11, 1, 31, 31
    li r9, 2
    lwzu r6, 4(r7)
    b L_291c
    nop
L_2908:
    slwi r5, r5, 1
    lwzu r6, 4(r7)
    add r5, r5, r10
    addi r9, r9, 1
    addi r12, r12, 1
L_291c:
    cmpw r5, r6
    rlwnm r10, r11, r9, 31, 31
    bgt L_2908
    stw r9, 1700(r3)
    slwi r0, r12, 2
    add r6, r8, r0
    lwz r0, 64(r8)
    lwz r6, 140(r6)
    add r0, r0, r5
    lbzx r5, r6, r0
L_2944:
    li r0, 32
    dcbz r4, r0
    li r0, 64
    li r7, 0
    dcbz r4, r0
    cmpwi cr1, r5, 0
    beq cr1, L_29e4
    lwz r10, 1700(r3)
    subfic r11, r10, 33
    lwz r7, 1696(r3)
    subfc. r12, r11, r5
    addi r29, r10, -1
    bgt L_2990
    add r0, r10, r5
    stw r0, 1700(r3)
    slw r10, r7, r29
    subfic r0, r5, 32
    srw r7, r10, r0
    b L_29bc
L_2990:
    slw r0, r7, r29
    lwz r10, 1692(r3)
    lwzu r7, 4(r10)
    addi r12, r12, 1
    stw r7, 1696(r3)
    srw r7, r7, r11
    stw r10, 1692(r3)
    add r0, r7, r0
    stw r12, 1700(r3)
    subfic r12, r5, 32
    srw r7, r0, r12
L_29bc:
    extsh r6, r7
    subfic r0, r5, 32
    cntlzw r6, r6
    cmpw cr1, r6, r0
    ble cr1, L_29e4
    li r0, -1
    slw r0, r0, r5
    add r5, r7, r0
    addi r0, r5, 1
    extsh r7, r0
L_29e4:
    li r0, 96
    dcbz r4, r0
    lis r8, THPZigzagTable@ha
    lha r0, 1674(r3)
    addi r8, r8, THPZigzagTable@l
    li r6, 1
    li r9, -1
    add r0, r0, r7
    sth r0, 1674(r3)
    sth r0, 0(r4)
    b L_2d2c
L_2a10:
    lwz r29, 1700(r3)
    lwz r11, THPAcTable1(r0)
    cmpwi r29, 28
    lwz r30, 1696(r3)
    addi r5, r29, 4
    addi r31, r11, 32
    rlwnm r12, r30, r5, 27, 31
    bgt L_2ae8
    lbzx r5, r11, r12
    lbzx r31, r31, r12
    cmpwi r5, 255
    beq L_2a4c
    add r29, r29, r31
    stw r29, 1700(r3)
    b L_2c84
L_2a4c:
    addi r7, r11, 88
    li r5, 5
    addi r29, r29, 5
L_2a58:
    cmpwi r29, 33
    slwi r12, r12, 1
    beq L_2a78
    rlwnm r31, r30, r29, 31, 31
    lwzu r0, 4(r7)
    or r12, r12, r31
    addi r29, r29, 1
    b L_2abc
L_2a78:
    lwz r31, 1692(r3)
    li r29, 1
    lwzu r30, 4(r31)
    lwzu r0, 4(r7)
    rlwimi r12, r30, 1, 31, 31
    stw r31, 1692(r3)
    stw r30, 1696(r3)
    b L_2aa8
L_2a98:
    slwi r12, r12, 1
    rlwnm r31, r30, r29, 31, 31
    lwzu r0, 4(r7)
    or r12, r12, r31
L_2aa8:
    cmpw r12, r0
    addi r29, r29, 1
    addi r5, r5, 1
    bgt L_2a98
    b L_2ac8
L_2abc:
    cmpw r12, r0
    addi r5, r5, 1
    bgt L_2a58
L_2ac8:
    stw r29, 1700(r3)
    slwi r0, r5, 2
    add r5, r11, r0
    lwz r0, 64(r11)
    lwz r5, 140(r5)
    add r0, r0, r12
    lbzx r5, r5, r0
    b L_2c84
L_2ae8:
    cmpwi r29, 33
    lwz r12, 1692(r3)
    beq L_2ba0
    cmpwi r29, 32
    rlwnm r5, r30, r5, 27, 31
    beq L_2b28
    lbzx r12, r11, r5
    lbzx r31, r31, r5
    cmpwi r12, 255
    add r5, r29, r31
    beq L_2c04
    cmpwi r5, 33
    stw r5, 1700(r3)
    bgt L_2c04
    mr r5, r12
    b L_2c84
L_2b28:
    lwzu r30, 4(r12)
    stw r12, 1692(r3)
    rlwimi r5, r30, 4, 28, 31
    lbzx r12, r11, r5
    lbzx r31, r31, r5
    cmpwi r12, 255
    stw r31, 1700(r3)
    stw r30, 1696(r3)
    beq L_2b54
    mr r5, r12
    b L_2c84
L_2b54:
    slwi r12, r5, 27
    addi r7, r11, 88
    rlwimi r12, r30, 31, 1, 31
    li r29, 5
    nop
L_2b68:
    subfic r30, r29, 31
    lwzu r0, 4(r7)
    srw r5, r12, r30
    addi r29, r29, 1
    cmpw r5, r0
    bgt L_2b68
    stw r29, 1700(r3)
L_2b84:
    slwi r0, r29, 2
    lwz r10, 64(r11)
    add r7, r11, r0
    lwz r7, 140(r7)
    add r0, r10, r5
    lbzx r5, r7, r0
    b L_2c84
L_2ba0:
    lwzu r30, 4(r12)
    stw r12, 1692(r3)
    srwi r5, r30, 27
    lbzx r29, r11, r5
    lbzx r31, r31, r5
    cmpwi r29, 255
    stw r30, 1696(r3)
    addi r31, r31, 1
    beq L_2bd0
    stw r31, 1700(r3)
    mr r5, r29
    b L_2c84
L_2bd0:
    li r29, 5
    li r7, 20
L_2bd8:
    subfic r12, r29, 31
    addi r7, r7, 4
    add r5, r11, r7
    addi r29, r29, 1
    lwz r0, 68(r5)
    srw r5, r30, r12
    cmpw cr1, r5, r0
    bgt cr1, L_2bd8
    addi r0, r29, 1
    stw r0, 1700(r3)
    b L_2b84
L_2c04:
    subfic r0, r29, 33
    lwz r12, 1692(r3)
    slw r10, r9, r0
    andc r5, r30, r10
    addi r10, r11, 68
    subfic r7, r29, 33
    lwzu r30, 4(r12)
    addi r29, r7, 1
    slwi r7, r7, 2
    stw r30, 1696(r3)
    add r10, r10, r7
    slwi r5, r5, 1
    stw r12, 1692(r3)
    rlwimi r5, r30, 1, 31, 31
    li r12, 2
    lwzu r7, 4(r10)
    b L_2c5c
L_2c48:
    slwi r5, r5, 1
    lwzu r7, 4(r10)
    add r5, r5, r31
    addi r12, r12, 1
    addi r29, r29, 1
L_2c5c:
    cmpw r5, r7
    rlwnm r31, r30, r12, 31, 31
    bgt L_2c48
    stw r12, 1700(r3)
    slwi r0, r29, 2
    add r7, r11, r0
    lwz r0, 64(r11)
    lwz r7, 140(r7)
    add r0, r0, r5
    lbzx r5, r7, r0
L_2c84:
    clrlwi. r30, r5, 28
    srawi r7, r5, 4
    beq L_2d1c
    lwz r10, 1700(r3)
    add r6, r6, r7
    subfic r11, r10, 33
    lwz r7, 1696(r3)
    subf. r12, r11, r30
    addi r29, r10, -1
    bgt L_2cc4
    add r0, r10, r30
    stw r0, 1700(r3)
    slw r10, r7, r29
    subfic r0, r30, 32
    srw r7, r10, r0
    b L_2cf0
L_2cc4:
    slw r0, r7, r29
    lwz r10, 1692(r3)
    lwzu r7, 4(r10)
    addi r12, r12, 1
    stw r7, 1696(r3)
    srw r7, r7, r11
    stw r10, 1692(r3)
    add r0, r7, r0
    stw r12, 1700(r3)
    subfic r12, r30, 32
    srw r7, r0, r12
L_2cf0:
    cntlzw r5, r7
    subfic r0, r30, 32
    cmpw cr1, r5, r0
    ble cr1, L_2d0c
    slw r0, r9, r30
    add r5, r0, r7
    addi r7, r5, 1
L_2d0c:
    lbzx r0, r8, r6
    slwi r0, r0, 1
    sthx r7, r4, r0
    b L_2d28
L_2d1c:
    cmpwi cr1, r7, 15
    bne cr1, L_2d34
    addi r6, r6, 15
L_2d28:
    addi r6, r6, 1
L_2d2c:
    cmpwi cr1, r6, 64
    blt cr1, L_2a10
L_2d34:
    lwz r31, 28(r1)
    lwz r30, 24(r1)
    lwz r29, 20(r1)
    addi r1, r1, 32
    blr
}

/* Huffman-decodes one red-difference chroma block with the component 2 tables (paired-single stores). */
asm void THPDecodeBlockV(THPDecoder* state, s16* block)
{
    nofralloc
    stwu r1, -32(r1)
    stw r31, 28(r1)
    stw r30, 24(r1)
    stw r29, 20(r1)
    dcbz 0, r4
    lwz r12, 1700(r3)
    lwz r8, THPDcTable2(r0)
    cmpwi r12, 28
    lwz r11, 1696(r3)
    addi r5, r12, 4
    addi r10, r8, 32
    rlwnm r9, r11, r5, 27, 31
    bgt L_2e40
    lbzx r5, r8, r9
    lbzx r10, r10, r9
    cmpwi r5, 255
    beq L_2da0
    add r12, r12, r10
    stw r12, 1700(r3)
    b L_2fe4
L_2da0:
    addi r6, r8, 88
    li r5, 5
    addi r12, r12, 5
    nop
L_2db0:
    cmpwi r12, 33
    slwi r9, r9, 1
    beq L_2dd0
    rlwnm r10, r11, r12, 31, 31
    lwzu r0, 4(r6)
    or r9, r9, r10
    addi r12, r12, 1
    b L_2e14
L_2dd0:
    lwz r10, 1692(r3)
    li r12, 1
    lwzu r11, 4(r10)
    lwzu r0, 4(r6)
    rlwimi r9, r11, 1, 31, 31
    stw r10, 1692(r3)
    stw r11, 1696(r3)
    b L_2e00
L_2df0:
    slwi r9, r9, 1
    rlwnm r10, r11, r12, 31, 31
    lwzu r0, 4(r6)
    or r9, r9, r10
L_2e00:
    cmpw r9, r0
    addi r12, r12, 1
    addi r5, r5, 1
    bgt L_2df0
    b L_2e20
L_2e14:
    cmpw r9, r0
    addi r5, r5, 1
    bgt L_2db0
L_2e20:
    stw r12, 1700(r3)
    slwi r0, r5, 2
    add r5, r8, r0
    lwz r0, 64(r8)
    lwz r5, 140(r5)
    add r0, r0, r9
    lbzx r5, r5, r0
    b L_2fe4
L_2e40:
    cmpwi r12, 33
    lwz r9, 1692(r3)
    beq L_2ef8
    cmpwi r12, 32
    rlwnm r5, r11, r5, 27, 31
    beq L_2e80
    lbzx r9, r8, r5
    lbzx r10, r10, r5
    cmpwi r9, 255
    add r5, r12, r10
    beq L_2f5c
    cmpwi r5, 33
    stw r5, 1700(r3)
    bgt L_2f5c
    mr r5, r9
    b L_2fe4
L_2e80:
    lwzu r11, 4(r9)
    stw r9, 1692(r3)
    rlwimi r5, r11, 4, 28, 31
    lbzx r9, r8, r5
    lbzx r10, r10, r5
    cmpwi r9, 255
    stw r10, 1700(r3)
    stw r11, 1696(r3)
    beq L_2eac
    mr r5, r9
    b L_2fe4
L_2eac:
    slwi r9, r5, 27
    addi r6, r8, 88
    rlwimi r9, r11, 31, 1, 31
    li r12, 5
    nop
L_2ec0:
    subfic r11, r12, 31
    lwzu r0, 4(r6)
    srw r5, r9, r11
    addi r12, r12, 1
    cmpw r5, r0
    bgt L_2ec0
    stw r12, 1700(r3)
L_2edc:
    slwi r0, r12, 2
    lwz r7, 64(r8)
    add r6, r8, r0
    lwz r6, 140(r6)
    add r0, r7, r5
    lbzx r5, r6, r0
    b L_2fe4
L_2ef8:
    lwzu r11, 4(r9)
    stw r9, 1692(r3)
    srwi r5, r11, 27
    lbzx r12, r8, r5
    lbzx r10, r10, r5
    cmpwi r12, 255
    stw r11, 1696(r3)
    addi r10, r10, 1
    beq L_2f28
    stw r10, 1700(r3)
    mr r5, r12
    b L_2fe4
L_2f28:
    li r12, 5
    li r6, 20
L_2f30:
    subfic r9, r12, 31
    addi r6, r6, 4
    add r5, r8, r6
    addi r12, r12, 1
    lwz r0, 68(r5)
    srw r5, r11, r9
    cmpw cr1, r5, r0
    bgt cr1, L_2f30
    addi r0, r12, 1
    stw r0, 1700(r3)
    b L_2edc
L_2f5c:
    subfic r0, r12, 33
    li r5, -1
    slw r7, r5, r0
    lwz r9, 1692(r3)
    andc r5, r11, r7
    addi r7, r8, 68
    subfic r6, r12, 33
    lwzu r11, 4(r9)
    addi r12, r6, 1
    slwi r6, r6, 2
    stw r11, 1696(r3)
    add r7, r7, r6
    slwi r5, r5, 1
    stw r9, 1692(r3)
    rlwimi r5, r11, 1, 31, 31
    li r9, 2
    lwzu r6, 4(r7)
    b L_2fbc
    nop
L_2fa8:
    slwi r5, r5, 1
    lwzu r6, 4(r7)
    add r5, r5, r10
    addi r9, r9, 1
    addi r12, r12, 1
L_2fbc:
    cmpw r5, r6
    rlwnm r10, r11, r9, 31, 31
    bgt L_2fa8
    stw r9, 1700(r3)
    slwi r0, r12, 2
    add r6, r8, r0
    lwz r0, 64(r8)
    lwz r6, 140(r6)
    add r0, r0, r5
    lbzx r5, r6, r0
L_2fe4:
    li r0, 32
    dcbz r4, r0
    li r0, 64
    li r7, 0
    dcbz r4, r0
    cmpwi cr1, r5, 0
    beq cr1, L_3084
    lwz r10, 1700(r3)
    subfic r11, r10, 33
    lwz r7, 1696(r3)
    subf. r12, r11, r5
    addi r29, r10, -1
    bgt L_3030
    add r0, r10, r5
    stw r0, 1700(r3)
    slw r10, r7, r29
    subfic r0, r5, 32
    srw r7, r10, r0
    b L_305c
L_3030:
    slw r0, r7, r29
    lwz r10, 1692(r3)
    lwzu r7, 4(r10)
    addi r12, r12, 1
    stw r7, 1696(r3)
    srw r7, r7, r11
    stw r10, 1692(r3)
    add r0, r7, r0
    stw r12, 1700(r3)
    subfic r12, r5, 32
    srw r7, r0, r12
L_305c:
    extsh r6, r7
    subfic r0, r5, 32
    cntlzw r6, r6
    cmpw cr1, r6, r0
    ble cr1, L_3084
    li r0, -1
    slw r0, r0, r5
    add r5, r7, r0
    addi r0, r5, 1
    extsh r7, r0
L_3084:
    li r0, 96
    dcbz r4, r0
    lis r8, THPZigzagTable@ha
    lha r0, 1680(r3)
    addi r8, r8, THPZigzagTable@l
    li r6, 1
    li r9, -1
    add r0, r0, r7
    sth r0, 1680(r3)
    sth r0, 0(r4)
    b L_33cc
L_30b0:
    lwz r29, 1700(r3)
    lwz r11, THPAcTable2(r0)
    cmpwi r29, 28
    lwz r30, 1696(r3)
    addi r5, r29, 4
    addi r31, r11, 32
    rlwnm r12, r30, r5, 27, 31
    bgt L_3188
    lbzx r5, r11, r12
    lbzx r31, r31, r12
    cmpwi r5, 255
    beq L_30ec
    add r29, r29, r31
    stw r29, 1700(r3)
    b L_3324
L_30ec:
    addi r7, r11, 88
    li r5, 5
    addi r29, r29, 5
L_30f8:
    cmpwi r29, 33
    slwi r12, r12, 1
    beq L_3118
    rlwnm r31, r30, r29, 31, 31
    lwzu r0, 4(r7)
    or r12, r12, r31
    addi r29, r29, 1
    b L_315c
L_3118:
    lwz r31, 1692(r3)
    li r29, 1
    lwzu r30, 4(r31)
    lwzu r0, 4(r7)
    rlwimi r12, r30, 1, 31, 31
    stw r31, 1692(r3)
    stw r30, 1696(r3)
    b L_3148
L_3138:
    slwi r12, r12, 1
    rlwnm r31, r30, r29, 31, 31
    lwzu r0, 4(r7)
    or r12, r12, r31
L_3148:
    cmpw r12, r0
    addi r29, r29, 1
    addi r5, r5, 1
    bgt L_3138
    b L_3168
L_315c:
    cmpw r12, r0
    addi r5, r5, 1
    bgt L_30f8
L_3168:
    stw r29, 1700(r3)
    slwi r0, r5, 2
    add r5, r11, r0
    lwz r0, 64(r11)
    lwz r5, 140(r5)
    add r0, r0, r12
    lbzx r5, r5, r0
    b L_3324
L_3188:
    cmpwi r29, 33
    lwz r12, 1692(r3)
    beq L_3240
    cmpwi r29, 32
    rlwnm r5, r30, r5, 27, 31
    beq L_31c8
    lbzx r12, r11, r5
    lbzx r31, r31, r5
    cmpwi r12, 255
    add r5, r29, r31
    beq L_32a4
    cmpwi r5, 33
    stw r5, 1700(r3)
    bgt L_32a4
    mr r5, r12
    b L_3324
L_31c8:
    lwzu r30, 4(r12)
    stw r12, 1692(r3)
    rlwimi r5, r30, 4, 28, 31
    lbzx r12, r11, r5
    lbzx r31, r31, r5
    cmpwi r12, 255
    stw r31, 1700(r3)
    stw r30, 1696(r3)
    beq L_31f4
    mr r5, r12
    b L_3324
L_31f4:
    slwi r12, r5, 27
    addi r7, r11, 88
    rlwimi r12, r30, 31, 1, 31
    li r29, 5
    nop
L_3208:
    subfic r30, r29, 31
    lwzu r0, 4(r7)
    srw r5, r12, r30
    addi r29, r29, 1
    cmpw r5, r0
    bgt L_3208
    stw r29, 1700(r3)
L_3224:
    slwi r0, r29, 2
    lwz r10, 64(r11)
    add r7, r11, r0
    lwz r7, 140(r7)
    add r0, r10, r5
    lbzx r5, r7, r0
    b L_3324
L_3240:
    lwzu r30, 4(r12)
    stw r12, 1692(r3)
    srwi r5, r30, 27
    lbzx r29, r11, r5
    lbzx r31, r31, r5
    cmpwi r29, 255
    stw r30, 1696(r3)
    addi r31, r31, 1
    beq L_3270
    stw r31, 1700(r3)
    mr r5, r29
    b L_3324
L_3270:
    li r29, 5
    li r7, 20
L_3278:
    subfic r12, r29, 31
    addi r7, r7, 4
    add r5, r11, r7
    addi r29, r29, 1
    lwz r0, 68(r5)
    srw r5, r30, r12
    cmpw cr1, r5, r0
    bgt cr1, L_3278
    addi r0, r29, 1
    stw r0, 1700(r3)
    b L_3224
L_32a4:
    subfic r0, r29, 33
    lwz r12, 1692(r3)
    slw r10, r9, r0
    andc r5, r30, r10
    addi r10, r11, 68
    subfic r7, r29, 33
    lwzu r30, 4(r12)
    addi r29, r7, 1
    slwi r7, r7, 2
    stw r30, 1696(r3)
    add r10, r10, r7
    slwi r5, r5, 1
    stw r12, 1692(r3)
    rlwimi r5, r30, 1, 31, 31
    li r12, 2
    lwzu r7, 4(r10)
    b L_32fc
L_32e8:
    slwi r5, r5, 1
    lwzu r7, 4(r10)
    add r5, r5, r31
    addi r12, r12, 1
    addi r29, r29, 1
L_32fc:
    cmpw r5, r7
    rlwnm r31, r30, r12, 31, 31
    bgt L_32e8
    stw r12, 1700(r3)
    slwi r0, r29, 2
    add r7, r11, r0
    lwz r0, 64(r11)
    lwz r7, 140(r7)
    add r0, r0, r5
    lbzx r5, r7, r0
L_3324:
    clrlwi. r30, r5, 28
    srawi r7, r5, 4
    beq L_33bc
    lwz r10, 1700(r3)
    add r6, r6, r7
    subfic r11, r10, 33
    lwz r7, 1696(r3)
    subf. r12, r11, r30
    addi r29, r10, -1
    bgt L_3364
    add r0, r10, r30
    stw r0, 1700(r3)
    slw r10, r7, r29
    subfic r0, r30, 32
    srw r7, r10, r0
    b L_3390
L_3364:
    slw r0, r7, r29
    lwz r10, 1692(r3)
    lwzu r7, 4(r10)
    addi r12, r12, 1
    stw r7, 1696(r3)
    srw r7, r7, r11
    stw r10, 1692(r3)
    add r0, r7, r0
    stw r12, 1700(r3)
    subfic r12, r30, 32
    srw r7, r0, r12
L_3390:
    cntlzw r5, r7
    subfic r0, r30, 32
    cmpw cr1, r5, r0
    ble cr1, L_33ac
    slw r0, r9, r30
    add r5, r0, r7
    addi r7, r5, 1
L_33ac:
    lbzx r0, r8, r6
    slwi r0, r0, 1
    sthx r7, r4, r0
    b L_33c8
L_33bc:
    cmpwi cr1, r7, 15
    bne cr1, L_33d4
    addi r6, r6, 15
L_33c8:
    addi r6, r6, 1
L_33cc:
    cmpwi cr1, r6, 64
    blt cr1, L_30b0
L_33d4:
    lwz r31, 28(r1)
    lwz r30, 24(r1)
    lwz r29, 20(r1)
    addi r1, r1, 32
    blr
}

s32 THPInit(void)
{
    OSRegisterVersion(THPVersion);
    THPPlane512[0] = 0xE0000000;
    THPPlane512[1] = 0xE0002000;
    THPPlane512[2] = 0xE0002800;
    THPPlane640[0] = 0xE0000000;
    THPPlane640[1] = 0xE0002A00;
    THPPlane640[2] = 0xE0003480;
    asm {
        li r3, 4
        oris r3, r3, 4
        mtspr GQR2, r3
        li r3, 5
        oris r3, r3, 5
        mtspr GQR3, r3
        li r3, 6
        oris r3, r3, 6
        mtspr GQR4, r3
        li r3, 7
        oris r3, r3, 7
        mtspr GQR5, r3
    }
    THPInitialized = 1;
    return 1;
}
