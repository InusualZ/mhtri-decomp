/*
 * SC/SCSystemConfig.c - the SC (system configuration) library core: `SCInit`, the SYSCONF file reload and flush, the
 *    item table and the typed item readers.
 *
 * RANGE. .text 0x804DB050-0x804DC9E0 (21 functions, 0x1990 B); .rodata 0x80573B58-0x80573BB0; .data
 *    0x80629CA8-0x80629E88; .bss 0x8074E460-0x80756600; .sdata 0x80794000-0x80794110; .sbss
 *    0x80795448-0x80795460.  Cut from the old SC block; the left edge is `SCInit` (0x804DB050, after the RSO list
 *    functions), the right edge 0x804DC9E0 is the first item getter of `SC/SCApi.c`.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `SCInit`, `SCCheckStatus`, `SCReloadConfFileAsync`, `SCFlushAsync`, `SCFind*Item`, `SCReplaceByteArrayItem`
 *    and the *CallbackFromReload names are the map's; the file name `SCSystemConfig.c` is a GUESS.  The helpers
 *    `SCReloadNextFile`, `SCReloadNextFileCallback`, `SCParseConfBuffer`, `SCParseItem`, `SCDeleteItem`, `SCAddItem`,
 *    `SCFlushWakeup`, `SCFlushStep`, `SCReplaceU8Item` and the statics `ItemNames`, `ItemNumMax`, `ConfDirty`, `FlushBuf`,
 *    `SysDirName` are GUESSes from what each does; `NANDPrivateGetTypeAsync` (0x804C93A0) and
 *    `NANDPrivateCreateDirAsync` (0x804C7A90) are GUESS names for the two NAND leaves the flush machine calls.
 * EVIDENCE. `.rodata` 0x80573B58..0x80573BB0 holds `/shared2/sys`, `/shared2/sys/SYSCONF` and
 *    `/title/00000001/00000002/data/setting.txt`, read by `SCReloadConfFileAsync`, `SCFlushAsync` and the
 *    0x804DC6A0 flush; `.data` 0x80629CA8 is the build string `<< RVL_SDK - SC release build ... (0x4302_145)
 *    >>` (`__SCVersion`), followed by the `IPL.*` item-name strings and the item table 0x80629D38; `.bss`
 *    0x8074E460 (`Control`) and 0x8074E600 (`ConfBuf`, 0x4000 B, 32-aligned) and 0x80752600 (`FlushBuf`) are read here
 *    only; `.sbss` 0x80795448..0x80795460 is the job state.  The image is `SCv0`, a u16 item count, a u16 offset
 *    table, the items (type|name-length byte, name, value) and `SCed`; the item-id table is built downwards from
 *    the `SCed` marker.
 * RESIDUALS. 12 of 21 rows match the target.  `SCParseConfBuffer` 0x804DB6C0, `SCFlushStep` 0x804DC6A0,
 *    `SCAddItem` 0x804DBC30, `SCFlushAsync` 0x804DC480 and `SCDeleteItem` 0x804DBA90 differ in the
 *    shared failure exits (the target branches to one block from inside nested loops and from every NAND call,
 *    which needs `goto`), `SCAddItem`'s six-way `switch` (the target emits a binary search, the compiler emits a
 *    linear chain for the same cases) and register numbering; `SCFindByteArrayItem` and `SCCheckStatus` differ in
 *    register numbering only.  flipcheck: `.text` (the five rows above), the `@NN` relocation names of `SCv0`/`SCed`,
 *    and the tail padding of `.rodata`/`.sdata`/`.sbss`.
 * SHAPES. `NULL` is redefined as `((void*)0)` for the pointer compares that the target emits as `li`+`cmplw`; the
 *    item lookup and the add/replace paths run inside `do { ... } while (0)` so every failure shares one exit; the
 *    buffers are `__attribute__((aligned(32)))`; the flush state machine is one `switch` whose cases fall through
 *    into the shared error tail.
 */

#include "types.h"

#include "MSL/strlen.h"
#include "MSL_C/alloc.h"
#include "NAND/nand.h"
#include "OS/OS.h"
#include "OS/OSInitThreadQueue.h"
#include "OS/OSInterrupt.h"
#include "OS/OSThread.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "SC/SCSystemConfig.h"

#undef NULL
#define NULL ((void*)0)

#define SC_CONF_SIZE 0x4000
#define SC_PRODUCT_INFO_SIZE 0x100
#define SC_PRODUCT_INFO_ADDR 0x80003800

