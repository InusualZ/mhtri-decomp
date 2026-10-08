/*
 * THP/THPDec.h - the THP movie decoder's state record and the declarations of the symbols owned by `THP/THPDec.c`
 * that other units call.  Every field name is a GUESS read off how the decoder uses the offset (it is a baseline
 * JPEG decoder: quantisation tables, Huffman tables, three colour components and a bit reader).
 */
#ifndef THP_THPDEC_H
#define THP_THPDEC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0xE0 - one Huffman table: a 5-bit lookup for short codes, then per-length limits for the long ones. */
typedef struct THPHuffTable {
    /* +0x00 */ u8 fastSymbol[32]; /* the symbol of a 5-bit code prefix (0xFF = longer than 5 bits) */
    /* +0x20 */ u8 fastLength[32]; /* the code length behind a 5-bit prefix */
    /* +0x40 */ u8* symbols;
    /* +0x44 */ s32 maxCode[18];    /* the largest code of each length, indexed by length */
    /* +0x8C */ s32 valueOffset[17]; /* symbol index minus code, indexed by length */
    /* +0xD0 */ u8 pad_0xD0[0x10];
} THPHuffTable;

/* size: 0x6 - one colour component of the frame. */
typedef struct THPComponent {
    /* +0x0 */ u8 quantTable;
    /* +0x1 */ u8 dcTable;
    /* +0x2 */ u8 acTable;
    /* +0x3 */ u8 pad_0x03;
    /* +0x4 */ s16 dcPredictor;
} THPComponent;

/* size: 0x6BC - the decoder state, built in the caller's work buffer (32-byte aligned). */
typedef struct THPDecoder {
    /* +0x000 */ f32 quant[3][64]; /* quantisation tables scaled by the inverse-DCT factors */
    /* +0x300 */ THPHuffTable huffman[4]; /* DC then AC table of each of two table slots */
    /* +0x680 */ THPComponent component[3];
    /* +0x692 */ u16 width;
    /* +0x694 */ u16 height;
    /* +0x696 */ u16 mcuColumns; /* macroblocks per row */
    /* +0x698 */ u16 rowY;       /* the first pixel row still to decode */
    /* +0x69A */ u8 pad_0x69A[2];
    /* +0x69C */ u8* cursor;
    /* +0x6A0 */ u32 bitBuffer;
    /* +0x6A4 */ u32 bitPosition;
    /* +0x6A8 */ u8 tableMask; /* bit n set once Huffman table n was defined */
    /* +0x6A9 */ u8 restartEnabled;
    /* +0x6AA */ u16 restartInterval;
    /* +0x6AC */ u16 restartCounter;
    /* +0x6AE */ u8 pad_0x6AE[2];
    /* +0x6B0 */ u8* outputY;
    /* +0x6B4 */ u8* outputU;
    /* +0x6B8 */ u8* outputV;
} THPDecoder;

/* 0x804E1120 - decodes one frame from `data` into the three output planes using `work` as scratch; returns 0 or an
 * error code. */
u8 THPVideoDecode(u8* data, u8* planeY, u8* planeU, u8* planeV, u8* work);

/* 0x804E4510 - registers the library version, sets up the locked-cache plane addresses and the quantised load/store
 * registers; returns 1. */
s32 THPInit(void);

#ifdef __cplusplus
}
#endif

#endif
