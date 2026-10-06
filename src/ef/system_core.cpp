/*
 * ef/system_core.cpp - the game-system core: the `system_w` interface and its mode/loading accessors, the file
 *   loader, the work heap, the texture loader and the hbm/PMIC block.
 * RANGE. .text 0x800CE5A8-0x800D2FEC (127 functions); extab 0x8000A5E4-0x8000A77C, extabindex 0x80023C7C-0x80023EE0,
 *   .ctors 0x8056F2EC-0x8056F2F0, .data 0x80595118-0x80595378, .bss 0x80694C68-0x80695610, .sdata
 *   0x80791358-0x80791370, .sbss 0x80794958-0x80794970, .sdata2 0x80796360-0x807963C0.  Left edge: `ef/ef_sphere.cpp` ends at 0x800CE5A8 and
 *   `fn_800CE5A8` (the `system_w` side-table size 0x84D0) is called only from this unit's `fn_800CF3E4`.  Right edge:
 *   `fn_800D2F88` is the TU's `__sinit` (the `.ctors` word) and `fn_800D2F94` its element constructor; every function
 *   from 0x800D2FEC reads `.sbss` 0x80794970 (the `nw_resource.cpp` resource manager).
 * FLAGS. `cflags_main`.
 * NAMES. The unit name is a GUESS (no `__FILE__` string names the range; it is the game-system core).  The runtime
 *   dump names the loader, mode, work-heap, texture and string-table accessors (`file_loading_ck`, `load_file_req`,
 *   `load_file`, `ran_suu`, `system_w_clr`, `all_reset`, `PlayMode_ck`, `GameMode_set`, `PlayMode_set`,
 *   `work_mem_alloc`, `work_mem_free`, `get_move_work_adrs`, `get_move_work_max`, `set_move_work_max`,
 *   `create_move_work`, `TPLtexLoad`, `loading_disp_set`, `hbm_disable`, `hbm_enable`, `pmic_disp_off`,
 *   `get_str_tbl_ptr`, `get_str_tbl`); `GameMode_ck`, `my_player_no`, `my_player_no_set`, `player_count_get`,
 *   `player_count_set`, `game_ready_ck`, `game_reset_to_title`, `move_work_state_ck`, `ef_move_state_dispatch` and
 *   `setTransferDisplayState` are GUESSes from their bodies and callers (the dump's `SaveLoad::DidGameIDChange` /
 *   `BTM_IsDeviceUp` at four of these addresses contradict the one-byte bodies).
 *   GUESS: `monster_size_value_get` (0x800CEE74, from its body and its quest-result caller).
 * RESIDUALS. 54 rows unwritten in 23 runs (and `TPLtexLoad`, written, at 0 %: ours emits the plain `TPLtexLoad`
 *   where retail defines `TPLtexLoad__FPvP9_tex_info`); the largest by size is 0x800D0764-0x800D2098 (6 rows, the
 *   texture loader up to `fn_800D104C`); `sweepcomments.py --unit ef/system_core` lists them.
 *   31 partial rows, including:
 *  - `fn_800CEF18`, `ran_suu`: retail reduces the 0xB0/0xAD mix with the magic 0x00AD7539, `srwi 15` and
 *    `mulli 0xFF53`; the `u32` spelling here emits the 32-bit magic 0x7AD2208F (the original operand width is open);
 *  - `set_move_work_max`, `GameMode_set`, `PlayMode_set`, `get_move_work_adrs`, `get_move_work_max`: retail narrows
 *    the argument (`clrlwi`) at entry, ours folds it into the store or the `clrlslwi`;
 *  - `fn_800D20B4`: retail `extsh`s both GX-FIFO halfwords before the `sth`;
 *  - `fn_800D0568`, `fn_800D05A4`, `fn_800D05DC`: retail hoists `lis`/`addi system_w` above the `index < 0x20`
 *    check, ours materialises the base after the branch;
 *  - `fn_800CFD20`: the branch sense around the `fn_800CFC64` call is inverted.
 *   The other 19 partial rows have no recorded cause (`symdiff.py -u ef/system_core --all`).
 *   flipcheck: `.bss`/`.ctors`/`.data`/`.sbss`/`.sdata`/`.sdata2` claimed, not emitted; `.text` (0xA6C of 0x4A44),
 *   extab (0x58 of 0x198) and extabindex (0x84 of 0x264) short of the claim and differing; row 36: 8 retail
 *   force-active functions (`fn_800CF228`, `fn_800CF270`, `fn_800CF3A4`, `fn_800CF3B4`, `fn_800CF668`,
 *   `fn_800CFBE0`, `fn_800D20D8`, `fn_800D2108`) are not exported in ours.
 */