#define SC_STATUS_IDLE 0
#define SC_STATUS_BUSY 1
#define SC_STATUS_ERROR 2
#define SC_STATUS_RELOADED 3

#define SC_ITEM_MAX 37

#define SC_TYPE_BIGARRAY 0x20
#define SC_TYPE_ARRAY 0x40
#define SC_TYPE_U8 0x60
#define SC_TYPE_U16 0x80
#define SC_TYPE_U32 0xA0
#define SC_TYPE_U64 0xC0
#define SC_TYPE_BOOL 0xE0

/* One entry of the item-name table: the item's name in the configuration file and its identifier. */
typedef struct SCItemName {
    /* +0x00 */ const char* name;
    /* +0x04 */ s32 id;
} SCItemName; /* size: 0x08 */

/* One parsed configuration item. */
typedef struct SCItem {
    /* +0x00 */ u8 value[8]; /* a scalar item's value, copied out of the file */
    /* +0x08 */ u8 type;     /* the scalar type (0 for an array) */
    /* +0x09 */ u8 isArray;
    /* +0x0A */ u8 pad_0x0A[2];
    /* +0x0C */ u32 nameLength;
    /* +0x10 */ u32 dataLength;
    /* +0x14 */ const u8* name;
    /* +0x18 */ const u8* data;
    /* +0x1C */ u32 totalLength; /* the item's encoded size in bytes */
} SCItem; /* size: 0x20 */

/* The 8-byte NAND answer area: the node type of a type probe or the status record of a status query. */
typedef union SCNandScratch {
    /* +0x00 */ u8 type;
    /* +0x00 */ NANDStatus status;
} SCNandScratch; /* size: 0x08 */

/* The job state of the reload and flush machines. */
typedef struct SCControl {
    /* +0x000 */ OSThreadQueue queue;
    /* +0x008 */ NANDFileInfo fileInfo;
    /* +0x094 */ u8 commandBlock[0xBC]; /* the NANDCommandBlock storage (the type is opaque here) */
    /* +0x150 */ SCNandScratch scratch;
    /* +0x158 */ u8 step;
    /* +0x159 */ u8 fileOpen;
    /* +0x15A */ u8 fileIndex;
    /* +0x15B */ u8 pad_0x15B;
    /* +0x15C */ void (*reloadCallback)(s32 result);
    /* +0x160 */ s32 reloadResult;
    /* +0x164 */ const char* fileName[2];
    /* +0x16C */ u8* buffer[2];
    /* +0x174 */ u32 bufferSize[2];
    /* +0x17C */ u32 readSize[2];
    /* +0x184 */ void (*flushCallback)(s32 result);
    /* +0x188 */ s32 flushResult;
    /* +0x18C */ u32 flushSize;
} SCControl; /* size: 0x190 */

/* The head of the configuration image: the "SCv0" magic, the item count and the table of item offsets. */
typedef struct SCConfHeader {
    /* +0x00 */ char magic[4];
    /* +0x04 */ u16 itemCount;
    /* +0x06 */ u16 offsets[1]; /* itemCount + 1 entries; the last is the end of the item data */
} SCConfHeader; /* size: 0x08 */

#define CONF_HEADER(buf) ((SCConfHeader*)(buf))

#define COMMAND_BLOCK() ((NANDCommandBlock*)Control.commandBlock)

const char* __SCVersion = "<< RVL_SDK - SC \trelease build: Feb 27 2009 10:05:17 (0x4302_145) >>";

static const char SysDirName[] = "/shared2/sys";
static const char ConfFileName[] = "/shared2/sys/SYSCONF";
static const char ProductInfoFileName[] = "/title/00000001/00000002/data/setting.txt";

static SCItemName ItemNames[SC_ITEM_MAX] = {
    {"IPL.CB", 0},   {"IPL.AR", 1},   {"IPL.ARN", 2},  {"IPL.CD", 3},   {"IPL.CD2", 4},  {"IPL.DH", 5},
    {"IPL.E60", 6},  {"IPL.EULA", 7}, {"IPL.FRC", 8},  {"IPL.IDL", 9},  {"IPL.INC", 10}, {"IPL.LNG", 11},
    {"IPL.NIK", 12}, {"IPL.PC", 13},  {"IPL.PGS", 14}, {"IPL.SSV", 15}, {"IPL.SADR", 16}, {"IPL.SND", 17},
    {"IPL.UPT", 18}, {"NET.CNF", 19}, {"NET.CTPC", 20}, {"NET.PROF", 21}, {"NET.WCPC", 22}, {"NET.WCFG", 23},
    {"DEV.BTM", 24}, {"DEV.VIM", 25}, {"DEV.CTC", 26}, {"DEV.DSM", 27}, {"BT.DINF", 28}, {"BT.CDIF", 29},
    {"BT.SENS", 30}, {"BT.SPKV", 31}, {"BT.MOT", 32},  {"BT.BAR", 33},  {"DVD.CNF", 34}, {"WWW.RST", 35},
    {"IPL.TID", 36},
};

