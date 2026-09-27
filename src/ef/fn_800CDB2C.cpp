/*
 * ef/fn_800CDB2C.cpp - the `.text` 0x800CDB2C..0x800D45AC run, 165 functions / 0x6A80 bytes.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every
 * `fn_` name this file uses is a bare `.text` entry in config/RMHE08/symbols.txt; only the 24 named
 * symbols - file_loading_ck, load_file_req, load_file, ran_suu, system_w_clr, all_reset, PlayMode_ck,
 * GameMode_set, PlayMode_set, work_mem_alloc, work_mem_free, get_move_work_adrs, get_move_work_max,
 * set_move_work_max, create_move_work, TPLtexLoad, loading_disp_set, hbm_disable, hbm_enable,
 * pmic_disp_off, get_str_tbl_ptr, get_str_tbl, push_g3d_wk - carry a name)
 *
 * What it is.  The brief's `proposal/800CDB2C_fn_800CDB2C` range is one of discovery's `--max-bytes`
 * cuts and its seam is a guess: the range is **several original translation units**, not one.  The
 * evidence is the split objects' own `__FILE__` strings (from MAIN's `build/RMHE08/asm/auto_*.s`,
 * which carries the pooled `.data` labels), in address order:
 *   - `ef_sphere.cpp` (0x80595058, the `em`/`pm`/`params` pointer asserts at 0x800CDCA4..0x800CDEC4)
 *     names the first function only, `ef_sphere_spawn` (0xA78).  `src/ef/ef_point.cpp`'s own header
 *     records the seam `ef_point.cpp -> ef_sphere.cpp`, so the range starts inside the ef band.
 *   - `00/saveicon.tpl` (0x80595198), `00/capcom.tpl` (0x805951A8) then `m.m.tpl` are the texture
 *     names of the loading/UI tail (TPLtexLoad et al.), which is not ef code.
 *   - `memorymanagertmp.h` (0x80595520/0x80595584) and `g3d_anmobj.h` (0x8059577C, beside
 *     `g3d_xsi.cpp`) name the TUs that end the range.
 * More than one original file is evidenced, so no single name names the unit: the registration uses
 * the map's `fn_` stem (evidence class 4), exactly as `ef/fn_800AEE48.cpp` did for the same
 * capped-seam situation.  The module is `ef` (the `__FILE__` string and the band below are ef) and
 * the language is C++ (the `.cpp`/`.h` names and the `nw4r::db::Panic` callee).
 *
 * The game-system core - `system_w` and its mode/loading accessors, the file loader and the work
 * heap - is the middle of the range and the reason it is not one ef file.  This unit owns the
 * `system_w` interface, so `SystemWork`'s newly-evidenced fields were added in
 * `include/unsplit/unknown.h` (the symbol is unsplit - no `.bss` range is registered - so that is its
 * named home; docs/plan.md 6.5 rule 2's gap).
 *
 * Status.  70 of the 165 symbols are reconstructed (0x9EC = 2540 B of 0x6A80), 63 of them at the
 * 80 % bar or above and 41 byte-identical (100 %); the unit as a whole is 7.86 % of code bytes.  The
 * reconstructed set is the `system_w` interface and the loader/stream accessors at the head of the
 * range (0x800CE5A8..0x800D2FEC, minus the ones this round did not reach).  `fn_800D20F0` (a
 * `fn_800E4E3C` thunk) is left out on purpose: declaring that sound unit's symbol here would be a
 * rule-2 addition, and the owner's header does not exist yet.  The 95 missing symbols are
 * the large players in the middle and tail: the ef_sphere emitter `ef_sphere_spawn` (0xA78), the loader
 * state machine `fn_800CE6A0`/`fn_800CE884`/`fn_800CE920` (the 0x154-entry record copier), the file
 * loader `file_loading_ck`/`load_file_req`/`load_file`, the mode reset block `fn_800CF154`..`fn_800CF3E4`,
 * `fn_800CF948` (the 0x800D streams), the texture loader `fn_800D0764`..`fn_800D104C`, the hbm/PMIC
 * block, and the whole 0x800D3000..0x800D45AC group (g3d/memory-manager TUs).
 *
 * Residuals of the below-80 % reconstructions (all still applied, per AGENTS.md's best-variant rule):
 *   - `fn_800CEF18` 77.4 % and `ran_suu__Fl` 68.2 %: the 0xB0/0xAD 16-bit mix.  The target materialises
 *     the divisor magic as `0x00AD7539` + `srwi ...,15` + `mulli ...,0xFF53`; writing `(v * 0xB0) %
 *     0xAD` on `u32` (best, 77.4 %) emits the 32-bit magic `0x7AD2208F`, and the `u16` spelling (60.2 %)
 *     a third shape.  The target's magic is not the `x / 173` constant for a 32-bit dividend, so the
 *     original operand width is still unidentified - a residual to re-derive from the caller.
 *   - `set_move_work_max` 63.1 %: the target masks `value` with an explicit `clrlwi r5,r4,16` before the
 *     `sthx`; every spelling here (long, u16 local, u16 cast) lets the store width do the truncation.
 *   - `fn_800D20B4` 62.5 %: the target `extsh`s both GX-FIFO halfwords; the `s16` local/cast still emits
 *     a bare `sth` because the union member's store already truncates.
 *   - `fn_800D0568`/`fn_800D05A4`/`fn_800D05DC` 32-59.6 %: the target hoists `lis/addi system_w` above
 *     the `index < 0x20` bound check and keeps the index in r3, base in r4; every source order tried
 *     materialises the base after the branch.  The bodies' behaviour is right (the accessors agree with
 *     `get_move_work_*`), only the schedule differs.
 *   - `GameMode_set`/`PlayMode_set` (85 %), `fn_800CF638`/`fn_800CF650` (98.3 %), `fn_800D28FC` (83.3 %),
 *     `fn_800D2954` (87.2 %), `get_move_work_adrs`/`get_move_work_max` (89.7/89.0 %), `fn_800CF8EC`
 *     (90.7 %), `fn_800CFD00` (93.1 %), `fn_800D2928` (95.9 %), `fn_800CE610` (99.6 %) and
 *     `system_w_clr` (99.9 %) are above the bar; their sub-instruction deltas are not chased.
 *
 * Flags.  The range is measured under the ef lib's real command line (`-O3 -inline noauto
 * -Cpp_exceptions on`, `Wii/1.3`).  No flag was probed or changed: the below-80 % residuals are
 * source/schedule shapes, not instruction-set fingerprints (no `lmw`/`stmw`, no fused float op seen).
 * A `-O4,p` / `-inline` sweep over the range is the next probe if a later round needs it.
 */