#include "types.h"
#include "gx.h"
#include "Runtime.PPCEABI.H/memset.h" /* memset is owned by Runtime.PPCEABI.H/memset.c (rule 2) */
#include "unsplit/unknown.h" /* SystemWork / system_w (undecided module, rule 2's unsplit gap) */
#include "ai/ainpc_w.h" /* `ainpc_w`, owned by ai/fn_802D44F4.cpp (rule 2): fn_800CEF60 clears it */
#include "fn_80047398.h" /* color_rgba_copy, owned by userdata_item.cpp's range (rule 2) */

/* Claimed data (splits.txt): `.data` 0x80595118-0x80595378, `.bss` 0x80694C68-0x80695610 (five objects, each still
 * `extern`-declared below) and `.sbss` 0x80794958-0x80794970 (the words at 0x80794958/0x80794960/com_data_func).  None of it is
 * emitted yet: the `.data` run holds the jump table of the unwritten `fn_800D21CC` (0x80595318, a rule-10 run `vtableaudit`
 * counts until the function is written) and strings/tables of unwritten bodies, and the `.bss`/`.sbss` words need their 8-byte
 * rows reproduced. */

/* --------------------------------------------------------------------------------------------- */
/* The unit's own claimed data (.bss 0x80694C68-0x80695610 and the rest of its splits block); declared, never
 * defined (playbook 29). */
/* --------------------------------------------------------------------------------------------- */

/* The system side table at .bss:0x80694C68: a 0x4C-byte record whose +0x48 word is the 0x84D0-byte
 * work buffer.  `fn_800CE5B4` zeroes it; `fn_800CE680`/`fn_800CEDBC`/`fn_800CE9F0` read the two
 * leading fields and the buffer pointer. */
typedef struct SysEntry {
    /* +0x000 */ u8 pad_0x00[0x150];
    /* +0x150 */ u32 field_0x150;
} SysEntry; /* size: 0x154 */

typedef struct SysBuf {
    /* +0x000 */ SysEntry entries[100]; /* the count limit fn_800CE9F0 tests is 0x63 */
} SysBuf; /* size: 0x84D0 */

typedef struct SysSideTable {
    /* +0x00 */ u32 field_0x00;   /* fn_800CE680 writes 3 */
    /* +0x04 */ s32 count;        /* fn_800CE9F0's allocation counter */
    /* +0x08 */ u8 pad_0x08[0x40];
    /* +0x48 */ SysBuf* buf;      /* the work buffer the loader allocates */
} SysSideTable; /* size: 0x4C */

extern SysSideTable lbl_80694C68; /* .bss:0x80694C68 */
extern u8 lbl_80794960;           /* .sdata, the debug-BBA flag */
extern u8 lbl_806954AC[];         /* .bss, the message-table tail */

/* The command-table dispatch pair the data layer calls through (.sdata:com_data_func). */
typedef void (*ComDataFunc)(u8, u16);
typedef void (*ComDataFunc1)(u8);
extern void* com_data_func[2]; /* [0] and [1], called as the two typedefs above */

/* The 0x44-byte system info/message record at .bss:0x80695468. */
typedef struct SysInfo {
    /* +0x00 */ u8 pad_0x00[0x1];
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 pad_0x02[0x4];
    /* +0x06 */ u16 field_0x06;
    /* +0x08 */ u8 pad_0x08[0x18];
    /* +0x20 */ u32 field_0x20;
    /* +0x24 */ u32 field_0x24;
    /* +0x28 */ u8 field_0x28;
    /* +0x29 */ u8 pad_0x29[0x1B];
} SysInfo; /* size: 0x44 */

/* The ring the texture/stream counter walks at .bss:0x80694CB8. */
typedef struct SysRing {
    /* +0x00 */ u32 head;
    /* +0x04 */ u8 pad_0x04[0x4];
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0c;
    /* +0x10 */ u32 field_0x10;
} SysRing; /* size: 0x14 */

extern SysInfo lbl_80695468;    /* .bss:0x80695468 *//* The message/resource pointer table `get_str_tbl_ptr` returns (.bss:0x806954AC): three of its
 * pointer slots are indexed by a u16 id. */