static u8 Initialized;
static u8 ConfDirty;
static u8 IsDevKit;
static u32 ItemIDOffsetTblOffset;
static u32 ItemNumMax;
static u32 ItemNumTotal;
static u32 ItemRestSize;
static u8 BgJobStatus;

SCControl Control;
u8 ConfBuf[SC_CONF_SIZE] __attribute__((aligned(32)));
static u8 FlushBuf[SC_CONF_SIZE] __attribute__((aligned(32)));

static void OpenCallbackFromReload(s32 result, NANDCommandBlock* block);
static void ReadCallbackFromReload(s32 result, NANDCommandBlock* block);
static void CloseCallbackFromReload(s32 result, NANDCommandBlock* block);
static void SCReloadNextFile(void);
static void SCReloadNextFileCallback(s32 result, NANDCommandBlock* block);
static u32 SCParseConfBuffer(u8* buf, u32 size);
static BOOL SCParseItem(const u8* p, SCItem* item);
static void SCDeleteItem(u32 id);
/* untyped: the byte range of an item */
static BOOL SCAddItem(u32 id, s32 type, const void* value, u32 size);
static void SCFlushWakeup(s32 result);
static void SCFlushStep(s32 result, NANDCommandBlock* block);

/* Starts the SC reader: hooks the version string, probes the console type and begins the first configuration reload. */
void SCInit(void)
{
    BOOL enabled;

    enabled = OSDisableInterrupts();
    if (Initialized) {
        OSRestoreInterrupts(enabled);
        return;
    }
    Initialized = TRUE;
    BgJobStatus = SC_STATUS_BUSY;
    OSRestoreInterrupts(enabled);

    OSRegisterVersion(__SCVersion);
    OSInitThreadQueue(&Control.queue);
    if (OSGetConsoleType() & 0x10000000) {
        IsDevKit = TRUE;
    }
    if (NANDInit() != 0 || SCReloadConfFileAsync(ConfBuf, SC_CONF_SIZE, NULL) != 0) {
        BgJobStatus = SC_STATUS_ERROR;
    }
}

u32 SCCheckStatus(void)
{
    BOOL enabled;
    u32 status;
    u8* buf;
    u32 size;

    enabled = OSDisableInterrupts();
    status = BgJobStatus;
    if (status == SC_STATUS_RELOADED) {
        BgJobStatus = SC_STATUS_BUSY;
        OSRestoreInterrupts(enabled);
        if (SCParseConfBuffer(Control.buffer[0], Control.readSize[0]) == 0) {
            enabled = OSDisableInterrupts();
            if (ConfBuf != Control.buffer[0]) {
                memcpy(ConfBuf, Control.buffer[0], SC_CONF_SIZE);
            }
            ConfDirty = FALSE;
            OSRestoreInterrupts(enabled);
        } else {
            enabled = OSDisableInterrupts();
            buf = Control.buffer[0];
            size = SC_CONF_SIZE;
            memset(buf, 0, size);
            if (size > 12) {
                memcpy(buf, "SCv0", 4);
                memcpy(buf + SC_CONF_SIZE - 4, "SCed", 4);
                CONF_HEADER(buf)->offsets[0] = 8;
            }
            ConfDirty = FALSE;
            OSRestoreInterrupts(enabled);
        }
        status = 0;
        BgJobStatus = status;
    } else {
        OSRestoreInterrupts(enabled);
    }
    return status;
}

