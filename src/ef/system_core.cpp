/*
 * ef/system_core.cpp - the game-system core: the `system_w` interface and its mode/loading accessors, the file
 *   loader, the work heap, the texture loader and the hbm/PMIC block.
 * RANGE. .text 0x800CE5A8-0x800D2FEC (127 functions); extab 0x8000A5E4-0x8000A77C, extabindex 0x80023C7C-0x80023EE0,
 *   .ctors 0x8056F2EC-0x8056F2F0, .data 0x80595118-0x80595378, .bss 0x80694C68-0x80695610, .sdata
 *   0x80791358-0x80791370, .sbss 0x80794958-0x80794970, .sdata2 0x80796360-0x807963C0.  Left edge: `ef/ef_sphere.cpp` ends at 0x800CE5A8 and
 *   `fn_800CE5A8` (the `system_w` side-table size 0x84D0) is called only from this unit's `memory_layout_init`.  Right edge:
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
 *   GUESS (from each body and its callers): rand_lcg_step
 *   GUESS (from its body and its kitchen caller): str_tbl_course_get
 *   GUESS (from each body and its menu callers): str_tbl_skill_get (+0x90, read by the skill-status page),
 *   str_tbl_item_kind_get (+0x94, read through Pl's item_kind_name_get)
 *   GUESS (from each body and its callers): system_stream_count_step, system_scene_reset, system_full_reset
 *   GUESS: `monster_size_value_get` (0x800CEE74, from its body and its quest-result caller).
 *   GUESS (from each body): `loading_caption_set` (0x800D0FCC: retail passes it the quest's field text and it copies
 *   the loading caption - `quest/quest_entry.cpp`'s zero-argument call is wrong), `sys_work_stack` (.bss 0x80694CB8, the bottom/top
 *   work stack `work_stack_alloc`..`work_stack_free` run), `sys_loading` (.bss 0x80695320, the loading display),
 *   `sys_loading_sprites`/`sys_loading_pos`/`sys_loading_counts`, `move_work_counts`, `sys_resident_path`.
 *   GUESS (from each body and its callers): `dvd_file_info_copy`, `load_queue_pop`, `load_req_copy`,
 *   `load_file_direct`, `dvd_read_wait`, `dvd_open_wait`, `quest_system_reset`, `memory_layout_init`,
 *   `pack_entry_size`, `enemy_work_release_all`, `move_work_kind_alloc`, `resident_file_load`, `stream_ring_reset`,
 *   `work_stack_alloc`, `work_stack_find`, `work_stack_free_index`, `work_stack_free`, `work_stack_reset`,
 *   `tex_palette_bind`, `tex_palette_bind_point`, `tex_slot_free`, `loading_disp_reset`, `system_nop`,
 *   `tex_coord_gen_set`, `sound_volume_mute_set`, `sys_msg_build`.
 * RESIDUALS. 10 rows unwritten: `fn_800D0980` (0x594), `fn_800D104C` (0x1048, the loading display's draw),
 *   `fn_800D21CC` (0x448), `fn_800D2614`, `fn_800D2678`, `fn_800D27AC`, `fn_800D2978` (0x3CC), `fn_800D2D50`, and the
 *   `.ctors` pair `fn_800D2F88`/`fn_800D2F94` (the loading display's four `VEC3` constructions, which need
 *   `SysLoading` to be a class).
 *   Partial rows (`measure.py ef/system_core` lists them), including:
 *  - `rand_lcg_step`, `ran_suu`: retail reduces the 0xB0/0xAD mix with the magic 0x00AD7539, `srwi 15` and
 *    `mulli 0xFF53`; the `u32` spelling here emits the 32-bit magic 0x7AD2208F (the original operand width is open);
 *  - `set_move_work_max`, `GameMode_set`, `PlayMode_set`, `get_move_work_adrs`, `get_move_work_max`: retail narrows
 *    the argument (`clrlwi`) at entry, ours folds it into the store or the `clrlslwi`;
 *  - `fn_800D20B4`: retail `extsh`s both GX-FIFO halfwords before the `sth`;
 *  - `fn_800D0568`, `fn_800D05A4`, `sys_tex_image_get`: retail hoists `lis`/`addi system_w` above the `index < 0x20`
 *    check, ours materialises the base after the branch;
 *  - `fn_800CFD20`: the branch sense around the `fn_800CFC64` call is inverted;
 *  - `tex_palette_bind`, `fn_800D02C0`, `tex_palette_bind_point`: retail reloads the slot's image fields for `GXInitTexObj`, ours
 *    forwards the stored values;
 *  - `loading_disp_reset`: retail unrolls the sprite copy by 4 with a separate offset for the y column, ours by 8;
 *  - `memory_layout_init`: the layout's size array is kept `volatile` to keep retail's stack stores (580 vs 568 bytes).
 *   A STOPGAP block declares `enemy_work_release(_ENEMY_WORK*)` (retail passes the record; `unsplit/enemy.h` and
 *   `enemy/fn_80138074.cpp` spell it without one) until its request lands.
 *   flipcheck: `.bss`/`.ctors`/`.data`/`.sbss`/`.sdata` claimed, not emitted; `.sdata2` ours is the literal pool;
 *   row 36: 8 retail force-active functions (`fn_800CF228`, `fn_800CF270`, `fn_800CF3A4`, `fn_800CF3B4`,
 *   `fn_800CF668`, `fn_800CFBE0`, `fn_800D20D8`, `fn_800D2108`) are not exported in ours.
 */