typedef struct StrTbl {
    /* +0x000 */ u8 pad_0x00[0x90];
    /* +0x090 */ u32* field_0x90;
    /* +0x094 */ u32* field_0x94;
    /* +0x098 */ u8 pad_0x098[0x64];
    /* +0x0FC */ u32* field_0xfc;
} StrTbl; /* size: 0x100 (lower bound) */

extern SysRing lbl_80694CB8;    /* .bss:0x80694CB8 */
extern u8 lbl_80695320[];       /* .bss:0x80695320, the hbm state byte */
extern s32 lbl_80791358;        /* .sdata, added to system_w[+0x4C] */
extern u32 lbl_80794958;        /* .sdata, the entry high-water mark */

struct _tex_info;

void* work_mem_alloc(u32 size);
u8* get_str_tbl_ptr(void);

/* Forward declarations: the bare `fn_` bodies below call each other, and the map's placeholder
 * names must be visible before the definitions (rule 9's spelling still comes from these exact names). */
extern "C" void fn_800CE6A0(void);
extern "C" void fn_800CFC64(void);

extern "C" u32 fn_800D02C0(void* data, struct _tex_info* info);
extern "C" void fn_800417F0(void* fn, u32 count);
extern "C" void* MEMAllocFromAllocator(void* allocator, u32 size);
extern "C" void MEMFreeToAllocator(void* allocator, void* ptr);
extern "C" void MEMDestroyExpHeap(void* heap);
extern "C" void* MEMCreateExpHeapEx(u32 base, u32 size, u32 align);
extern "C" void MEMInitAllocatorForExpHeap(void* allocator, void* heap, u32 align);
extern "C" u32 MEMGetAllocatableSizeForExpHeapEx(void* handle, u32 mode);
extern "C" u32 fn_80267548(void);

/* --------------------------------------------------------------------------------------------- */
/* extern "C": the map's bare `fn_XXXXXXXX` placeholders (unmangled while the file is C++) */
/* --------------------------------------------------------------------------------------------- */