s32 SCReloadConfFileAsync(u8* buf, u32 size, void (*callback)(s32 result))
{
    if (size < SC_CONF_SIZE) {
        return -128;
    }
    Control.reloadCallback = callback;
    BgJobStatus = SC_STATUS_BUSY;
    Control.reloadResult = 0;
    Control.fileIndex = 0;
    Control.readSize[0] = 0;
    Control.readSize[1] = 0;
    Control.fileName[0] = ConfFileName;
    Control.fileName[1] = ProductInfoFileName;
    Control.buffer[0] = buf;
    Control.buffer[1] = (u8*)SC_PRODUCT_INFO_ADDR;
    Control.bufferSize[0] = SC_CONF_SIZE;
    Control.bufferSize[1] = SC_PRODUCT_INFO_SIZE;
    memset(buf, 0, SC_CONF_SIZE);
    memcpy(buf, "SCv0", 4);
    memcpy(buf + SC_CONF_SIZE - 4, "SCed", 4);
    CONF_HEADER(buf)->offsets[0] = 8;
    Control.fileOpen = FALSE;
    ItemIDOffsetTblOffset = 0;
    ItemNumTotal = 0;
    ItemRestSize = 0;
    return NANDPrivateOpenAsync(Control.fileName[Control.fileIndex], &Control.fileInfo, 1, OpenCallbackFromReload,
                                COMMAND_BLOCK());
}

static void OpenCallbackFromReload(s32 result, NANDCommandBlock* block)
{
    if (result == 0) {
        Control.fileOpen = TRUE;
        if (NANDReadAsync(&Control.fileInfo, Control.buffer[Control.fileIndex], Control.bufferSize[Control.fileIndex],
                          ReadCallbackFromReload, COMMAND_BLOCK()) == 0) {
            return;
        }
    }
    if (Control.fileIndex == 0) {
        Control.reloadResult = result;
    }
    Control.readSize[Control.fileIndex] = 0;
    if (Control.fileOpen) {
        if (NANDCloseAsync(&Control.fileInfo, SCReloadNextFileCallback, COMMAND_BLOCK()) == 0) {
            return;
        }
    }
    SCReloadNextFile();
}

static void ReadCallbackFromReload(s32 result, NANDCommandBlock* block)
{
    if ((u32)result == Control.bufferSize[Control.fileIndex]) {
        Control.readSize[Control.fileIndex] = result;
        Control.fileOpen = FALSE;
        if (NANDCloseAsync(&Control.fileInfo, CloseCallbackFromReload, COMMAND_BLOCK()) == 0) {
            return;
        }
    }
    if (Control.fileIndex == 0) {
        Control.reloadResult = result != 0 ? result : -128;
    }
    Control.readSize[Control.fileIndex] = 0;
    if (Control.fileOpen) {
        if (NANDCloseAsync(&Control.fileInfo, SCReloadNextFileCallback, COMMAND_BLOCK()) == 0) {
            return;
        }
    }
    SCReloadNextFile();
}

static void CloseCallbackFromReload(s32 result, NANDCommandBlock* block)
{
    if (result == 0) {
        SCReloadNextFile();
    } else {
        if (Control.fileIndex == 0) {
            Control.reloadResult = result;
        }
        Control.readSize[Control.fileIndex] = 0;
        if (Control.fileOpen) {
            if (NANDCloseAsync(&Control.fileInfo, SCReloadNextFileCallback, COMMAND_BLOCK()) == 0) {
                return;
            }
        }
        SCReloadNextFile();
    }
}

/* Opens the next file of the reload, or finishes the reload once every file has been read. */
static void SCReloadNextFile(void)
{
    u8* buf;
    u32 status;

    for (;;) {
        Control.fileIndex++;
        if (Control.fileIndex >= 2) {
            break;
        }
        Control.fileOpen = FALSE;
        if (NANDPrivateOpenAsync(Control.fileName[Control.fileIndex], &Control.fileInfo, 1, OpenCallbackFromReload,
                                 COMMAND_BLOCK()) == 0) {
            return;
        }
    }
    if (Control.reloadResult == 0) {
        status = SC_STATUS_RELOADED;
    } else {
        buf = Control.buffer[0];
        memset(buf, 0, SC_CONF_SIZE);
        memcpy(buf, "SCv0", 4);
        memcpy(buf + SC_CONF_SIZE - 4, "SCed", 4);
        CONF_HEADER(buf)->offsets[0] = 8;
        status = SC_STATUS_RELOADED;
        Control.readSize[0] = Control.bufferSize[0];
    }
    *(u8*)(SC_PRODUCT_INFO_ADDR + SC_PRODUCT_INFO_SIZE - 1) = 0;
    if (Control.reloadCallback) {
        Control.reloadCallback(Control.reloadResult);
        Control.reloadCallback = NULL;
    }
    BgJobStatus = status;
}

static void SCReloadNextFileCallback(s32 result, NANDCommandBlock* block)
{
    SCReloadNextFile();
}