#include "types.h"
#include "gx.h"
#include "Runtime.PPCEABI.H/memset.h" /* memset is owned by Runtime.PPCEABI.H/memset.c (rule 2) */
#include "unsplit/unknown.h" /* SystemWork / system_w (undecided module, rule 2's unsplit gap) */
#include "ai/ainpc_w.h" /* `ainpc_w`, owned by ai/fn_802D44F4.cpp (rule 2): fn_800CEF60 clears it */
#include "fn_80047398.h" /* color_rgba_copy, owned by userdata_item.cpp's range (rule 2) */
#include "DVD/dvdfs.h" /* DVDFileInfo, DVDOpen, DVDClose, DVDReadAsyncPrio */
#include "OS/OSCache.h" /* DCFlushRange */
#include "MSL_C/string.h" /* strcpy */
#include "MSL/strlen.h" /* strlen */
#include "ef/sys_quest_move.h" /* SysQuestMove */
#include "menu/movie.h" /* fn_8043F088 */
#include "sound/fn_800E3CBC.h" /* prim_init_all, fn_800E3DE0 */
#include "ef/eft_res.h" /* eft_res_release_all, fn_800F6520 */
#include "enemy/ENEMY_WORK.h" /* _ENEMY_WORK */
#include "Pl/pl_coll.h" /* fn_8029708C */
#include "font/flfnt.h" /* fn_8005B26C */
#include "TPL/tpl.h" /* TPLBind, TPLGet */
#include "fn_8004CAD8.h" /* wii_sysmsg_gen */
#include "RVLGX/GXAttr.h" /* GXSetTexCoordGen2 */
#include "sound/fn_800E46E8.h" /* fn_800E4E3C */
#include "RVLGX/GXTexture.h" /* GXInitTexObj, GXInitTexObjLOD */
#include "ef/effect.h" /* fn_800F9628 */
#include "enemy/em_model.h" /* fn_803B9FC8 */
#include "Pl/player_control.h" /* player_control_release */
#include "enemy/em_prog_support.h" /* fn_803854E4 */
#include "camera/camera_work_init.h" /* camera_work_init */
#include "light/light.h" /* light_init, fn_802BF5BC */
#include "fn_80056F24.h" /* filter_reset, filter_flag_clear, fade_reset */
#include "nw_resource.h" /* nwWorkInitialize */
#include "userdata_item.h" /* fn_80048F98 */
#include "pad_connect.h" /* fn_80047020 */
#include "draw_shape.h" /* fn_80052760 */
#include "sound/snd_bank_loader.h" /* fn_800F0554 */
#include "mh3_pad.h" /* fn_80044A54, fn_80041680, fn_80041944 */
#include "sound/fn_800D7F54.h" /* sound_frame_entry */
#include "sound/fn_800F2A94.h" /* bgm_stop_all, snd_quest_result_bgm_set */
#include "stage/stg_w.h" /* stage_area_slot_release */
#include "ef/sys_tex_file_table.h" /* sys_tex_file_table */
#include "enemy/em_pop.h" /* quest_move_sub_state_4_set */
#include "menu/menu_item.h" /* fn_802A0188 */
#include "stage/get_now_mapno.h" /* get_now_mapno */
#include "lobby/lobby_w.h" /* lobby_w */

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
/* A load request's path, copied as one record.  size: 0x100 */
typedef struct SysPath {
    /* +0x00 */ char text[0x100];
} SysPath;

/* A load request's callback arguments, copied as one record.  size: 0x40 */
typedef struct SysArgs {
    /* +0x00 */ u32 arg[16];
} SysArgs;

/* The completion callback a load request carries: the path, the buffer and the request's arguments. */
/* untyped: byte range - the loaded file image */
typedef void (*SysLoadCallback)(char* path, void* buffer, s32 argc, u32* args);

/* One background load request: the buffer and its size, the path, the callback and its arguments, and the state
 * (1 queued, 2 read).  size: 0x154 */
typedef struct SysEntry {
    /* +0x000 */ u32 size;
    /* +0x004 */ void* buffer;
    /* +0x008 */ SysPath path;
    /* +0x108 */ SysLoadCallback callback;
    /* +0x10C */ s32 argc;
    /* +0x110 */ SysArgs args;
    /* +0x150 */ u32 state;
} SysEntry; /* size: 0x154 */

typedef struct SysBuf {
    /* +0x000 */ SysEntry entries[100]; /* the count limit fn_800CE9F0 tests is 0x63 */
} SysBuf; /* size: 0x84D0 */