extern "C" {

/* 0x800CE5A8 - the work-heap size the loader hands out (0x84D0). */
u32 fn_800CE5A8(void) {
    return 0x84D0;
}

/* 0x800CE5B4 - reset the side table and zero the work heap. */
void fn_800CE5B4(void) {
    memset(&lbl_80694C68, 0, 0x4C);
    lbl_80694C68.buf = (SysBuf*)system_w.field_0x948;
    memset(lbl_80694C68.buf, 0, sizeof(SysBuf));
}

/* 0x800CE680 - mark the side table live and stamp the first entry. */
void fn_800CE680(void) {
    lbl_80694C68.buf->entries[0].field_0x150 = 2;
    lbl_80694C68.field_0x00 = 3;
}

/* 0x800CEDBC - whether the side table's entry list is empty. */
u32 fn_800CEDBC(void) {
    return lbl_80694C68.count == 0;
}

/* 0x800CEE74/0x800CEE88 - the two command-table tails. */
void monster_size_value_get(u32 a, u32 b) {
    ((ComDataFunc)com_data_func[0])((u8)a, (u16)b);
}

void fn_800CEE88(u32 a) {
    ((ComDataFunc1)com_data_func[1])((u8)a);
}

/* 0x800CEE9C - seed the four random-ring cells with one value. */
void fn_800CEE9C(u8 value) {
    system_w.field_0x18[0] = value;
    system_w.field_0x18[1] = value;
    system_w.field_0x18[2] = value;
    system_w.field_0x18[3] = value;
}

/* 0x800CEF18 - the ring's value transform (0 -> 1, then the 0xB0 over 0xAD mix). */
u16 fn_800CEF18(u16 value) {
    u32 v = value;

    if (v == 0) {
        v = 1;
    }
    return (u16)((v * 0xB0) % 0xAD);
}

/* 0x800CEF60 - clear the 0x4A8-byte command record. */
void fn_800CEF60(void) {
    memset(&ainpc_w, 0, 0x4A8);
}

/* 0x800CF208 - the current game mode. */
u8 GameMode_ck(void) {
    return system_w.game_mode;
}

/* 0x800CF228 - the loading-screen mode byte. */
u8 fn_800CF228(void) {
    return system_w.field_0x29;
}

/* 0x800CF270 - store the loading-screen mode byte. */
void fn_800CF270(u8 value) {
    system_w.field_0x29 = value;
}

/* 0x800CF384 - the small mode bytes the reset path and the loader toggle. */
u8 my_player_no(void) {
    return system_w.field_0x27;
}

void my_player_no_set(u8 value) {
    system_w.field_0x27 = value;
}

u8 fn_800CF3A4(void) {
    return system_w.field_0x26;
}

void fn_800CF3B4(u8 value) {
    system_w.field_0x26 = value;
}

u8 player_count_get(void) {
    return system_w.field_0x28;
}

void player_count_set(u8 value) {
    system_w.field_0x28 = value;
}

/* 0x800CF624 - reset the loaded-byte counter. */
void fn_800CF624(void) {
    system_w.field_0x74 = 0;
}

/* 0x800CF638 - bytes loaded plus the streaming offset. */
u32 fn_800CF638(void) {
    return system_w.field_0x6c + system_w.field_0x74;
}

/* 0x800CF650 - the remaining byte count. */
u32 fn_800CF650(void) {
    return system_w.field_0x70 - system_w.field_0x74;
}

/* 0x800CF668 - advance the loaded-byte counter if it fits. */
u32 fn_800CF668(u32 delta) {
    u32 total = system_w.field_0x74 + delta;

    if (total > system_w.field_0x70) {
        return 0;
    }
    system_w.field_0x74 = total;
    return 1;
}

/* 0x800D06D0/0x800D06E0 - the loading-display flag. */
void fn_800D06D0(u8 value) {
    system_w.field_0x867 = value;
}

u8 fn_800D06E0(void) {
    return system_w.field_0x867;
}

/* 0x800D0724/0x800D0734/0x800D0744/0x800D0754 - the texture-load flags. */
u8 fn_800D0724(void) {
    return system_w.field_0x877;
}

void fn_800D0734(u8 value) {
    system_w.field_0x877 = value;
}

u8 fn_800D0744(void) {
    return system_w.field_0x886;
}

void fn_800D0754(u8 value) {
    system_w.field_0x886 = value;
}

/* 0x800D2D44 - clear the debug-BBA flag the reset path owns. */
void fn_800D2D44(void) {
    lbl_80794960 = 0;
}

/* 0x800D2E68/0x800D2EA4/0x800D2EE0 - the three message-table lookups. */
u32 fn_800D2E68(u32 id) {
    return ((StrTbl*)get_str_tbl_ptr())->field_0x90[(u16)id];
}

u32 fn_800D2EA4(u32 id) {
    return ((StrTbl*)get_str_tbl_ptr())->field_0xfc[(u16)id];
}

u32 fn_800D2EE0(u32 id) {
    return ((StrTbl*)get_str_tbl_ptr())->field_0x94[(u16)id];
}

/* 0x800D2F1C - step the two stream counters. */
void fn_800D2F1C(void) {
    u32 n = ++system_w.field_0x894;

    if (n % 0x1E != 0) {
        return;
    }
    n = ++system_w.field_0x890;
    if (n > 0x22550FF) {
        system_w.field_0x890 = 0x22550FF;
    }
}

/* --- the loader/stream group (0x800CE610..) --- */

/* 0x800CE610 - keep the work heap, clear the side table and register the stream callback. */
void fn_800CE610(void) {
    SysBuf* buf = lbl_80694C68.buf;

    memset(&lbl_80694C68, 0, 0x4C);
    lbl_80694C68.buf = buf;
    memset(buf, 0, sizeof(SysBuf));
    fn_800417F0((void*)fn_800CE6A0, 0x0F);
}

/* 0x800CE9F0 - hand out the next side-table entry. */
SysEntry* fn_800CE9F0(void) {
    SysBuf* buf = lbl_80694C68.buf;
    s32 n = lbl_80694C68.count;
    SysEntry* entry;

    if (n >= 0x63) {
        return 0;
    }
    entry = &buf->entries[n];
    lbl_80694C68.count = n + 1;
    if (lbl_80794958 < (u32)(n + 1)) {
        lbl_80794958 = n + 1;
    }
    return entry;
}

/* 0x800CF61C - the stream handle's read-size report. */
u32 fn_800CF61C(void* handle) {
    return MEMGetAllocatableSizeForExpHeapEx(handle, 4);
}

/* 0x800CF73C - rebuild the work heap. */
void fn_800CF73C(void) {
    if (system_w.field_0x90) {
        MEMDestroyExpHeap(system_w.field_0x90);
    }
    system_w.field_0x90 = MEMCreateExpHeapEx(system_w.field_0x60, system_w.field_0x64, 4);
    MEMInitAllocatorForExpHeap(&system_w.mem_allocator, system_w.field_0x90, 0x20);
    system_w.field_0x68 = fn_800CF61C(system_w.field_0x90);
}

/* 0x800CF8EC - lazily build the move-work record. */
void fn_800CF8EC(void) {
    if (system_w.field_0xa4 == 0) {
        system_w.field_0xa4 = (MoveWork*)work_mem_alloc(0x2C);
    }
    if (system_w.field_0xa4) {
        memset(system_w.field_0xa4, 0, 0x2C);
    }
}

/* 0x800CFBA0 - reset the stream record and stamp its top byte. */
void fn_800CFBA0(u32 value) {
    color_rgba_copy((u8*)&system_w.field_0x7bc, (const u8*)value);
    system_w.field_0x7bc.bytes[3] = 0xFF;
}

/* 0x800CFBE0 - the stream record's word. */
u32 fn_800CFBE0(void) {
    return system_w.field_0x7bc.value;
}

/* 0x800CFD00 - arm the two stream bounds. */
void fn_800CFD00(void) {
    system_w.field_0x878 = system_w.field_0x4c;
    system_w.field_0x87c = system_w.field_0x4c + lbl_80791358;
}

/* 0x800CFD20 - pick the stream arm path by the +0x22 mode byte. */
void fn_800CFD20(void) {
    if (system_w.field_0x22 < 2) {
        fn_800CFD00();
    } else {
        fn_800CFC64();
    }
}

/* 0x800CFDB0 - rewind the counter ring. */
void fn_800CFDB0(void) {
    lbl_80694CB8.field_0x08 = lbl_80694CB8.head;
    lbl_80694CB8.field_0x0c = lbl_80694CB8.head;
    lbl_80694CB8.field_0x10 = 0;
}

/* 0x800D0568/0x800D05A4/0x800D05DC - the task table accessors. */
u8* fn_800D0568(s32 index) {
    if (index >= 0x20) {
        return 0;
    }
    if (system_w.tasks[index].active == 0) {
        return 0;
    }
    return system_w.tasks[index].body;
}

SysTask* fn_800D05A4(s32 index) {
    if (index >= 0x20) {
        return 0;
    }
    if (system_w.tasks[index].active == 0) {
        return 0;
    }
    return &system_w.tasks[index];
}

/* 0x800D05DC - the task's +0x24 word, or NULL. */
u32 fn_800D05DC(s32 index) {
    if (index >= 0x20) {
        return 0;
    }
    if (system_w.tasks[index].active == 0) {
        return 0;
    }
    return system_w.tasks[index].field_0x24;
}

/* 0x800D06B8 - TPL texture load entry. */
s32 TPLtexLoad(void* data, struct _tex_info* info) {
    if (data == 0) {
        return -1;
    }
    return fn_800D02C0(data, info);
}

/* 0x800D06F0 - raise both loader flags. */
void fn_800D06F0(void) {
    system_w.field_0x7cc = 1;
    system_w.field_0x7cd = 1;
}

/* 0x800D0708 - whether the +0x7D3 flag is exactly 1. */
u32 game_ready_ck(void) {
    return system_w.field_0x7d3 == 1;
}

/* 0x800D2098/0x800D20A8/0x800D20B4 - the GX FIFO writers. */
void fn_800D2098(f32 a, f32 b) {
    GXWGFifo.f32 = a;
    GXWGFifo.f32 = b;
}

void fn_800D20A8(u32 a) {
    GXWGFifo.u32 = a;
}

void fn_800D20B4(s32 a, s32 b) {
    s16 lo = a;
    s16 hi = b;

    GXWGFifo.s16 = lo;
    GXWGFifo.s16 = hi;
}

/* 0x800D20D8 - whether the hbm state byte is set. */
u32 fn_800D20D8(void) {
    return lbl_80695320[0] != 0;
}

/* 0x800D2108 - arm the hbm state. */
void fn_800D2108(u32 flag) {
    if (flag == 0) {
        system_w.field_0x2c = 1;
    } else {
        system_w.field_0x2c = 2;
    }
}

/* 0x800D2178 - clear the system info record and suppress the home menu. */
void fn_800D2178(void) {
    memset(&lbl_80695468, 0, 0x44);
    lbl_80695468.field_0x24 = 0x10;
    system_w.hbm_disabled = 1;
}

/* 0x800D2660 - clear the info record's flags. */
void fn_800D2660(void) {
    lbl_80695468.field_0x01 = 0;
    lbl_80695468.field_0x06 = 0;
}

/* 0x800D28FC - step the display state. */
void setTransferDisplayState(u8 value) {
    system_w.field_0xa58 = (u8)(value + 1);
}

/* 0x800D2928 - notify the display-state owner. */
void fn_800D2928(void) {
    if (system_w.field_0xa58 != 0) {
        system_w.field_0xa54(0x14, 0x140);
    }
}

/* 0x800D2954 - arm one of the four stream slots. */
void fn_800D2954(u8 index) {
    system_w.field_0x7e4[index] = 1;
    system_w.field_0x7e0[index] = 0x78;
}

} /* extern "C" */