/* Validates the configuration image in `buf` (`size` bytes) and builds the item-id table behind its offset table;
 * returns 0 on success and 2 when the image is malformed. */
static u32 SCParseConfBuffer(u8* buf, u32 size)
{
    u8* end;
    u16* offsets;
    u16* idTable;
    u16* tail;
    const SCItemName* entry;
    const SCItemName* entryEnd;
    SCItem item;
    u32 count;
    u32 offset;
    u32 i;
    u32 nameLength;
    u32 pad;
    u8* tableStart;
    u16* cursor;

    if (size - 12 > 0x3FF4) {
        return 2;
    }
    ItemNumMax = SC_ITEM_MAX;
    end = buf + size - 4;
    if (memcmp(buf, "SCv0", 4) != 0) {
        return 2;
    }
    if (memcmp(end, "SCed", 4) != 0) {
        return 2;
    }
    if (size < SC_CONF_SIZE) {
        pad = SC_CONF_SIZE - size;
        memset(end, 0, pad);
        end += pad;
        memcpy(end, "SCed", 4);
    }
    if (buf + 6 > end) {
        return 2;
    }
    count = (buf[4] << 8) | buf[5];
    offsets = CONF_HEADER(buf)->offsets;
    offset = (u8*)(offsets + count + 1) - buf;
    cursor = offsets;
    for (i = 0; i < count; i++) {
        if (offset > size) {
            return 2;
        }
        if ((u8*)cursor - buf > size) {
            return 2;
        }
        if (*cursor != offset) {
            return 2;
        }
        if (!SCParseItem(buf + offset, &item)) {
            return 2;
        }
        cursor++;
        offset += item.totalLength;
    }
    if (offset > size) {
        return 2;
    }
    if (offset != offsets[i]) {
        return 2;
    }
    tableStart = end - 72;
    if (buf + offset > tableStart) {
        return 2;
    }
    offset = tableStart - (buf + offset);
    memset(tableStart, 0, end - tableStart);
    tail = (u16*)end;
    idTable = tail - 1;
    entry = ItemNames;
    entryEnd = ItemNames + ItemNumMax;
    for (; entry < entryEnd && entry->name != NULL; entry++) {
        nameLength = strlen(entry->name);
        cursor = offsets;
        for (i = 0; i < count; i++) {
            const u8* header = buf + *cursor;
            if (nameLength == (header[0] & 0x1F) + 1u && memcmp(entry->name, header + 1, nameLength) == 0) {
                idTable[-entry->id] = (u8*)(offsets + i) - buf;
                break;
            }
            cursor++;
        }
    }
    ItemIDOffsetTblOffset = (u8*)idTable - buf;
    ItemNumTotal = count;
    ItemRestSize = offset;
    return 0;
}

/* Decodes the item at `p` into `item`; returns whether it has a known type. */
static BOOL SCParseItem(const u8* p, SCItem* item)
{
    s32 type;
    u32 length;

    memset(item, 0, sizeof(SCItem));
    type = p[0] & 0xE0;
    item->name = p + 1;
    item->nameLength = (p[0] & 0x1F) + 1;
    item->data = p + item->nameLength + 1;
    do {
        switch (type) {
        case SC_TYPE_U8:
        case SC_TYPE_BOOL:
            item->dataLength = 1;
            break;
        case SC_TYPE_U16:
            item->dataLength = 2;
            break;
        case SC_TYPE_U32:
            item->dataLength = 4;
            break;
        case SC_TYPE_U64:
            item->dataLength = 8;
            break;
        case SC_TYPE_ARRAY:
            item->dataLength = item->data[0] + 1;
            item->data += 1;
            item->totalLength += 1;
            break;
        case SC_TYPE_BIGARRAY:
            length = (item->data[0] << 8) | item->data[1];
            item->dataLength = length + 1;
            item->data += 2;
            item->totalLength += 2;
            break;
        default:
            continue;
        }
        if ((u32)type == SC_TYPE_ARRAY || (u32)type == SC_TYPE_BIGARRAY) {
            item->isArray = SC_TYPE_ARRAY;
        } else {
            item->type = type;
            memcpy(item->value, item->data, item->dataLength);
        }
        item->totalLength = item->nameLength + item->dataLength + item->totalLength + 1;
    } while (0);
    return item->dataLength != 0;
}