typedef struct SysSideTable {
    /* +0x00 */ s32 state;        /* 0 idle, 1 opening, 2 reading, 3 read (`fn_800CE680` writes it) */
    /* +0x04 */ s32 count;        /* fn_800CE9F0's allocation counter */
    /* +0x08 */ s32 frames;       /* the frames the current read has taken */
    /* +0x0C */ DVDFileInfo file; /* the open file of the head request */
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

/* One work-stack block's name.  size: 0x10 */
typedef struct SysStackName {
    /* +0x00 */ char text[0x10];
} SysStackName;

/* The work stack at .bss:0x80694CB8, spanning the +0x4C region: blocks are taken from the bottom (named, at most 64)
 * or from the top. */
typedef struct SysStack {
    /* +0x000 */ u32 head;            /* the region base */
    /* +0x004 */ u32 size;            /* the region size */
    /* +0x008 */ u32 field_0x08;      /* the bottom the rewind returns to */
    /* +0x00C */ u32 cur_0x0C;        /* the next bottom block */
    /* +0x010 */ u32 used_0x10;       /* the bytes taken from the bottom */
    /* +0x014 */ u32 end_0x14;        /* the region end */
    /* +0x018 */ u32 top_0x18;        /* the top mark */
    /* +0x01C */ u32 used_top_0x1C;   /* the bytes taken from the top */
    /* +0x020 */ u8 count_0x20;       /* the bottom blocks */
    /* +0x021 */ u8 live_0x21[64];
    /* +0x061 */ u8 pad_0x61[0x3];
    /* +0x064 */ u32 addr_0x64[64];
    /* +0x164 */ u32 size_0x164[64];
    /* +0x264 */ SysStackName name_0x264[64];
} SysStack; /* size: 0x664 (the block `stream_ring_reset` clears) */

/* One entry of an archive's big-endian table: its offset from the archive and its size.  size: 0x8 */
typedef struct SysArchiveEntry {
    /* +0x0 */ u32 offset;
    /* +0x4 */ u32 size;
} SysArchiveEntry;

/* An archive the system core reads entries out of: the big-endian count and the table.  size: 0xC (lower bound,
 * an approximation: one entry) */
typedef struct SysArchive {
    /* +0x0 */ s32 count;
    /* +0x4 */ SysArchiveEntry entries[1];
} SysArchive;

/* The move-work record counts of each scene kind (`.data` 0x80595118, 9 x 7 halfwords) and the resident file's
 * path (`.data` 0x80595198), in the unit's `.data` claim; declared, not defined. */
extern u16 move_work_counts[9][7];
extern char sys_resident_path[];
extern char sys_common_tex_path[];

extern SysInfo lbl_80695468;    /* .bss:0x80695468 *//* The message/resource pointer table `get_str_tbl_ptr` returns (.bss:0x806954AC): three of its
 * pointer slots are indexed by a u16 id. */
typedef struct StrTbl {
    /* +0x000 */ u8 pad_0x00[0x90];
    /* +0x090 */ u32* field_0x90;
    /* +0x094 */ u32* field_0x94;
    /* +0x098 */ u8 pad_0x098[0x64];
    /* +0x0FC */ u32* field_0xfc;
} StrTbl; /* size: 0x100 (lower bound) */

extern SysStack sys_work_stack;    /* .bss:0x80694CB8 */
/* The loading display at .bss:0x80695320: shown now and last call, its sprites with their positions and timers, the
 * spinner quad, its style and caption, and its texture.  size: 0x110 */
typedef struct SysLoading {
    /* +0x000 */ u8 shown_0x00;
    /* +0x001 */ u8 prev_0x01;
    /* +0x002 */ u8 field_0x02;
    /* +0x003 */ u8 field_0x03;
    /* +0x004 */ u8 field_0x04;
    /* +0x005 */ u8 pad_0x05;
    /* +0x006 */ u16 frame_0x06;
    /* +0x008 */ u32 sprite_0x08[11];
    /* +0x034 */ Vec quad_0x34[4];
    /* +0x064 */ s16 timer_0x64[11];
    /* +0x07A */ s16 x_0x7A[11];
    /* +0x090 */ s16 y_0x90[11];
    /* +0x0A6 */ u8 mode_0xA6;
    /* +0x0A7 */ char text_0xA7[0x2B];
    /* +0x0D2 */ u8 pad_0xD2[0x2];
    /* +0x0D4 */ u32 texture_0xD4;  /* the texture file's load address */
    /* +0x0D8 */ SysTask tex_0xD8;
} SysLoading;
extern SysLoading sys_loading;

/* One sprite position of the loading display's per-language tables.  size: 0x4 */
typedef struct SysLoadingPos {
    /* +0x0 */ s16 x;
    /* +0x2 */ s16 y;
} SysLoadingPos;

/* The loading display's sprite ids, its per-language position tables and their counts, in the unit's `.data` and
 * `.sdata` claims; declared, not defined. */
extern u16 sys_loading_sprites[12];
extern SysLoadingPos* sys_loading_pos[6];
extern u8 sys_loading_counts[8];
extern s32 lbl_80791358[2];     /* .sdata, the two stream reserves below the resident file */
extern u32 lbl_80794958;        /* .sdata, the entry high-water mark */

struct _tex_info;

void* work_mem_alloc(u32 size);
u8* get_str_tbl_ptr(void);

/* Forward declarations: the bare `fn_` bodies below call each other, and the map's placeholder
 * names must be visible before the definitions (rule 9's spelling still comes from these exact names). */
extern "C" void fn_800CE6A0(void);
extern "C" void fn_800CE680(void);
extern "C" void dvd_file_info_copy(DVDFileInfo* dst, DVDFileInfo* src);
extern "C" void load_queue_pop(void);
extern "C" void load_req_copy(SysEntry* dst, SysEntry* src);
extern "C" SysEntry* fn_800CE9F0(void);
/* untyped: byte range - the read destination */
extern "C" s32 dvd_read_wait(DVDFileInfo* info, void* buffer, s32 length, s32 offset);
extern "C" void enemy_work_release_all(void);
extern "C" void work_stack_reset(void);
extern "C" void fn_800CFBA0(u8* color);
extern "C" void fn_800CFDB0(void);
extern "C" void fn_800D06D0(u8 value);
extern "C" void fn_800CEF60(void);
/* STOPGAP-BEGIN(ef-game4#149) */
extern "C" void enemy_work_release(_ENEMY_WORK* work);
/* STOPGAP-END(ef-game4#149) */
extern "C" void loading_disp_reset(u8 mode);
extern "C" void fn_800D2954(u8 index);
extern "C" void fn_800CF73C(void);
extern "C" void fn_800CF8EC(void);
extern "C" void fn_800CFD20(void);
extern "C" u32 fn_800CF61C(void* handle);
extern "C" u32 fn_800CE5A8(void);
extern "C" void move_work_kind_alloc(u8 kind, u16 count);
extern "C" void tex_slot_free(s32 index, s32 count);
extern "C" s32 tex_palette_bind(TPLPalette* pal, s32 index);
long load_file(char* path, unsigned long buffer, long size);
s32 TPLtexLoad(void* data, _tex_info* info);
u16 get_move_work_max(u8 index);
void* work_mem_alloc(u32 size);
void* get_move_work_adrs(u8 index);
extern "C" void fn_800CE610(void);
extern "C" void my_player_no_set(u8 value);
extern "C" u8 GameMode_ck(void);
u8 PlayMode_ck(void);
void all_reset(void);
void system_w_clr(void);

extern "C" void fn_800CFC64(void);

extern "C" s32 fn_800D02C0(TPLPalette* pal, SysTask* slot);
extern "C" void fn_800417F0(void* fn, u32 count);
extern "C" void* MEMAllocFromAllocator(void* allocator, u32 size);
extern "C" void MEMFreeToAllocator(void* allocator, void* ptr);
extern "C" void MEMDestroyExpHeap(void* heap);
extern "C" void* MEMCreateExpHeapEx(u32 base, u32 size, u32 align);
extern "C" void MEMInitAllocatorForExpHeap(void* allocator, void* heap, u32 align);
extern "C" u32 MEMGetAllocatableSizeForExpHeapEx(void* handle, u32 mode);
extern "C" u32 player_work_array_get(void);

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
    lbl_80694C68.buf->entries[0].state = 2;
    lbl_80694C68.state = 3;
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
u16 rand_lcg_step(u16 value) {
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

/* 0x800CF05C (0x94): Resets the quest-side systems (network work, primitives, sounds, effects, camera, light,
 * filters, fade) and the player number. */
void quest_system_reset(void) {
    u32 color;

    nwWorkInitialize();
    prim_init_all();
    sound_free_lists_reset();
    eft_res_pool_reset();
    camera_work_init();
    light_init();
    filter_reset();
    fn_800CEF60();
    color = 0;
    fn_800CFBA0((u8*)&color);
    fade_reset(0);
    filter_flag_clear();
    userdata_frame_reset();
    system_w.field_0x2a = 0;
    system_w.field_0x7c8 = 0;
    system_w.field_0x7c9 = 0;
    system_w.field_0x7ca = 0;
    system_w.field_0x7cb = 0;
    system_w.field_0x7cc = 0;
    system_w.field_0x7cd = 0;
    pad_connect_reset();
    my_player_no_set(0);
}

/* 0x800CF0F0 (0x64): Stops the sound, releases both stage area slots and resets everything back to the title. */
void game_reset_to_title(void) {
    s32 i;

    sound_frame_entry();
    bgm_stop_all();
    pad_motor_stop_all();
    for (i = 0; i < 2; i++) {
        stage_area_slot_release(i);
    }
    system_w_clr();
    all_reset();
    task_slot_table_clear();
    fn_800CE610();
    arena_task_start();
    fn_800D06D0(0);
}

/* 0x800CF154 (0x60): Stops the sound, releases both stage area slots and resets the whole system and the loader. */
void system_full_reset(void) {
    s32 i;

    sound_frame_entry();
    bgm_stop_all();
    pad_motor_stop_all();
    for (i = 0; i < 2; i++) {
        stage_area_slot_release(i);
    }
    system_w_clr();
    all_reset();
    task_slot_table_clear();
    fn_800CE610();
    fn_800D06D0(0);
}

/* 0x800CF1B4 (0x54): Stops the sound, releases both stage area slots and resets the scene's systems. */
void system_scene_reset(void) {
    s32 i;

    sound_frame_entry();
    bgm_stop_all();
    pad_motor_stop_all();
    for (i = 0; i < 2; i++) {
        stage_area_slot_release(i);
    }
    all_reset();
    fn_800D06D0(0);
}

/* 0x800CF280 (0x44): Whether the quest move work is in its state 1. */
u32 move_work_state_ck(void) {
    SysQuestMove* quest = (SysQuestMove*)get_move_work_adrs(0);

    if (quest == NULL) {
        return 0;
    }
    return quest->state_0x113 == 1;
}

/* 0x800CF2C4 (0xC0): Ends the current move: in a quest it raises the sub-state (state 1) or the result (state 7)
 * with its menu and BGM; in the lobby it marks the exit and counts it. */
void ef_move_state_dispatch(u8 mode) {
    u8 game = GameMode_ck();
    SysQuestMove* quest;

    PlayMode_ck();
    switch (game) {
    case 1:
        if (move_work_state_ck() == 1) {
            quest_move_sub_state_4_set(mode);
            break;
        }
        quest = (SysQuestMove*)get_move_work_adrs(0);
        if (quest != NULL) {
            PlayMode_ck();
            quest->result_0x0FA = 7;
            menu_item_close();
            snd_quest_result_bgm_set();
        }
        break;
    case 2:
        get_now_mapno();
        lobby_w.field_0x0B0 = 1;
        system_w.field_0x7ce++;
        break;
    }
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
u8 system_lobby_flag_get(void) {
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
u32 str_tbl_skill_get(u32 id) {
    return ((StrTbl*)get_str_tbl_ptr())->field_0x90[(u16)id];
}

u32 str_tbl_course_get(u32 id) {
    return ((StrTbl*)get_str_tbl_ptr())->field_0xfc[(u16)id];
}

u32 str_tbl_item_kind_get(u32 id) {
    return ((StrTbl*)get_str_tbl_ptr())->field_0x94[(u16)id];
}

/* 0x800D2F1C - step the two stream counters. */
void system_stream_count_step(void) {
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

/* 0x800CE6A0 (0x168): Steps the background loader: opens the head request, reads it asynchronously, and once the
 * read finishes flushes it, runs its callback and drops it from the queue. */
void fn_800CE6A0(void) {
    SysSideTable* table = &lbl_80694C68;
    SysEntry* entry = table->buf->entries;
    DVDFileInfo info;
    u32 size;

    switch (table->state) {
    case 0:
        if (table->count == 0) {
            break;
        }
        table->state = 1;
    case 1:
        if (!DVDOpen(entry->path.text, &info)) {
            for (;;) {
            }
        }
        size = (info.length + 0x1F) & ~0x1F;
        if (entry->size < size) {
            for (;;) {
            }
        }
        table->frames = 0;
        dvd_file_info_copy(&table->file, &info);
        table->state = 2;
        DVDReadAsyncPrio(&table->file, entry->buffer, size, 0, (DVDCallback)fn_800CE680, 2);
    case 2:
        table->frames++;
        break;
    case 3:
        DVDClose(&table->file);
        DCFlushRange(entry->buffer, entry->size);
        if (entry->state == 2) {
            table->frames = 0;
            if (entry->callback != NULL) {
                entry->callback(entry->path.text, entry->buffer, entry->argc, entry->args.arg);
            }
            load_queue_pop();
        }
        lbl_80694C68.state = lbl_80694C68.count != 0;
        break;
    }
}

/* 0x800CE808 (0x7C): Copies a DVD file record field by field. */
void dvd_file_info_copy(DVDFileInfo* dst, DVDFileInfo* src) {
    *dst = *src;
}

/* 0x800CE884 (0x9C): Drops the head request: shifts the queue down one entry and clears the freed tail. */
void load_queue_pop(void) {
    SysSideTable* table = &lbl_80694C68;
    SysEntry* entry = table->buf->entries;
    s32 i;

    if (table->count != 0) {
        if (table->count != 1) {
            for (i = 0; i < table->count - 1; i++, entry++) {
                load_req_copy(entry, entry + 1);
            }
        }
        memset(entry, 0, sizeof(SysEntry));
        table->count--;
    }
}

/* 0x800CE920 (0xD0): Copies a load request field by field. */
void load_req_copy(SysEntry* dst, SysEntry* src) {
    dst->size = src->size;
    dst->buffer = src->buffer;
    dst->path = src->path;
    dst->callback = src->callback;
    dst->argc = src->argc;
    dst->args = src->args;
    dst->state = src->state;
}

/* 0x800CED10 (0xAC): Loads `path` into `buffer` at once, as `load_file` does; returns the bytes read, 0 on failure. */
/* untyped: byte range - the load destination */
s32 load_file_direct(char* path, void* buffer, u32 size) {
    DVDFileInfo info;
    u32 length;

    movie_drive_wait(NULL);
    if (!DVDOpen(path, &info)) {
        return 0;
    }
    length = (info.length + 0x1F) & ~0x1F;
    if (size < length) {
        return 0;
    }
    size = dvd_read_wait(&info, buffer, length, 0);
    DVDClose(&info);
    DCFlushRange(buffer, size);
    return size;
}

/* 0x800CEDD4 (0x58): Reads `length` bytes of an open file at `offset` and waits for it; returns the bytes read
 * (the file's length when shorter). */
/* untyped: byte range - the read destination */
s32 dvd_read_wait(DVDFileInfo* info, void* buffer, s32 length, s32 offset) {
    s32 read;

    DVDReadAsyncPrio(info, buffer, length, offset, NULL, 2);
    movie_drive_wait(info);
    read = info->length;
    if (read > length) {
        read = length;
    }
    return read;
}

/* 0x800CEE2C (0x48): Opens `path` into `info` once the drive is idle. */
BOOL dvd_open_wait(char* path, DVDFileInfo* info) {
    movie_drive_wait(NULL);
    return DVDOpen(path, info);
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
void fn_800CFBA0(u8* color) {
    color_rgba_copy((u8*)&system_w.field_0x7bc, color);
    system_w.field_0x7bc.bytes[3] = 0xFF;
}

/* 0x800CFBE0 - the stream record's word. */
u32 fn_800CFBE0(void) {
    return system_w.field_0x7bc.value;
}

/* 0x800CFD00 - arm the two stream bounds. */
void fn_800CFD00(void) {
    system_w.field_0x878 = system_w.field_0x4c;
    system_w.field_0x87c = system_w.field_0x4c + lbl_80791358[0];
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
    sys_work_stack.field_0x08 = sys_work_stack.head;
    sys_work_stack.cur_0x0C = sys_work_stack.head;
    sys_work_stack.used_0x10 = 0;
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
u32 sys_tex_image_get(s32 index) {
    if (index >= 0x20) {
        return 0;
    }
    if (system_w.tasks[index].active == 0) {
        return 0;
    }
    return system_w.tasks[index].field_0x24;
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
    return sys_loading.shown_0x00 != 0;
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

/* 0x800CF3E4 (0x238): Lays the game's memory out: the resident blocks from MEM2 0x90048000 (user data, font,
 * loader work, the fixed work areas and the primitive pool), the arena regions, then clears them and builds the
 * work heap. */
void memory_layout_init(void) {
    volatile u32 sizes[11];
    u32 addr;
    u32 base;

    sizes[0] = 0;
    addr = 0x90048000;
    system_w.layout_0x940 = addr;
    addr += (sizes[1] = userdata_size_get()) * 2;
    system_w.layout_0x944 = addr;
    addr += (sizes[2] = font_buffer_size_get());
    system_w.field_0x948 = (void*)addr;
    addr += (sizes[3] = fn_800CE5A8());
    system_w.layout_0x94c = addr;
    addr += (sizes[4] = 0x29088);
    base = ((addr - 0x90048000 + 0x1F) & ~0x1F) + 0x90048000;
    system_w.layout_0x950 = base;
    system_w.layout_0x954 = base + (sizes[5] = 0x14000);
    system_w.layout_0x958 = base + 0x14000 + (sizes[6] = 0x29670);
    system_w.layout_0x95c = base + 0x14000 + 0x29670 + (sizes[7] = 0x97C0);
    base += 0x14000 + 0x29670 + 0x97C0 + (sizes[8] = 0x6000);
    system_w.layout_0x960 = base;
    base += (sizes[9] = 0xC4B0);
    system_w.layout_0x968 = base;
    sizes[10] = 0xEA00;
    prim_rec_pool[0] = base;
    prim_rec_pool[1] = base + 0x1A00;
    system_w.field_0x70 = 0x300000;
    system_w.field_0x6c = 0x90348000;
    system_w.field_0x74 = 0;
    system_w.field_0x50 = 0x380000;
    system_w.field_0x4c = 0x907C8000;
    system_w.field_0x5c = 0x400000;
    system_w.field_0x58 = 0x90B48000;
    system_w.field_0x7c = 0x01B59C00;
    system_w.field_0x78 = 0x90F48000;
    system_w.stream_area_0x44 = 0x90348000;
    system_w.stream_area_size_0x48 = 0x025D9C00;
    system_w.field_0x64 = 0x1C0000;
    system_w.field_0x60 = 0x90148000;
    system_w.mem1_size_0x84 = 0x280000;
    system_w.mem1_0x80 = 0x80FF6600;
    system_w.arena_0x3c = 0x90148000;
    system_w.arena_size_0x40 = 0x800000;
    memset((void*)system_w.field_0x4c, 0, system_w.field_0x50);
    memset((void*)system_w.field_0x60, 0, system_w.field_0x64);
    memset((void*)system_w.field_0x6c, 0, system_w.field_0x70);
    memset((void*)system_w.mem1_0x80, 0, system_w.mem1_size_0x84);
    system_w.field_0x54 = 0;
    system_w.field_0x90 = MEMCreateExpHeapEx(system_w.field_0x60, system_w.field_0x64, 4);
    MEMInitAllocatorForExpHeap(&system_w.mem_allocator, system_w.field_0x90, 0x20);
    system_w.field_0x68 = fn_800CF61C(system_w.field_0x90);
    nw_resource_init();
    pad_module_init();
    fn_800CFD20();
}

#pragma push
#pragma peephole off
/* Swaps a big-endian word of an archive header. */
static inline u32 sys_swap32(s32 v) {
    return ((u32)v >> 24) | ((v & 0xFF0000) >> 8) | (((v & 0xFF) << 24) | ((v & 0xFF00) << 8));
}

/* 0x800CF698 (0x30): Returns an archive's entry count (stored big-endian). */
u32 pack_entry_count(SysArchive* arc) {
    return sys_swap32(arc->count);
}

/* 0x800CF6C8 (0x3C): Returns the address of an archive's entry `index`. */
u8* pack_entry_data(SysArchive* arc, s32 index) {
    return (u8*)arc + sys_swap32(arc->entries[index].offset);
}

/* 0x800CF704 (0x38): Returns the size of an archive's entry `index`. */
u32 pack_entry_size(SysArchive* arc, s32 index) {
    return sys_swap32(arc->entries[index].size);
}
#pragma pop

/* 0x800CF858 (0x94): Releases every enemy work record and the move work, rebuilds the work heap and resets the player
 * collision. */
void enemy_work_release_all(void) {
    SystemWork* sys = &system_w;
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 max = get_move_work_max(3);
    s32 i;

    for (i = 0; i < max; i++, work++) {
        if (work != NULL) {
            enemy_work_release(work);
        }
    }
    sys->field_0xa4 = NULL;
    fn_800CF73C();
    pl_coll_table_clear();
}

/* 0x800CF948 (0x148): Allocates move work `kind` for `count` records (kind 2 is the player array) and records it. */
void move_work_kind_alloc(u8 kind, u16 count) {
    u32 size;
    void* records;

    if (system_w.field_0xa4 == NULL || kind >= 7 || count == 0) {
        return;
    }
    switch (kind) {
    case 0:
        size = count * 0x22E8;
        break;
    case 1:
        size = count * 0x24;
        break;
    case 2:
        records = (void*)player_work_array_get();
        system_w.field_0xa4->max[kind] = count;
        system_w.field_0xa4->addr[kind] = records;
        return;
    case 3:
        size = count * 0xB18;
        break;
    case 4:
        size = count * 0x48;
        break;
    case 5:
        size = count * 0x168;
        break;
    case 6:
        size = count * 0x41;
        break;
    default:
        return;
    }
    records = work_mem_alloc(size);
    if (records != NULL) {
        system_w.field_0xa4->max[kind] = count;
        system_w.field_0xa4->addr[kind] = records;
        memset(records, 0, size);
    }
}

/* 0x800CFBF0 (0x74): Loads the resident file into the top 0xA900 bytes of the +0x4C region and binds it. */
void resident_file_load(void) {
    u32 file = system_w.field_0x4c + system_w.field_0x50 - 0xA900;

    system_w.field_0x880 = file;
    load_file(sys_resident_path, file, 0xA900);
    tex_slot_free(0, 5);
    tex_palette_bind((TPLPalette*)file, 0);
}

/* 0x800CFD3C (0x74): Clears the stream ring and spans it over the +0x4C region. */
void stream_ring_reset(void) {
    u32 head;
    u32 size;

    memset(&sys_work_stack, 0, 0x664);
    sys_work_stack.head = head = system_w.field_0x4c;
    sys_work_stack.size = size = system_w.field_0x50;
    sys_work_stack.field_0x08 = head;
    sys_work_stack.cur_0x0C = head;
    sys_work_stack.used_0x10 = 0;
    sys_work_stack.end_0x14 = head + size;
    sys_work_stack.top_0x18 = head + size;
    sys_work_stack.used_top_0x1C = 0;
}

/* 0x800CFDD0 (0x108): Takes `size` bytes of the work stack: from the bottom (recorded under `name`, at most 64
 * blocks) or, with `top`, from the top; 0 when it does not fit. */
u32 work_stack_alloc(u32 size, u8 top, char* name) {
    u32 addr;
    u32 end;

    if (top == 0) {
        if (sys_work_stack.count_0x20 >= 64) {
            return 0;
        }
        addr = sys_work_stack.cur_0x0C;
        end = addr + size;
        if (end > sys_work_stack.top_0x18) {
            return 0;
        }
        sys_work_stack.used_0x10 += size;
        sys_work_stack.cur_0x0C = end;
        sys_work_stack.live_0x21[sys_work_stack.count_0x20] = 1;
        sys_work_stack.addr_0x64[sys_work_stack.count_0x20] = addr;
        sys_work_stack.size_0x164[sys_work_stack.count_0x20] = size;
        strcpy(sys_work_stack.name_0x264[sys_work_stack.count_0x20].text, name);
        sys_work_stack.count_0x20++;
        return addr;
    }
    if (sys_work_stack.top_0x18 - size < sys_work_stack.cur_0x0C) {
        return 0;
    }
    sys_work_stack.used_top_0x1C += size;
    return sys_work_stack.top_0x18 += size;
}

/* 0x800CFED8 (0x88): Returns the index of the live work-stack block named `name`, -1 when none. */
s32 work_stack_find(char* name) {
    s32 i;
    SysStackName* entry = sys_work_stack.name_0x264;
    s32 count = sys_work_stack.count_0x20;

    for (i = 0; i < count; i++, entry++) {
        if (sys_work_stack.live_0x21[i] != 0 && strcmp(entry->text, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* 0x800CFF60 (0xAC): Frees work-stack block `index` when it is the last one; 1 when it was. */
s32 work_stack_free_index(s32 index) {
    u8 last;

    if (sys_work_stack.live_0x21[index] == 0) {
        return 0;
    }
    if ((last = sys_work_stack.count_0x20 - 1) != index) {
        return 0;
    }
    sys_work_stack.count_0x20 = last;
    sys_work_stack.used_0x10 -= sys_work_stack.size_0x164[last];
    sys_work_stack.cur_0x0C -= sys_work_stack.size_0x164[last];
    sys_work_stack.live_0x21[last] = 0;
    sys_work_stack.addr_0x64[sys_work_stack.count_0x20] = 0;
    sys_work_stack.size_0x164[sys_work_stack.count_0x20] = 0;
    sys_work_stack.name_0x264[sys_work_stack.count_0x20].text[0] = 0;
    return 1;
}

/* 0x800D000C (0xE0): Frees the work-stack block at `addr` when it is the last one; 1 when it was. */
s32 work_stack_free(u32 addr) {
    s32 i;
    u8 last;

    for (i = 0; i < sys_work_stack.count_0x20; i++) {
        if (sys_work_stack.live_0x21[i] != 0 && addr == sys_work_stack.addr_0x64[i]) {
            if (i != sys_work_stack.count_0x20 - 1) {
                return 0;
            }
            last = --sys_work_stack.count_0x20;
            sys_work_stack.used_0x10 -= sys_work_stack.size_0x164[last];
            sys_work_stack.cur_0x0C -= sys_work_stack.size_0x164[last];
            sys_work_stack.live_0x21[last] = 0;
            sys_work_stack.addr_0x64[sys_work_stack.count_0x20] = 0;
            sys_work_stack.size_0x164[sys_work_stack.count_0x20] = 0;
            sys_work_stack.name_0x264[sys_work_stack.count_0x20].text[0] = 0;
            return 1;
        }
    }
    return 0;
}

/* 0x800D00EC (0x24): Resets the work stack and rebuilds the resident blocks. */
void work_stack_reset(void) {
    stream_ring_reset();
    fn_800CFC64();
}

/* Builds texture slot `slot` from palette entry `id` (its image parameters and the GX object). */
static inline void sys_tex_slot_init(SysTask* slot, TPLPalette* pal, u32 id, u32 filter) {
    TPLDescriptor* desc = TPLGet(pal, id);

    slot->field_0x24 = (u32)desc->textureHeader->data;
    slot->width_0x28 = desc->textureHeader->width;
    slot->height_0x2A = desc->textureHeader->height;
    slot->format_0x2C = desc->textureHeader->format;
    slot->wrap_s_0x30 = desc->textureHeader->wrapS;
    slot->wrap_t_0x34 = desc->textureHeader->wrapT;
    GXInitTexObj((GXTexObj*)slot->body, (void*)slot->field_0x24, slot->width_0x28, slot->height_0x2A,
                 slot->format_0x2C, slot->wrap_s_0x30, slot->wrap_t_0x34, 0);
    GXInitTexObjLOD((GXTexObj*)slot->body, filter, filter, 0.0f, 0.0f, 0.0f, 0, 0, 0);
    slot->active = 1;
}

/* 0x800D0110 (0x1B0): Binds palette `pal` and builds its textures into the free slots from `index` (the special
 * slots 0xFD..0xFF take one; slot 5's third texture goes to slot 11), linear-filtered; returns the count built. */
s32 tex_palette_bind(TPLPalette* pal, s32 index) {
    SysTask* slot;
    s32 built;
    u32 i;

    TPLBind(pal);
    built = 0;
    switch (index) {
    case 0xFF:
        slot = &system_w.tex_0x96c[0];
        break;
    case 0xFE:
        slot = &system_w.tex_0x96c[1];
        break;
    case 0xFD:
        slot = &system_w.tex_0x96c[2];
        break;
    default:
        if (index >= 0x20) {
            return 0;
        }
        slot = &system_w.tasks[index];
        break;
    }
    for (i = 0; i < pal->numDescriptors; i++, slot++) {
        if ((u32)(index - 0xFD) <= 2) {
            if (slot->active != 0) {
                break;
            }
        } else {
            if (index + i >= 0x20) {
                break;
            }
            if (slot->active != 0) {
                continue;
            }
        }
        if (index == 5 && i == 2) {
            slot = &system_w.tasks[11];
        }
        sys_tex_slot_init(slot, pal, i, 1);
        built++;
        if (index == 0xFF) {
            break;
        }
    }
    return built;
}

/* 0x800D02C0 (0x108): Binds palette `pal` and builds every texture into the slots from `slot`; returns the count. */
s32 fn_800D02C0(TPLPalette* pal, SysTask* slot) {
    s32 built;
    u32 i;

    TPLBind(pal);
    built = 0;
    for (i = 0; i < pal->numDescriptors; i++, slot++) {
        sys_tex_slot_init(slot, pal, i, 1);
        built++;
    }
    return built;
}

/* 0x800D03C8 (0x1A0): As `tex_palette_bind` with the special slots 0xFE/0xFF only, point-filtered. */
s32 tex_palette_bind_point(TPLPalette* pal, s32 index) {
    SysTask* slot;
    s32 built;
    u32 i;

    TPLBind(pal);
    built = 0;
    if (index == 0xFF) {
        slot = &system_w.tex_0x96c[0];
    } else if (index == 0xFE) {
        slot = &system_w.tex_0x96c[1];
    } else {
        if (index >= 0x20) {
            return 0;
        }
        slot = &system_w.tasks[index];
    }
    for (i = 0; i < pal->numDescriptors; i++, slot++) {
        if (index == 0xFF || index == 0xFE) {
            if (slot->active != 0) {
                break;
            }
        } else {
            if (index + i >= 0x20) {
                break;
            }
            if (slot->active != 0) {
                continue;
            }
        }
        if (index == 5 && i == 2) {
            slot = &system_w.tasks[11];
        }
        sys_tex_slot_init(slot, pal, i, 0);
        built++;
        if (index == 0xFF) {
            break;
        }
    }
    return built;
}

/* 0x800D0618 (0xA0): Frees `count` texture slots from `index`. */
void tex_slot_free(s32 index, s32 count) {
    SysTask* slot;
    s32 i;

    if (index >= 0x20) {
        return;
    }
    slot = &system_w.tasks[index];
    for (i = 0; i < count; i++, slot++) {
        if (index + i >= 0x20) {
            break;
        }
        if (slot->active != 0) {
            memset(slot, 0, sizeof(SysTask));
        }
    }
}

/* 0x800D2094 (0x4): Does nothing. */
void system_nop(void) {
}

/* 0x800D20CC (0xC): Sets a texture coordinate generation without normalisation or post-transform. */
void tex_coord_gen_set(s32 dst, s32 func, s32 src, u32 mtx) {
    GXSetTexCoordGen2(dst, func, src, mtx, 0, 0x7D);
}

/* 0x800D20F0 (0x18): Mutes (0) or restores (non-zero) the sound volume. */
void sound_volume_mute_set(u32 on) {
    if (on == 0) {
        stream_volume_slot_set(0);
    } else {
        stream_volume_slot_set(0x7F);
    }
}

/* 0x800D0FCC (0x80): Sets the loading display's caption (at most 42 characters; an empty text clears it). */
void loading_caption_set(char* text) {
    SysLoading* disp = &sys_loading;
    s32 len = flfntStrLen(text);

    if (len == 0) {
        disp->text_0xA7[0] = 0;
    } else if (len > 42) {
        strncpy(disp->text_0xA7, text, 42);
        disp->text_0xA7[42] = 0;
    } else {
        strcpy(disp->text_0xA7, text);
    }
}

/* 0x800D0764 (0x21C): Resets the loading display to style `mode`: its sprites and their positions for the current
 * language, the spinner quad, and (except in style 6) loads its texture file. */
void loading_disp_reset(u8 mode) {
    SysLoading* disp = &sys_loading;
    SysLoadingPos* pos;
    u8 count;
    s32 i;
    char path[0x80];

    disp->shown_0x00 = 0;
    disp->prev_0x01 = 0;
    disp->field_0x02 = 0;
    disp->field_0x03 = 0;
    disp->frame_0x06 = 1;
    disp->field_0x04 = 0;
    disp->mode_0xA6 = mode;
    disp->text_0xA7[0] = 0;
    pos = sys_loading_pos[system_w.field_0x09];
    count = sys_loading_counts[system_w.field_0x09];
    for (i = 0; i < count; i++) {
        disp->sprite_0x08[i] = sys_loading_sprites[i];
        disp->x_0x7A[i] = pos[i].x;
        disp->y_0x90[i] = pos[i].y;
        disp->timer_0x64[i] = 0;
    }
    disp->quad_0x34[0].x = -32.0f;
    disp->quad_0x34[0].y = -32.0f;
    disp->quad_0x34[0].z = 0.0f;
    disp->quad_0x34[1].x = 32.0f;
    disp->quad_0x34[1].y = -32.0f;
    disp->quad_0x34[1].z = 0.0f;
    disp->quad_0x34[2].x = 32.0f;
    disp->quad_0x34[2].y = 32.0f;
    disp->quad_0x34[2].z = 0.0f;
    disp->quad_0x34[3].x = -32.0f;
    disp->quad_0x34[3].y = 32.0f;
    disp->quad_0x34[3].z = 0.0f;
    if (disp->mode_0xA6 != 6) {
        disp->texture_0xD4 = 0x90F08000;
        cnvt_eur_fname(path, (char*)sys_tex_file_table[2].name);
        load_file(path, disp->texture_0xD4, sys_tex_file_table[2].size);
        TPLtexLoad((void*)disp->texture_0xD4, &disp->tex_0xD8);
    }
}

/* 0x800CFC64 (0x9C): Arms the two stream bounds below the resident file, frees texture slots 5..6 and 8, and loads
 * the common texture file into slot 8. */
void fn_800CFC64(void) {
    u32 end = system_w.field_0x4c + system_w.field_0x50 - 0xA900;

    system_w.field_0x878 = end -= lbl_80791358[0];
    system_w.field_0x87c = end - lbl_80791358[1];
    tex_slot_free(5, 2);
    tex_slot_free(8, 1);
    load_file(sys_common_tex_path, 0x90348000, 0x322C0);
    tex_palette_bind((TPLPalette*)0x90348000, 8);
}

/* 0x800D2F74 (0x14): Builds system message `id` into `buf`. */
void sys_msg_build(char* buf, long id) {
    wii_sysmsg_gen(id, buf, 0);
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

/* 0x800CEF9C (0xC0): Resets every game system (primitives, sounds, effects, enemies, the player, camera, light,
 * filters, the loader and the network work, fade, user data and pads) and the system flags. */
void all_reset(void) {
    u32 color;

    prim_init_all();
    sound_free_lists_reset();
    eft_res_release_all();
    effect_layer_reset();
    eft_res_pool_reset();
    em_model_pool_reset();
    player_control_release();
    em_prog_support_reset();
    camera_work_init();
    light_work_reset();
    light_init();
    filter_reset();
    fn_800CEF60();
    enemy_work_release_all();
    work_stack_reset();
    nwWorkInitialize();
    color = 0;
    fn_800CFBA0((u8*)&color);
    fade_reset(0);
    filter_flag_clear();
    userdata_frame_reset();
    system_w.field_0x2a = 0;
    system_w.field_0x7c8 = 0;
    system_w.field_0x7c9 = 0;
    system_w.field_0x7ca = 0;
    system_w.field_0x7cb = 0;
    system_w.field_0x7cc = 0;
    system_w.field_0x7cd = 0;
    system_w.field_0x877 = 0;
    pad_connect_reset();
    draw_shape_reset();
    fn_800CFDB0();
    snd_bank_loader_reset();
    pad_motor_stop_all();
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

/* 0x800CEA34 (0x7C): Whether the background loader is busy; while it reads, copies the file's path and the frames
 * it has taken. */
BOOL file_loading_ck(char* path, long* frames) {
    if (lbl_80694C68.state == 0) {
        return FALSE;
    }
    if (lbl_80694C68.state == 2) {
        if (path != NULL) {
            strcpy(path, lbl_80694C68.buf->entries[0].path.text);
        }
        if (frames != NULL) {
            *frames = lbl_80694C68.frames;
        }
    }
    return TRUE;
}

#pragma push
#pragma peephole off
/* 0x800CEAB0 (0x1B4): Queues a background load of `path` into `buffer` (`size` bytes) with the completion callback
 * and up to 16 of its arguments; 0 when the request is invalid or the queue is full. */
BOOL load_file_req(char* path, unsigned long buffer, long size, unsigned long callback, long argc,
                   unsigned long* args) {
    SysEntry* entry;
    s32 i;

    if (*path == 0) {
        return FALSE;
    }
    if (size <= 0) {
        return FALSE;
    }
    if (buffer == 0) {
        return FALSE;
    }
    if ((s32)strlen(path) >= 0x100) {
        return FALSE;
    }
    entry = fn_800CE9F0();
    if (entry == NULL) {
        return FALSE;
    }
    strcpy(entry->path.text, path);
    entry->state = 1;
    entry->buffer = (void*)buffer;
    entry->size = size;
    entry->callback = (SysLoadCallback)callback;
    if (argc > 16) {
        argc = 16;
    }
    entry->argc = argc;
    if (args != NULL) {
        for (i = 0; i < argc; i++) {
            entry->args.arg[i] = *args++;
        }
    }
    return TRUE;
}
#pragma pop

/* 0x800CEC64 (0xAC): Loads `path` into `buffer` at once (the buffer must hold the 32-byte-rounded file); returns the
 * bytes read, 0 on failure. */
long load_file(char* path, unsigned long buffer, long size) {
    DVDFileInfo info;
    u32 length;

    movie_drive_wait(NULL);
    if (!DVDOpen(path, &info)) {
        return 0;
    }
    length = (info.length + 0x1F) & ~0x1F;
    if ((u32)size < length) {
        return 0;
    }
    size = dvd_read_wait(&info, (void*)buffer, length, 0);
    DVDClose(&info);
    DCFlushRange((void*)buffer, size);
    return size;
}

/* 0x800CFB2C (0x74): Builds the move work of scene kind `kind` (one record count per move-work kind). */
void create_move_work(long kind) {
    u16* counts;
    s32 i;

    if (kind >= 9) {
        return;
    }
    fn_800CF8EC();
    counts = move_work_counts[kind];
    for (i = 0; i < 7; i++, counts++) {
        if (*counts != 0) {
            move_work_kind_alloc(i, *counts);
        }
    }
}

/* 0x800D06B8 - TPL texture load entry. */
s32 TPLtexLoad(void* data, _tex_info* info) {
    if (data == 0) {
        return -1;
    }
    return fn_800D02C0((TPLPalette*)data, info);
}

#pragma push
#pragma peephole off
/* 0x800D0F14 (0xB8): Shows (`on`) or hides the loading display in style `mode` (rebuilding its sprites when it
 * appears) and re-arms both pad motors. */
void loading_disp_set(u8 on, u8 mode) {
    SysLoading* disp = &sys_loading;

    disp->prev_0x01 = disp->shown_0x00;
    disp->mode_0xA6 = mode;
    if (mode == 0) {
        disp->text_0xA7[0] = 0;
    }
    if (on == 0) {
        disp->shown_0x00 = 0;
        system_w.field_0x7dc[0] = 0xFF;
        system_w.field_0x7dc[1] = 0xFF;
    } else {
        if (on != disp->prev_0x01) {
            loading_disp_reset(mode);
            system_w.field_0x7dc[0] = 0xFF;
            system_w.field_0x7dc[1] = 0xFF;
        }
        disp->shown_0x00 = 1;
    }
    fn_800D2954(0);
    fn_800D2954(1);
}
#pragma pop

/* 0x800D2E34 (0x34): Returns string table `index` of the message tables. */
u8** get_str_tbl(long index) {
    return ((u8***)get_str_tbl_ptr())[index];
}