#include "types.h"
#include "gx.h"
#include "Runtime.PPCEABI.H/memset.h" /* memset is owned by Runtime.PPCEABI.H/memset.c (rule 2) */
#include "unsplit/unknown.h" /* SystemWork / system_w (undecided module, rule 2's unsplit gap) */

/* --------------------------------------------------------------------------------------------- */
/* Data owned by no registered unit (its range sits outside every splits.txt block); declared,
 * never defined - the auto split's own bss/data object still provides it (playbook 29). */
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
extern u8 lbl_806BD360[0x4A8];    /* .bss:0x806BD360, cleared by fn_800CEF60 */
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
extern "C" void fn_8004C4F0(void* record, u32 value);
extern "C" void* MEMAllocFromAllocator(void* allocator, u32 size);
extern "C" void MEMFreeToAllocator(void* allocator, void* ptr);
extern "C" void MEMDestroyExpHeap(void* heap);
extern "C" void* MEMCreateExpHeapEx(u32 base, u32 size, u32 align);
extern "C" void MEMInitAllocatorForExpHeap(void* allocator, void* heap, u32 align);
extern "C" u32 fn_804C2380(void* handle, u32 mode);
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
void fn_800CEE74(u32 a, u32 b) {
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
    memset(lbl_806BD360, 0, 0x4A8);
}

/* 0x800CF208 - the current game mode. */
u8 fn_800CF208(void) {
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

void fn_800CF394(u8 value) {
    system_w.field_0x27 = value;
}

u8 fn_800CF3A4(void) {
    return system_w.field_0x26;
}

void fn_800CF3B4(u8 value) {
    system_w.field_0x26 = value;
}

u8 fn_800CF3C4(void) {
    return system_w.field_0x28;
}

void fn_800CF3D4(u8 value) {
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
    return fn_804C2380(handle, 4);
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
    fn_8004C4F0(&system_w.field_0x7bc, value);
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
void fn_800D28FC(u8 value) {
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