/* Looks up the item `id` and decodes it into `item`. */
static inline BOOL FindItem(u32 id, SCItem* item)
{
    u16* idTable;
    u32 position;
    u8* conf = ConfBuf;

    if (id < ItemNumMax && ItemIDOffsetTblOffset != 0) {
        idTable = (u16*)(conf + ItemIDOffsetTblOffset);
        position = idTable[-id];
        if (position != 0) {
            return SCParseItem(conf + *(u16*)(conf + position), item);
        }
    }
    return FALSE;
}

/* Removes the item `id` from the image, closing the gap in the offset table, the data and the id table. */
static void SCDeleteItem(u32 id)
{
    u8* conf = ConfBuf;
    u16* idTable;
    u16* offsets;
    u16* last;
    u16* entry;
    u16* p;
    u32 position;
    u32 itemOffset;
    u32 itemSize;
    u32 end;
    u32 i;

    if (id < ItemNumMax && ItemIDOffsetTblOffset != 0) {
        idTable = (u16*)(conf + ItemIDOffsetTblOffset);
        position = idTable[-id];
        if (position != 0 && ItemNumTotal != 0) {
            entry = (u16*)(conf + position);
            offsets = CONF_HEADER(conf)->offsets;
            last = offsets + ItemNumTotal;
            itemOffset = *(u16*)(conf + position);
            itemSize = entry[1] - itemOffset + 2;
            end = *last;
            memmove(entry, entry + 1, itemOffset - (position + 2));
            for (p = last - 1; p >= offsets; p--) {
                if (p < entry) {
                    *p -= 2;
                } else {
                    *p -= itemSize;
                }
            }
            memmove(conf + *entry, conf + *entry + itemSize, end - (*entry + itemSize));
            memset(conf + end - itemSize, 0, itemSize);
            for (i = 0; i < ItemNumMax; i++) {
                u32 v = idTable[-i];
                if (v >= position) {
                    if (v > position) {
                        idTable[-i] = v - 2;
                    } else {
                        idTable[-i] = 0;
                    }
                }
            }
            ItemRestSize += itemSize;
            ItemNumTotal -= 1;
            CONF_HEADER(conf)->itemCount = ItemNumTotal;
            ConfDirty = TRUE;
        }
    }
}

/* Appends the item `id` with the given type and value; returns whether it fitted. */
/* untyped: the byte range of an item */
static BOOL SCAddItem(u32 id, s32 type, const void* value, u32 size)
{
    u32 total = 1;
    const SCItemName* entry = ItemNames;
    u8* conf = ConfBuf;
    const char* name;
    u32 nameLength;
    u16* offsets;
    u16* last;
    u16* p;
    u8* dst;
    u32 count;
    u32 first;

    do {
        if (id >= ItemNumMax || value == NULL || ItemNumTotal >= 0xFFFF || ItemIDOffsetTblOffset == 0) {
            break;
        }
        switch (type) {
        case SC_TYPE_U8:
        case SC_TYPE_BOOL:
            size = 1;
            break;
        case SC_TYPE_U16:
            size = 2;
            break;
        case SC_TYPE_U32:
            size = 4;
            break;
        case SC_TYPE_U64:
            size = 8;
            break;
        case SC_TYPE_ARRAY:
            if (size == 0 || size > 0x10000) {
                continue;
            }
            if (size > 256) {
                type = SC_TYPE_BIGARRAY;
                total = 3;
            } else {
                total = 2;
            }
            break;
        default:
            continue;
        }
        total += size;
        for (;;) {
            name = entry->name;
            if (name == NULL || entry->id == id) {
                break;
            }
            entry++;
        }
        if (name == NULL) {
            break;
        }
        nameLength = strlen(name);
        if (nameLength > 32) {
            break;
        }
        total += nameLength;
        if (ItemRestSize < total + 2) {
            break;
        }
        offsets = CONF_HEADER(conf)->offsets;
        first = *offsets;
        last = offsets + ItemNumTotal;
        memmove(conf + first + 2, conf + first, *last - first);
        p = offsets;
        do {
            *p += 2;
            p++;
        } while (p <= offsets + ItemNumTotal);
        dst = conf + *last;
        *dst = type | (nameLength - 1);
        memcpy(dst + 1, name, nameLength);
        dst += nameLength + 1;
        if (type == SC_TYPE_ARRAY) {
            *dst = size - 1;
            dst += 1;
        } else if (type == SC_TYPE_BIGARRAY) {
            dst[0] = (size - 1) >> 8;
            dst[1] = size - 1;
            dst += 2;
        }
        memcpy(dst, value, size);
        ((u16*)(conf + ItemIDOffsetTblOffset))[-id] = (u8*)last - conf;
        count = ItemNumTotal + 1;
        ItemRestSize -= total + 2;
        last[1] = *last + total;
        ItemNumTotal = count;
        CONF_HEADER(conf)->itemCount = count;
        ConfDirty = TRUE;
        return TRUE;
    } while (0);
    return FALSE;
}