/* --------------------------------------------------------------------------------------------- */
/* C++ free functions: the map's mangled names (`Name__F...`), so the C++ front-end reproduces the
 * spelling from the real signature (rule 9). */
/* --------------------------------------------------------------------------------------------- */

/* 0x800CEEBC - draw the next ring value and store it back. */
u16 ran_suu(long index) {
    u32 v = system_w.field_0x18[index];

    if (v == 0) {
        v = 1;
    }
    v = (v * 0xB0) % 0xAD;
    system_w.field_0x18[index] = (u16)v;
    return (u16)v;
}

/* 0x800CEF74 - clear the mode/loading flags the reset path owns. */
void system_w_clr(void) {
    system_w.field_0x7d5 = 0;
    system_w.field_0x05 = 0;
    system_w.field_0x30 = 0;
    system_w.field_0x866 = 0;
    system_w.field_0x884 = 0;
    system_w.field_0x877 = 0;
}

/* 0x800CF218 - the current play mode. */
u8 PlayMode_ck(void) {
    return system_w.play_mode;
}

/* 0x800CF238 - set the game mode, bounded. */
void GameMode_set(u8 mode) {
    if (mode < 4) {
        system_w.game_mode = mode;
    }
}

/* 0x800CF254 - set the play mode, bounded. */
void PlayMode_set(u8 mode) {
    if (mode < 7) {
        system_w.play_mode = mode;
    }
}

