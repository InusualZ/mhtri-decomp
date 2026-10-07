/*
 * SC/SCProductInfo.c - the SC product information: `__SCF1` (the product info file reader) and the product area, code
 *    and serial getters.
 *
 * RANGE. .text 0x804DD010-0x804DD350 (5 functions, 0x340 B); .data 0x80629EB8-0x80629F18; .sdata
 *    0x80794110-0x80794138; .sbss 0x80795460-0x80795468.  Cut from the old SC block between `SC/SCApi.c`
 *    (0x804DD010) and `SEQ/seq.c` (0x804DD350).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `SCGetProductArea`, `SCGetProductCode`, `SCGetProductSN` and `__SCF1` are the map's names;
 *    `SCGetProductGameRegion` (0x804DD2C0, reads the GAME tag) and `ProductCode` (.sbss 0x80795460) are GUESSes;
 *    the file name `SCProductInfo.c` is a GUESS.
 * EVIDENCE. `__SCF1` is called only by the four product getters; `.sdata` 0x80794110..0x80794138 are their short
 *    string literals (AREA, CODE, SERNO, %u, GAME); `.data` 0x80629EB8 (the area table `JPN`, `USA`, `EUR`, ...) and
 *    0x80629F00 (the region table `JP`, `US`, `EU`, `KR`, `CN`) are read by `SCGetProductArea` and
 *    `SCGetProductGameRegion`; `.sbss` 0x80795460 (8 B) by `SCGetProductCode`.  `__SCF1` decodes the scrambled
 *    key=value text at 0x80003800 with a rotating key and returns the value of the tag.
 * RESIDUALS. `__SCF1` 0x804DD010 is 95.1 %: identical instruction stream, five loop registers numbered differently
 *    (target key r11 / found r9 / tag r12 / count r6, ours r8 / r12 / r9 / r11); `mt.py permdecl` over all 119
 *    declaration orders reaches 95.05 % at best (applied).  flipcheck: .text, and the .rodata/.sdata strings carry
 *    `@NN` names where the target has labels (relocation names only).
 * SHAPES. the tag scan and the value copy are two loops over the 256-byte block, the key rotated in place.
 */

#include "types.h"

#include "MSL_C/alloc.h"
#include "SC/SCProductInfo.h"

/* One product area row: the SC area code and its three-letter name. */
typedef struct SCAreaEntry {
    /* +0x00 */ s8 code;
    /* +0x01 */ char name[4];
} SCAreaEntry; /* size: 0x05 */

/* One product game-region row: the region code and its two-letter name. */
typedef struct SCRegionEntry {
    /* +0x00 */ s8 code;
    /* +0x01 */ char name[3];
} SCRegionEntry; /* size: 0x04 */

static SCAreaEntry AreaTable[] = {
    {0, "JPN"}, {1, "USA"}, {2, "EUR"}, {3, "AUS"}, {4, "BRA"}, {5, "TWN"}, {5, "ROC"},
    {6, "KOR"}, {7, "HKG"}, {8, "ASI"}, {9, "LTN"}, {10, "SAF"}, {11, "CHN"}, {-1, ""},
};

static SCRegionEntry RegionTable[] = {
    {0, "JP"}, {1, "US"}, {2, "EU"}, {4, "KR"}, {5, "CN"}, {-1, ""},
};

static char ProductCode[8];

/* The scrambled product-information text the boot loader leaves at 0x80003800. */
#define ProductInfoBlock ((u8*)0x80003800)

BOOL __SCF1(const char* tag, char* buf, u32 size)
{
    u32 n = 0;
    BOOL found = FALSE;
    u32 i = 0;
    u32 key = 0x73B5DBFA;
    u32 tagIndex = 0;

    for (i = 0; i < 256; i++) {
        u8 c = ProductInfoBlock[i];
        if (c != 0) {
            u8 d = c ^ key;
            if (tag[tagIndex] == 0 && d == '=') {
                found = TRUE;
                break;
            }
            tagIndex = ((d ^ tag[tagIndex]) & 0xDF) == 0 ? tagIndex + 1 : 0;
        }
        key = (key >> 31) | (key << 1);
    }

    if (found) {
        for (i++; i < 256 && n < size; i++) {
            u8 c = ProductInfoBlock[i];
            key = (key >> 31) | (key << 1);
            if (c != 0) {
                c = c ^ key;
                if (c == '\r' || c == '\n') {
                    c = 0;
                }
            }
            buf[n++] = c;
            if (c == 0) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

s8 SCGetProductArea(void)
{
    char area[4];
    s8 code;
    SCAreaEntry* entry = AreaTable;

    if (__SCF1("AREA", area, 4)) {
        for (; (code = entry->code) != -1; entry++) {
            if (strcmp(entry->name, area) == 0) {
                return code;
            }
        }
    }
    return -1;
}

const char* SCGetProductCode(void)
{
    if (__SCF1("CODE", ProductCode, 6)) {
        return ProductCode;
    }
    return NULL;
}

BOOL SCGetProductSN(u32* serial)
{
    char text[12];

    if (__SCF1("SERNO", text, 11) && sscanf(text, "%u", serial) == 1) {
        return TRUE;
    }
    return FALSE;
}

s8 SCGetProductGameRegion(void)
{
    char region[3];
    s8 code;
    SCRegionEntry* entry = RegionTable;

    if (__SCF1("GAME", region, 3)) {
        for (; (code = entry->code) != -1; entry++) {
            if (strcmp(entry->name, region) == 0) {
                return code;
            }
        }
    }
    return -1;
}