/* untyped: the byte range of an item */
BOOL SCFindByteArrayItem(void* value, u32 id, u32 size)
{
    BOOL enabled;
    BOOL found = FALSE;
    SCItem item;

    enabled = OSDisableInterrupts();
    if (value != NULL) {
        if (FindItem(id, &item) && item.isArray && item.dataLength == size) {
            memcpy(value, item.data, size);
            found = TRUE;
        }
    }
    OSRestoreInterrupts(enabled);
    return found;
}

/* untyped: the byte range of an item */
BOOL SCReplaceByteArrayItem(const void* value, u32 id, u32 size)
{
    BOOL result = FALSE;
    BOOL enabled;
    SCItem item;

    enabled = OSDisableInterrupts();
    if (value != NULL) {
        do {
            if (FindItem(id, &item)) {
                if (item.isArray && item.dataLength == size) {
                    if (memcmp(item.data, value, size) != 0) {
                        memcpy((u8*)item.data, value, size);
                        ConfDirty = TRUE;
                    }
                    result = TRUE;
                    continue;
                }
                SCDeleteItem(id);
            }
            result = SCAddItem(id, SC_TYPE_ARRAY, value, size);
        } while (0);
    }
    OSRestoreInterrupts(enabled);
    return result;
}

BOOL SCFindU8Item(u8* value, u32 id)
{
    BOOL enabled;
    BOOL found = FALSE;
    SCItem item;

    enabled = OSDisableInterrupts();
    if (FindItem(id, &item) && item.type == SC_TYPE_U8) {
        memcpy(value, item.data, item.dataLength);
        found = TRUE;
    }
    OSRestoreInterrupts(enabled);
    return found;
}

BOOL SCFindS8Item(s8* value, u32 id)
{
    BOOL enabled;
    BOOL found = FALSE;
    SCItem item;

    enabled = OSDisableInterrupts();
    if (FindItem(id, &item) && item.type == SC_TYPE_U8) {
        memcpy(value, item.data, item.dataLength);
        found = TRUE;
    }
    OSRestoreInterrupts(enabled);
    return found;
}

BOOL SCFindU32Item(u32* value, u32 id)
{
    BOOL enabled;
    BOOL found = FALSE;
    SCItem item;

    enabled = OSDisableInterrupts();
    if (FindItem(id, &item) && item.type == SC_TYPE_U32) {
        memcpy(value, item.data, item.dataLength);
        found = TRUE;
    }
    OSRestoreInterrupts(enabled);
    return found;
}

BOOL SCReplaceU8Item(u32 value, u32 id)
{
    BOOL enabled;
    BOOL result;
    SCItem item;
    u8 v = value;

    enabled = OSDisableInterrupts();
    do {
        if (FindItem(id, &item)) {
            if (item.type == SC_TYPE_U8) {
                if (memcmp(item.data, &v, item.dataLength) != 0) {
                    memcpy((u8*)item.data, &v, item.dataLength);
                    ConfDirty = TRUE;
                }
                result = TRUE;
                continue;
            }
            SCDeleteItem(id);
        }
        result = SCAddItem(id, SC_TYPE_U8, &v, 0);
    } while (0);
    OSRestoreInterrupts(enabled);
    return result;
}

/* Wakes the threads waiting for the flush to finish. */
static void SCFlushWakeup(s32 result)
{
    OSWakeupThread(&Control.queue);
}

/* Ends the flush job: runs the caller's callback with the result, wakes the waiting threads and publishes the status. */
static inline void FinishFlush(void)
{
    void (*callback)(s32 result);

    if (Control.flushResult != 0) {
        ConfDirty = TRUE;
    }
    callback = Control.flushCallback;
    if (callback != 0) {
        Control.flushCallback = NULL;
        callback(Control.flushResult);
        if (Control.queue.head != NULL) {
            OSWakeupThread(&Control.queue);
        }
    }
    BgJobStatus = Control.flushResult;
}