/* 0x800D2138/0x800D214C - the home-button-menu disable flag. */
void hbm_disable(void) {
    system_w.hbm_disabled = 1;
}

void hbm_enable(void) {
    if (system_w.field_0x86b == 1) {
        system_w.hbm_disabled = 1;
    } else {
        system_w.hbm_disabled = 0;
    }
}

/* 0x800CF7A8 - aligned work-heap allocation. */
void* work_mem_alloc(u32 size) {
    void* p;

    if (size == 0) {
        return 0;
    }
    p = MEMAllocFromAllocator(&system_w.mem_allocator, (size + 0x1F) & ~0x1F);
    system_w.field_0x68 = fn_800CF61C(system_w.field_0x90);
    return p;
}

/* 0x800CF814 - release a work-heap block. */
void work_mem_free(void* ptr) {
    MEMFreeToAllocator(&system_w.mem_allocator, ptr);
    system_w.field_0x68 = fn_800CF61C(system_w.field_0x90);
}

/* 0x800CFA90 - the move-work pointer for one slot. */
void* get_move_work_adrs(u8 index) {
    if (system_w.field_0xa4 == 0) {
        return 0;
    }
    if (index >= 7) {
        return 0;
    }
    return system_w.field_0xa4->addr[index];
}

/* 0x800CFAD0 - the move-work maximum for one slot. */
u16 get_move_work_max(u8 index) {
    if (system_w.field_0xa4 == 0) {
        return 0;
    }
    if (index >= 7) {
        return 0;
    }
    return system_w.field_0xa4->max[index];
}

/* 0x800CFB0C - store the move-work maximum for one slot. */
void set_move_work_max(u8 index, long value) {
    u16 v = value;

    system_w.field_0xa4->max[index] = v;
}

/* 0x800D2914 - power the display off. */
void pmic_disp_off(void) {
    system_w.field_0xa58 = 0;
}

/* 0x800D2E28 - the message-table tail. */
u8* get_str_tbl_ptr(void) {
    return lbl_806954AC;
}