void SCFlushAsync(void (*callback)(s32 result))
{
    BOOL enabled;
    BOOL dirty;

    enabled = OSDisableInterrupts();
    if (BgJobStatus == SC_STATUS_IDLE) {
        BgJobStatus = SC_STATUS_BUSY;
        if (callback == NULL) {
            callback = SCFlushWakeup;
        }
        Control.flushCallback = callback;
        Control.flushResult = 0;
        Control.fileOpen = FALSE;
        Control.flushSize = SC_CONF_SIZE;
        dirty = ConfDirty ? TRUE : FALSE;
        if (!dirty) {
            OSRestoreInterrupts(enabled);
            FinishFlush();
        } else {
            ConfDirty = FALSE;
            memcpy(FlushBuf, ConfBuf, SC_CONF_SIZE);
            OSRestoreInterrupts(enabled);
            Control.step = 0;
            if (NANDPrivateGetTypeAsync(ConfFileName, &Control.scratch.type, SCFlushStep, COMMAND_BLOCK()) != 0) {
                Control.flushResult = SC_STATUS_ERROR;
                if (Control.fileOpen) {
                    Control.step = 9;
                    if (NANDCloseAsync(&Control.fileInfo, SCFlushStep, COMMAND_BLOCK()) == 0) {
                        return;
                    }
                }
                FinishFlush();
            }
        }
    } else {
        if (callback != NULL) {
            callback(BgJobStatus == SC_STATUS_BUSY ? SC_STATUS_BUSY : SC_STATUS_ERROR);
        }
        OSRestoreInterrupts(enabled);
    }
}

/* Advances the flush job: each step runs once the NAND operation the previous step started has completed. */
static void SCFlushStep(s32 result, NANDCommandBlock* block)
{
    switch (Control.step) {
    case 0:
        if (result == 0 && Control.scratch.type == 1) {
            Control.step = 1;
            if (NANDPrivateGetStatusAsync(ConfFileName, &Control.scratch.status, SCFlushStep, COMMAND_BLOCK()) == 0) {
                return;
            }
            break;
        }
        Control.step = 2;
        if (NANDPrivateDeleteAsync(ConfFileName, SCFlushStep, COMMAND_BLOCK()) == 0) {
            return;
        }
        break;
    case 1:
        if (result == 0 && Control.scratch.status.permission == 0x3F) {
            Control.step = 6;
            if (NANDPrivateOpenAsync(ConfFileName, &Control.fileInfo, 2, SCFlushStep, COMMAND_BLOCK()) == 0) {
                return;
            }
            break;
        }
        Control.step = 2;
        if (NANDPrivateDeleteAsync(ConfFileName, SCFlushStep, COMMAND_BLOCK()) == 0) {
            return;
        }
        break;
    case 2:
        Control.step = 3;
        if (NANDPrivateGetTypeAsync(SysDirName, &Control.scratch.type, SCFlushStep, COMMAND_BLOCK()) == 0) {
            return;
        }
        break;
    case 3:
        if (result != 0 || Control.scratch.type != 2) {
            Control.step = 4;
            if (NANDPrivateCreateDirAsync(SysDirName, 0x3F, 0, SCFlushStep, COMMAND_BLOCK()) == 0) {
                return;
            }
            break;
        }
        /* fall through */
    case 4:
        Control.step = 5;
        if (NANDPrivateCreateAsync(ConfFileName, 0x3F, 0, SCFlushStep, COMMAND_BLOCK()) == 0) {
            return;
        }
        break;
    case 5:
        Control.step = 6;
        if (NANDPrivateOpenAsync(ConfFileName, &Control.fileInfo, 2, SCFlushStep, COMMAND_BLOCK()) == 0) {
            return;
        }
        break;
    case 6:
        if (result != 0) {
            break;
        }
        Control.fileOpen = TRUE;
        Control.step = 7;
        if (NANDWriteAsync(&Control.fileInfo, FlushBuf, Control.flushSize, SCFlushStep, COMMAND_BLOCK()) == 0) {
            return;
        }
        break;
    case 7:
        if (result != (s32)Control.flushSize) {
            break;
        }
        Control.fileOpen = FALSE;
        Control.step = 8;
        if (NANDCloseAsync(&Control.fileInfo, SCFlushStep, COMMAND_BLOCK()) == 0) {
            return;
        }
        break;
    case 8:
        if (result != 0) {
            break;
        }
        /* fall through */
    case 9:
        FinishFlush();
        return;
    default:
        return;
    }
    Control.flushResult = SC_STATUS_ERROR;
    if (Control.fileOpen) {
        Control.step = 9;
        if (NANDCloseAsync(&Control.fileInfo, SCFlushStep, COMMAND_BLOCK()) == 0) {
            return;
        }
    }
    FinishFlush();
}
