/*
 * menu/menu_plsearch.cpp - the save-data code (the NAND step wrappers, `createSystemFile`, `createDataFile`,
 *   `readDataFile`, the banner, account and boot files, `setPlayerSave2Userdata`), the player search, friend list and
 *   mailbox pages, the lobby message menu, the lobby hunter card, the gallery demo task and the P-Mic terms object
 *   (`PatTerms`).
 * RANGE. .text 0x80448404-0x804513FC (118 functions); extab 0x8001DF24-0x8001E18C, extabindex 0x8003EBEC-0x8003EF88,
 *   .data 0x806073F0-0x80607E50, .bss 0x806E40C0-0x806F4658, .sdata 0x80793B88-0x80793C10, .sbss
 *   0x80794D50-0x80794D64.  The left edge is `quest/arenatask.cpp`'s end, the right one the `.data` vtable-then-string
 *   seam at 0x80607E50 (`menu/menu_sysmsg.cpp`).
 * FLAGS. `cflags_menu` (configure.py); file-scope `#pragma pool_data off` (pooled, MWCC addresses the save code's
 *   three `.bss` objects through one `...bss.0` base, which retail does not).
 * NAMES. Module and file from the `__FILE__` string "menu_plsearch.cpp".  `sPatTerms` (.sbss 0x80794D58) is a GUESS:
 *   the terms object's constructor publishes itself there.
 *   GUESS (from each body and its callers): game_data_file_create_start, game_system_file_create_start,
 *   game_system_file_create_wait, `plsearch_work_init`, `plsearch_step`, `plsearch_draw`.
 *   GUESS (save code, from the NAND call each wraps and the file each builds): `save_nand_*` (the step wrappers and
 *   their init halves), `save_space_check_*`, `system_file_*`, `data_file_*`, `banner_file_*`, `account_file_*`,
 *   `save_ctl_reset`, `save_error_get`, `save_check_blocks`/`save_check_inodes`/`save_data_slot_max`; the objects
 *   `save_nand_task`, `save_ctl`, `save_banner`, `save_banner_comment`, `save_data_file_names`, `save_banner_icon_anim`.
 *   GUESS (menus, from the page each steps or draws): `plsearch_search_draw`,
 *   `plsearch_friend_list_draw`, `plsearch_empty_draw`, `mailbox_*`, `plsearch_address_step`,
 *   `plsearch_main_draw`, `plsearch_window_draw`, `plsearch_choice_draw`, `plsearch_mail_body_clear`,
 *   `lobby_card_view_*`, `gallery_demo_running_ck`, `gallery_demo_start_step`; the objects
 *   `plsearch_message_work`, `plsearch_kbd_text`.
 *   GUESS (P-Mic): `patTermsPmicCallback`, `setPatTermsResult`, `patTermsEndThread`, `clearPatTerms`,
 *   `resetPatTermsState`.  `plsearch_page11_*`/`plsearch_page19_*`/`plsearch_page22_*` are the map's names (the
 *   page-table entries `menu/menu_item.cpp` calls through their leaf headers).
 * RESIDUALS. The range is at least four TUs (`.data` evidence), so the unit cannot flip until it is re-cut: the save
 *   code (0x80448404-0x8044BDE8, strings 0x806073F0-0x80607678), `menu_plsearch.cpp` (0x8044BDE8-0x8044E2CC, its
 *   `__FILE__` at 0x80607678), `menu_message.cpp` (0x8044E2CC-0x804504A0, its `__FILE__` at 0x80607870) and the P-Mic
 *   terms object (0x804504A0-0x804513B0, strings 0x806079F0-0x80607E50); `.sdata` repeats "%s" (0x80793BE8,
 *   0x80793C08), one copy per TU.  0x804513B0-0x804513FC is `menu/menu_sysmsg.cpp`'s (it reads that unit's `.sbss`
 *   0x80794DCC and is called from it; 0x804513F4/0x804513F8 are entries of its table 0x8060E700); `.sbss` 0x80794D60
 *   is `homebutton/hkb_widget_tail.c`'s (its only readers).
 *  - Unwritten: 0x8044F7FC-0x8044FB98 (the gesture page step and draw: they call `Pl/pl_act.cpp`'s unnamed
 *    0x80274584/0x80274624/0x80274904 and `lobby/lb_npc.cpp`'s unnamed 0x8020D624), 0x8044FC2C-0x8044FEF8
 *    (`GalleryDemo_task`) and 0x8044FF18-0x804504A0 (its load and run steps: fourteen unnamed callees), 0x804513B0-0x804513FC.
 *  - `plsearch_page19_step`: retail's step 40 reaches its profile-resend and command-18 tails by jumps from three
 *    sub-states; the source repeats the tails (rule 8), so the function is 0x78 bytes longer.
 *  - `plsearch_draw`, `plsearch_page19_open`, `plsearch_page22_draw`: retail tests its case ranges as `addi`+`cmplwi`
 *    or as two separate compares where MWCC builds the other form from the same switch/if.
 *  - `save_nand_read_step`: retail tests the read's `>= 0` result inside the `-5` dispatch tree.
 *  - `initPatTerms` (`bne`+`blr` vs `beqlr`), `resetPatTermsState` (the 0 is materialised after the state test),
 *    `writePatTermsVoice` (`getPatTermsFlag`'s `u8` return gets a `clrlwi`), `lb_plsearch_open` (a `clrlwi` on
 *    the server type index), `mailbox_draw`/`mailbox_reply_draw`/`plsearch_search_draw` (register colouring).
 *  - Data: `.data`, `.sdata`, `.bss` and the vtable are emitted, in this file's order rather than the four TUs';
 *    nine callees are registry names not yet in the map (`sys_tex_image_get`, `userdata_save_export`/`_import`,
 *    `friend_pick_*`, `friend_card_view`, `hud_mail_notice_clear`, `card_equip_ids_build`).
 * SHAPES. The NAND wrappers dispatch the call's result with `case 0` first and the busy cases falling into `default`;
 *   the file checksums walk a `bytes` pointer (`bytes = ctl->buffer; sum = 0; bytes += 8;` then `sum += *bytes;
 *   bytes++`); the file steps reach `save_ctl` through a `ctl` pointer but read `step` directly.  The draws pass the
 *   anchor `pos` by value to `put_lsp_anchor_offset` and index a `rows` table local; their colour flags are `bool`.
 */

#include "types.h"
#include "NAND/nand.h"
#include "OS/OSError.h"
#include "MSL_C/string.h"
#include "MSL_C/strcat.h"
#include "MSL_C/printf.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "mh3_pad/system_w.h"
#include "mh3_pad/option_w.h"
#include "menu/get_pop_dat_ptr.h"
#include "Network/network_pat_control.h"
#include "userdata_save_export.h"
#include "userdata_save_import.h"
#include "ef/sys_tex_image_get.h"
#include "PMIC/pmic.h"
#include "OS/OSThread.h"
#include "Network/NetworkStreamSink.h"
#include "Network/NetworkWiiMediator.h"
#include "unsplit/Network.h"
#include "menu/PatTerms.h"
#include "mh3_pad/Psw.h"
#include "hud/cockpit.h"
#include "nw4r/math.h"
#include "sound/fn_800D7F54.h"
#include "font/flfnt.h"
#include "fn_80040598.h"
#include "menu/menu_item.h"
#include "menu/menu_message.h"
#include "Network/net_session_close.h"
#include "nw4r/db_assert.h"
#include "ef/get_move_work_adrs.h"
#include "ef/system_core.h"
#include "enemy/lobby_state_block.h"
#include "enemy/em020_prog.h"
#include "menu/movie.h" /* the friend picker (rule 2: its owner) */
#include "menu/menu_friend_list_init.h"
#include "menu/friend_card_view.h"
#include "lobby/lb_npc.h"
#include "ef/GameMode_ck.h"
#include "hud/hud_mail_notice_clear.h"
#include "menu/put_menu_cursor.h"
#include "menu/card_equip_ids_build.h"
#include "lobby/hud_key_lookup.h"
#include "lobby/lb_panel_close.h"
#include "lobby/lobby_w.h"
#include "userdata_item.h"
#include "quest/quest_types.h"
#include "menu/demo_play_ck.h"
#include "menu/plsearch_page11_open.h"
#include "menu/plsearch_page19_open.h"
#include "menu/plsearch_page19_step.h"
#include "menu/plsearch_page19_draw.h"
#include "menu/plsearch_page22_open.h"
#include "menu/plsearch_page22_step.h"
#include "menu/plsearch_page22_draw.h"
#include "pl.h"
#include "hud/layout.h"
#include "hud/draw_layout_window.h"
#include "hud/draw_font_align.h"
#include "hud/put_lsp_anchor_offset.h"
#include "lobby/lb_cursor_prompt_draw.h"
#include "menu/menu_plsearch.h"

#pragma pool_data off

/* ---------------------------------------------------------------------------------------------------------------- */
/* The save code. */

/* size: 0x1B0 - the one NAND operation in flight: its step, retry budget and delay, the path, the buffer, the SDK
 * command block and file record, and the completion's result. */
typedef struct SaveNandTask {
    /* +0x000 */ s32 step;               /* 0: issue the call, 1: wait for the completion */
    /* +0x004 */ s32 retries;            /* busy retries left (3) */
    /* +0x008 */ s32 delay;              /* frames to wait before the next try (30 after a busy result) */
    /* +0x00C */ char path[64];
    /* +0x04C */ u8* buffer;             /* the read/write buffer */
    /* +0x050 */ s32 length;             /* the read/write length */
    /* +0x054 */ NANDCommandBlock block;
    /* +0x110 */ NANDFileInfo info;
    /* +0x19C */ u8 done;                /* set by the completion */
    /* +0x1A0 */ s32 result;             /* the completion's result */
    /* +0x1A4 */ u32 answer;             /* the quota check's answer bits */
    /* +0x1A8 */ u32 fs_block;           /* the blocks the quota check asks for */
    /* +0x1AC */ u32 inode;              /* the inodes the quota check asks for */
} SaveNandTask; /* size: 0x1B0 */

/* size: 0x130 - the file operation in flight: its step, the file name, the buffer it builds or reads, the error the
 * NAND step left, and the player/slot of a data file. */
typedef struct SaveCtl {
    /* +0x000 */ s32 step;
    /* +0x004 */ s32 scratch_0x04;       /* cleared by every init; nothing reads it */
    /* +0x008 */ s32 scratch_0x08;
    /* +0x00C */ s32 scratch_0x0C;
    /* +0x010 */ s32 scratch_0x10;
    /* +0x014 */ char file_name[64];
    /* +0x054 */ u8* buffer;             /* `system_w`'s +0x950 work block */
    /* +0x058 */ u32 size;               /* the file size */
    /* +0x05C */ s32 kind;               /* 1 system/account, 4 banner, 5 data */
    /* +0x060 */ s32 error;              /* the NAND result of the failed step */
    /* +0x064 */ s32 verify_error;       /* -1 when the read file's checksum is wrong */
    /* +0x068 */ u8 pad_0x068[0x98];
    /* +0x100 */ s8 player;              /* the data file's index (data00..data05) */
    /* +0x101 */ s8 slot;                /* the save slot (-1 none, -2 a fresh file, 4..6 special reads) */
    /* +0x102 */ u8 banner_exists;       /* set once banner.bin opened */
    /* +0x103 */ u8 pad_0x103;
    /* +0x104 */ s32 read_done;          /* 1 once the old data file was read back */
    /* +0x108 */ s32 corrupt_error;      /* the error that made the data file start over */
    /* +0x10C */ s32 clear;              /* non-zero: the data file is cleared before it is built */
    /* +0x110 */ u32 check_blocks;
    /* +0x114 */ u32 check_inodes;
    /* +0x118 */ u8 pad_0x118[0x18];
} SaveCtl; /* size: 0x130 */

/* size: 0x4000 - the system file. */
typedef struct SystemFile {
    /* +0x00 */ u32 version;             /* 1 */
    /* +0x04 */ u32 checksum;            /* the byte sum of bytes 8..0x3FFF */
    /* +0x08 */ u32 format;              /* 4 */
    /* +0x0C */ u8 pad_0x0C[0x4];
    /* +0x10 */ u8 reserved_0x10[6];
    /* +0x16 */ u8 save_keep[2];         /* `system_w`'s +0x910 pair */
    /* +0x18 */ u32 unlock_seen[2];      /* `system_w`'s +0x888 pair */
    /* +0x20 */ u8 pad_0x20[0x3FE0];
} SystemFile; /* size: 0x4000 */

/* size: 0x14000 - a data file: the in-use word, the checksum, the player name, the slots in use, the options and the
 * three 0x6000-byte user data slots. */
typedef struct DataFile {
    /* +0x00000 */ s32 in_use;
    /* +0x00004 */ u32 checksum;         /* the byte sum of bytes 8..0x13FFF */
    /* +0x00008 */ char name[0x11];
    /* +0x00019 */ u8 last_slot;
    /* +0x0001A */ u8 slot_used[3];
    /* +0x0001D */ u8 pad_0x1D[0xB];
    /* +0x00028 */ u8 options[0x20];     /* `option_w`'s first 0x1F bytes */
    /* +0x00048 */ u8 slots[3][0x6000];
    /* +0x12048 */ u8 pad_0x12048[0x1FB8];
} DataFile; /* size: 0x14000 */

#define SAVE_SLOT_SIZE 0x6000

SaveNandTask save_nand_task;
SaveCtl save_ctl;
NANDBanner save_banner __attribute__((aligned(32)));
u16 save_banner_comment[0x24];

const char* save_data_file_names[6] = {"data00", "data01", "data02", "data03", "data04", "data05"};

extern "C" {

/* 0x80448404 (0x18): the NAND completion: stores the result and raises the done byte. */
void save_nand_callback(s32 result, NANDCommandBlock* block) {
    save_nand_task.result = result;
    save_nand_task.done = 1;
}

/* 0x8044841C (0x28): arms a file creation of `path`. */
void save_nand_create_init(const char* path) {
    save_nand_task.step = 0;
    save_nand_task.retries = 3;
    save_nand_task.delay = 0;
    strcpy(save_nand_task.path, path);
}

/* 0x80448444 (0x168): steps the file creation; 1 done, 0 busy, -1 failed, -2 failed with a mapped result. */
s32 save_nand_create_step(void) {
    SaveNandTask* task = &save_nand_task;
    s32 ret;

    switch (task->step) {
    case 0:
        if (task->delay > 0) {
            task->delay--;
            break;
        }
        task->done = 0;
        ret = NANDCreateAsync(task->path, 0x30, 0, save_nand_callback, &task->block);
        switch (ret) {
        case 0:
            task->step++;
            break;
        case -3:
        case -2:
            if (--task->retries > 0) {
                task->delay = 30;
                break;
            }
        default:
            OSReport("NANDCreateAsync() failed. Result code: %d\n", ret);
            return -1;
        }
        break;
    case 1:
        if (task->done) {
            switch (task->result) {
            case 0:
            case -6:
                return 1;
            case -4:
                task->result = -6;
                system_w.nand_corrupt_0x931 = 1;
                return -2;
            case -11:
                task->result = -17;
                return -2;
            default:
                OSReport("NANDCreateAsync() failed. Result code: %d\n", task->result);
                return -1;
            }
        }
        break;
    }
    return 0;
}

/* 0x804485AC (0x28): arms an open of `path`. */
void save_nand_open_init(const char* path) {
    save_nand_task.step = 0;
    save_nand_task.retries = 3;
    save_nand_task.delay = 0;
    strcpy(save_nand_task.path, path);
}

/* 0x804485D4 (0x150): steps the open (read/write access). */
s32 save_nand_open_step(void) {
    SaveNandTask* task = &save_nand_task;
    s32 ret;

    switch (task->step) {
    case 0:
        if (task->delay > 0) {
            task->delay--;
            break;
        }
        task->done = 0;
        ret = NANDOpenAsync(task->path, &task->info, 3, save_nand_callback, &task->block);
        switch (ret) {
        case 0:
            task->step++;
            break;
        case -3:
        case -2:
            if (--task->retries > 0) {
                task->delay = 30;
                break;
            }
        default:
            OSReport("NANDOpenAsync() failed. Result code: %d\n", ret);
            return -1;
        }
        break;
    case 1:
        if (task->done) {
            switch (task->result) {
            case 0:
                return 1;
            case -12:
                task->result = -4;
                return -2;
            case -11:
                task->result = -17;
                return -2;
            default:
                OSReport("NANDOpenAsync() failed. Result code: %d\n", task->result);
                return -1;
            }
        }
        break;
    }
    return 0;
}

/* 0x80448724 (0x20): arms a close of the open file. */
void save_nand_close_init(void) {
    save_nand_task.step = 0;
    save_nand_task.retries = 3;
    save_nand_task.delay = 0;
}

/* 0x80448744 (0x140): steps the close. */
s32 save_nand_close_step(void) {
    SaveNandTask* task = &save_nand_task;
    s32 ret;

    switch (task->step) {
    case 0:
        if (task->delay > 0) {
            task->delay--;
            break;
        }
        task->done = 0;
        ret = NANDCloseAsync(&task->info, save_nand_callback, &task->block);
        switch (ret) {
        case 0:
            task->step++;
            break;
        case -3:
        case -2:
            if (--task->retries > 0) {
                task->delay = 30;
                break;
            }
        default:
            OSReport("NANDCloseAsync() failed. Result code: %d\n", ret);
            return -1;
        }
        break;
    case 1:
        if (task->done) {
            switch (task->result) {
            case 0:
                return 1;
            case -4:
                system_w.nand_corrupt_0x931 = 1;
                task->result = -6;
                return -2;
            default:
                OSReport("NANDCloseAsync() failed. Result code: %d\n", task->result);
                return -1;
            }
        }
        break;
    }
    return 0;
}

/* 0x80448884 (0x28): arms a read of `length` bytes into `buffer`. */
void save_nand_read_init(u8* buffer, u32 length) {
    save_nand_task.step = 0;
    save_nand_task.retries = 3;
    save_nand_task.delay = 0;
    save_nand_task.buffer = buffer;
    save_nand_task.length = length;
}

/* 0x804488AC (0x19C): steps the read; a short read is a corrupt file (-15). */
s32 save_nand_read_step(void) {
    SaveNandTask* task = &save_nand_task;
    s32 ret;

    switch (task->step) {
    case 0:
        if (task->delay > 0) {
            task->delay--;
            break;
        }
        task->done = 0;
        ret = NANDReadAsync(&task->info, task->buffer, task->length, save_nand_callback, &task->block);
        switch (ret) {
        case 0:
            task->step++;
            break;
        case -3:
        case -2:
            if (--task->retries > 0) {
                task->delay = 30;
                break;
            }
        default:
            OSReport("NANDReadAsync() failed. Result code: %d\n", ret);
            NANDClose(&task->info);
            return -1;
        }
        break;
    case 1:
        if (task->done) {
            switch (task->result) {
            default:
                if (task->result < 0) {
                    OSReport("NANDReadAsync() failed. Result code: %d\n", task->result);
                    NANDClose(&task->info);
                    return -1;
                }
                if (task->result == task->length) {
                    return 1;
                }
            case -15:
            case -5:
                task->result = -15;
                NANDClose(&task->info);
                return -2;
            case -4:
                system_w.nand_corrupt_0x931 = 1;
                task->result = -6;
                NANDClose(&task->info);
                return -2;
            }
        }
        break;
    }
    return 0;
}

/* 0x80448A48 (0x28): arms a write of `length` bytes from `buffer`. */
void save_nand_write_init(u8* buffer, u32 length) {
    save_nand_task.step = 0;
    save_nand_task.retries = 3;
    save_nand_task.delay = 0;
    save_nand_task.buffer = buffer;
    save_nand_task.length = length;
}

/* 0x80448A70 (0x18C): steps the write; a full device is -16. */
s32 save_nand_write_step(void) {
    SaveNandTask* task = &save_nand_task;
    s32 ret;

    switch (task->step) {
    case 0:
        if (task->delay > 0) {
            task->delay--;
            break;
        }
        task->done = 0;
        ret = NANDWriteAsync(&task->info, task->buffer, task->length, save_nand_callback, &task->block);
        switch (ret) {
        case 0:
            task->step++;
            break;
        case -3:
        case -2:
            if (--task->retries > 0) {
                task->delay = 30;
                break;
            }
        default:
            OSReport("NANDWriteAsync() failed. Result code: %d\n", ret);
            NANDClose(&task->info);
            return -1;
        }
        break;
    case 1:
        if (task->done) {
            if (task->result >= 0) {
                if (task->result == task->length) {
                    return 1;
                }
            } else {
                switch (task->result) {
                case -4:
                    system_w.nand_corrupt_0x931 = 1;
                    task->result = -6;
                    NANDClose(&task->info);
                    return -2;
                case -9:
                    task->result = -16;
                    NANDClose(&task->info);
                    return -2;
                }
            }
            OSReport("NANDWriteAsync() failed. Result code: %d\n", task->result);
            NANDClose(&task->info);
            return -1;
        }
        break;
    }
    return 0;
}

/* 0x80448BFC (0x28): arms a delete of `path`. */
void save_nand_delete_init(const char* path) {
    save_nand_task.step = 0;
    save_nand_task.retries = 3;
    save_nand_task.delay = 0;
    strcpy(save_nand_task.path, path);
}

/* 0x80448C24 (0x148): steps the delete; a missing file counts as deleted. */
s32 save_nand_delete_step(void) {
    SaveNandTask* task = &save_nand_task;
    s32 ret;

    switch (task->step) {
    case 0:
        if (task->delay > 0) {
            task->delay--;
            break;
        }
        task->done = 0;
        ret = NANDDeleteAsync(task->path, save_nand_callback, &task->block);
        switch (ret) {
        case 0:
            task->step++;
            break;
        case -3:
        case -2:
            if (--task->retries > 0) {
                task->delay = 30;
                break;
            }
        default:
            OSReport("NANDDeleteAsync() failed. Result code: %d\n", ret);
            return -1;
        }
        break;
    case 1:
        if (task->done) {
            switch (task->result) {
            case 0:
            case -12:
                return 1;
            case -4:
                system_w.nand_corrupt_0x931 = 1;
                task->result = -6;
                return -2;
            default:
                OSReport("NANDDeleteAsync() failed. Result code: %d\n", task->result);
                return -1;
            }
        }
        break;
    }
    return 0;
}

/* 0x80448D6C (0x28): arms a move of `path`. */
void save_nand_move_init(const char* path) {
    save_nand_task.step = 0;
    save_nand_task.retries = 3;
    save_nand_task.delay = 0;
    strcpy(save_nand_task.path, path);
}

/* 0x80448D94 (0x1B4): steps the move of the file into the home directory (`to_root`) or its nocopy directory. */
s32 save_nand_move_step(u8 to_root) {
    SaveNandTask* task = &save_nand_task;
    char dir[64] = "";
    s32 ret;

    switch (task->step) {
    case 0:
        if (task->delay > 0) {
            task->delay--;
            break;
        }
        NANDGetHomeDir(dir);
        if (to_root == 0) {
            strcat(dir, "/nocopy");
        }
        task->done = 0;
        ret = NANDMoveAsync(task->path, dir, save_nand_callback, &task->block);
        switch (ret) {
        case 0:
            task->step++;
            break;
        case -3:
        case -2:
            if (--task->retries > 0) {
                task->delay = 30;
                break;
            }
        default:
            OSReport("NANDMoveAsync() failed. Result code: %d\n", ret);
            return -1;
        }
        break;
    case 1:
        if (task->done) {
            switch (task->result) {
            case 0:
                return 1;
            case -4:
                system_w.nand_corrupt_0x931 = 1;
                task->result = -6;
                return -2;
            default:
                OSReport("NANDMoveAsync() failed. Result code: %d\n", task->result);
                return -1;
            }
        }
        break;
    }
    return 0;
}

/* 0x80448F48 (0x28): arms a quota check for `fs_block` blocks and `inode` inodes. */
void save_nand_check_init(u32 fs_block, u32 inode) {
    save_nand_task.step = 0;
    save_nand_task.retries = 3;
    save_nand_task.delay = 0;
    save_nand_task.fs_block = fs_block;
    save_nand_task.inode = inode;
}

/* 0x80448F70 (0x158): steps the quota check; -16 when the blocks do not fit, -17 when the inodes do not. */
s32 save_nand_check_step(void) {
    SaveNandTask* task = &save_nand_task;
    s32 ret;

    switch (task->step) {
    case 0:
        if (task->delay > 0) {
            task->delay--;
            break;
        }
        task->done = 0;
        ret = NANDCheckAsync(task->fs_block, task->inode, &task->answer, save_nand_callback, &task->block);
        switch (ret) {
        case 0:
            task->step++;
            break;
        case -3:
        case -2:
            if (--task->retries > 0) {
                task->delay = 30;
                break;
            }
        default:
            OSReport("NANDCheckAsync() failed. Result code: %d\n", ret);
            return -1;
        }
        break;
    case 1:
        if (task->done) {
            if (task->result == 0) {
                if ((task->answer & 5) != 0) {
                    task->result = -16;
                    return -2;
                }
                if ((task->answer & 10) != 0) {
                    task->result = -17;
                    return -2;
                }
                return 1;
            }
            OSReport("NANDCheckAsync() failed. Result code: %d\n", task->result);
            return -1;
        }
        break;
    }
    return 0;
}

/* 0x804490C8 (0x28): arms a directory creation of `path`. */
void save_nand_mkdir_init(const char* path) {
    save_nand_task.step = 0;
    save_nand_task.retries = 3;
    save_nand_task.delay = 0;
    strcpy(save_nand_task.path, path);
}

/* 0x804490F0 (0x168): steps the directory creation; an existing directory counts as created. */
s32 save_nand_mkdir_step(void) {
    SaveNandTask* task = &save_nand_task;
    s32 ret;

    switch (task->step) {
    case 0:
        if (task->delay > 0) {
            task->delay--;
            break;
        }
        task->done = 0;
        ret = NANDCreateDirAsync(task->path, 0x30, 0, save_nand_callback, &task->block);
        switch (ret) {
        case 0:
            task->step++;
            break;
        case -3:
        case -2:
            if (--task->retries > 0) {
                task->delay = 30;
                break;
            }
        default:
            OSReport("NANDCreateDirAsync() failed. Result code: %d\n", ret);
            return -1;
        }
        break;
    case 1:
        if (task->done) {
            switch (task->result) {
            case 0:
            case -6:
                return 1;
            case -4:
                system_w.nand_corrupt_0x931 = 1;
                task->result = -6;
                return -2;
            case -11:
                task->result = -17;
                return -2;
            default:
                OSReport("NANDCreateDirAsync() failed. Result code: %d\n", task->result);
                return -1;
            }
        }
        break;
    }
    return 0;
}

/* 0x80449258 (0x58): arms the free-space check for the save files. */
s32 save_space_check_init(void) {
    save_ctl.step = 0;
    save_ctl.scratch_0x04 = 0;
    save_ctl.scratch_0x08 = 0;
    save_ctl.scratch_0x0C = 0;
    save_ctl.scratch_0x10 = 0;
    save_ctl.check_blocks = save_check_blocks();
    save_ctl.check_inodes = save_check_inodes();
    return 1;
}

/* 0x804492B0 (0xB8): steps the free-space check; 1 when the files fit. */
s32 save_space_check_step(void) {
    SaveNandTask* task = &save_nand_task;
    SaveCtl* ctl = &save_ctl;

    switch (save_ctl.step) {
    case 0:
        save_nand_check_init(ctl->check_blocks, ctl->check_inodes);
        ctl->step++;
    case 1:
        switch (save_nand_check_step()) {
        case 1:
            return 1;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    }
    return 0;
}

} /* extern "C" */

/* 0x80449368 (0x90): arms the system file creation; 0 when the work block is missing. */
s32 createSystemFile_init(void) {
    SaveCtl* ctl = &save_ctl;
    u8* buffer;

    save_ctl.step = 0;
    ctl->scratch_0x04 = 0;
    ctl->scratch_0x08 = 0;
    ctl->scratch_0x0C = 0;
    ctl->scratch_0x10 = 0;
    strcpy(ctl->file_name, "system");
    buffer = (u8*)system_w.layout_0x950;
    ctl->buffer = buffer;
    if (buffer == NULL) {
        return 0;
    }
    ctl->size = 0x4000;
    ctl->kind = 1;
    memset(buffer, 0, 0x4000);
    return 1;
}

extern "C" {

/* 0x804493F8 (0x154): fills the system file and its checksum. */
void system_file_build(void) {
    SaveCtl* ctl = &save_ctl;
    SystemFile* file = (SystemFile*)ctl->buffer;
    u8* bytes;
    s32 i;

    memset(file, 0, 0x4000);
    file->version = 1;
    file->format = 4;
    if (system_w.sys_file_valid_0x856 == 0) {
        memset(file->reserved_0x10, 0, 6);
        file->unlock_seen[0] = 0;
        file->unlock_seen[1] = 0;
        file->save_keep[0] = 0;
        file->save_keep[1] = 0;
    } else {
        file->unlock_seen[0] = system_w.unlock_seen_0x888[0];
        file->unlock_seen[1] = system_w.unlock_seen_0x888[1];
        file->save_keep[0] = system_w.save_keep_0x910[0];
        file->save_keep[1] = system_w.save_keep_0x910[1];
    }
    bytes = ctl->buffer;
    file->checksum = 0;
    bytes += 8;
    for (i = 8; i < 0x4000; i++) {
        file->checksum += *bytes;
        bytes++;
    }
}

} /* extern "C" */

/* 0x8044954C (0x314): steps the system file creation (create in /tmp, write, move into the home directory). */
s32 createSystemFile(void) {
    SaveNandTask* task = &save_nand_task;
    SaveCtl* ctl = &save_ctl;
    char path[64] = "";

    sprintf(path, "/tmp/%s", ctl->file_name);
    switch (save_ctl.step) {
    case 0:
        save_nand_mkdir_init("nocopy");
        ctl->step++;
    case 1:
        switch (save_nand_mkdir_step()) {
        case 1:
            save_nand_create_init(path);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return (ctl->error == -4) - 2;
        }
        break;
    case 2:
        switch (save_nand_create_step()) {
        case 1:
            save_nand_open_init(path);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return (ctl->error == -4) - 2;
        }
        break;
    case 3:
        switch (save_nand_open_step()) {
        case 1:
            system_file_build();
            save_nand_write_init(ctl->buffer, 0x4000);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 4:
        switch (save_nand_write_step()) {
        case 1:
            save_nand_close_init();
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 5:
        switch (save_nand_close_step()) {
        case 1:
            save_nand_move_init(path);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 6:
        switch (save_nand_move_step(0)) {
        case 1:
            return 1;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    }
    return 0;
}

extern "C" {

/* 0x80449860 (0x18): starts writing the current player's data file. */
s32 game_data_file_create_start(void) {
    return createDataFile_init(system_w.save_keep_0x910[0], system_w.save_keep_0x910[1], 0);
}

} /* extern "C" */

/* 0x80449878 (0x50): steps the data file write and mirrors "busy" into `system_w`. */
s32 game_save_wait(void) {
    s32 ret = createDataFile(0);
    if (ret == 0) {
        system_w.field_0x865 = 1;
    } else {
        system_w.field_0x865 = 0;
    }
    return ret;
}

extern "C" {

/* 0x804498C8 (0x4): starts the system file creation. */
s32 game_system_file_create_start(void) {
    return createSystemFile_init();
}

/* 0x804498CC (0x4C): steps the system file creation and mirrors "busy" into `system_w`. */
s32 game_system_file_create_wait(void) {
    s32 ret = createSystemFile();
    if (ret == 0) {
        system_w.field_0x865 = 1;
    } else {
        system_w.field_0x865 = 0;
    }
    return ret;
}

/* 0x80449918 (0x4): starts writing the account file. */
void startAccountLoad(void) {
    account_file_create_init();
}

/* 0x8044991C (0x4C): steps the account file write and mirrors "busy" into `system_w`. */
s32 pollAccountLoad(s16 frame) {
    s32 ret = account_file_create();
    if (ret == 0) {
        system_w.field_0x865 = 1;
    } else {
        system_w.field_0x865 = 0;
    }
    return ret;
}

/* 0x80449968 (0x4): starts reading the account file. */
void startBootLoad(void) {
    account_file_read_init();
}

/* 0x8044996C (0x4): steps the account file read. */
s32 pollBootLoad(void) {
    return account_file_read();
}

} /* extern "C" */

/* 0x80449970 (0xD4): arms the write of player `player`'s data file (slot `slot`; `clear` empties it first). */
u32 createDataFile_init(long player, long slot, long clear) {
    SaveCtl* ctl = &save_ctl;
    u8* buffer;

    save_ctl.step = 0;
    ctl->scratch_0x04 = 0;
    ctl->scratch_0x08 = 0;
    ctl->scratch_0x0C = 0;
    ctl->player = player;
    ctl->slot = slot;
    ctl->corrupt_error = 0;
    ctl->clear = clear;
    ctl->scratch_0x10 = 0;
    strcpy(ctl->file_name, save_data_file_names[ctl->player]);
    buffer = (u8*)system_w.layout_0x950;
    ctl->buffer = buffer;
    if (buffer == NULL) {
        return 0;
    }
    ctl->size = 0x14000;
    ctl->kind = 5;
    memset(buffer, 0, 0x14000);
    memset(ctl->buffer, 0, 0x14000);
    return 1;
}

extern "C" {

/* 0x80449A44 (0x268): fills the data file (a fresh one for slot -2, else the options, the name and the slot's user
 * data) and its checksum. */
void data_file_build(void) {
    SaveCtl* ctl = &save_ctl;
    DataFile* file = (DataFile*)ctl->buffer;
    u8* bytes;
    u32 i;
    s32 j;

    if (ctl->slot == -2) {
        memset(file, 0, 0x14000);
        for (i = 0; i < 3; i++) {
            memset(file->slots[i], 0, 4);
        }
        option_cfg_load(file->options);
        file->slot_used[0] = 0;
        file->slot_used[1] = 0;
        file->slot_used[2] = 0;
        file->last_slot = 0;
        sprintf(file->name, "FILE%d", ctl->player + 1);
        strcpy(system_w.save_name_0x7ea[ctl->player], file->name);
        file->in_use = 0;
    } else {
        if (ctl->clear != 0) {
            memset(file, 0, 0x14000);
        }
        memcpy(system_w.option_copy_0x912, option_w, 0x1F);
        memcpy(file->options, option_w, 0x1F);
        file->options[0x16] = 0;
        strcpy(file->name, system_w.save_name_0x7ea[ctl->player]);
        file->in_use = 1;
        if (ctl->slot != -1) {
            u8* slot;
            file->slot_used[ctl->slot] = 1;
            slot = file->slots[ctl->slot];
            memset(slot, 0, 4);
            userdata_save_export(slot, SAVE_SLOT_SIZE);
            file->last_slot = system_w.save_keep_0x910[1];
        }
    }
    bytes = ctl->buffer;
    file->checksum = 0;
    bytes += 8;
    for (j = 8; j < 0x14000; j++) {
        file->checksum += *bytes;
        bytes++;
    }
}

} /* extern "C" */

/* 0x80449CAC (0x4CC): steps the data file write (read back the old file's slots, create in /tmp, write, move);
 * `check_only` stops after the read-back. */
s32 createDataFile(u8 check_only) {
    SaveNandTask* task = &save_nand_task;
    SaveCtl* ctl = &save_ctl;
    char home[64] = "";
    char path[64] = "";

    if (save_ctl.step < 4) {
        sprintf(path, "nocopy/%s", ctl->file_name);
    } else {
        sprintf(path, "/tmp/%s", ctl->file_name);
    }
    switch (ctl->step) {
    case 0:
        NANDGetHomeDir(home);
        sprintf(ctl->file_name, "%s/%s", home, path);
        sprintf(path, "%s", ctl->file_name);
        save_nand_open_init(path);
        ctl->step++;
        ctl->read_done = 0;
    case 1:
        switch (save_nand_open_step()) {
        case 1:
            save_nand_read_init(ctl->buffer, ctl->size);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            if (ctl->error == -4) {
                strcpy(ctl->file_name, save_data_file_names[ctl->player]);
                ctl->corrupt_error = ctl->error;
                ctl->step = 4;
                break;
            }
            return -2;
        }
        break;
    case 2:
        switch (save_nand_read_step()) {
        case 1:
            ctl->read_done = 1;
            save_nand_close_init();
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 3:
        switch (save_nand_close_step()) {
        case 1:
            if (check_only == 1) {
                return 1;
            }
            strcpy(ctl->file_name, save_data_file_names[ctl->player]);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 4:
        save_nand_create_init(path);
        ctl->step++;
    case 5:
        switch (save_nand_create_step()) {
        case 1:
            save_nand_open_init(path);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return (ctl->error == -4) - 2;
        }
        break;
    case 6:
        switch (save_nand_open_step()) {
        case 1:
            data_file_build();
            save_nand_write_init(ctl->buffer, 0x14000);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 7:
        switch (save_nand_write_step()) {
        case 1:
            save_nand_close_init();
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 8:
        switch (save_nand_close_step()) {
        case 1:
            save_nand_move_init(path);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 9:
        switch (save_nand_move_step(0)) {
        case 1:
            return 1;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    }
    return 0;
}

extern "C" {

/* 0x8044A178 (0x24): rewinds the file operation. */
s32 save_ctl_reset(void) {
    save_ctl.step = 0;
    save_ctl.scratch_0x04 = 0;
    save_ctl.scratch_0x08 = 0;
    save_ctl.scratch_0x0C = 0;
    return 1;
}

/* The banner's icon frames: per icon, the texture slot (minus one) and the animation speed; the ninth speed ends
 * the animation. */
s32 save_banner_icon_anim[18] = {0, 3, 0, 3, 0, 3, 0, 3, 2, 3, 2, 3, 2, 3, 2, 3, -1, 0};

#define BANNER_SET_ICON_SPEED(banner, n, speed) \
    ((banner)->iconSpeed = (u16)(((banner)->iconSpeed & ~(3 << (2 * (n)))) | ((speed) << (2 * (n)))))

/* 0x8044A19C (0x128): builds the save banner (title, banner texture, eight icon frames and their speeds). */
void banner_build(void) {
    s32 i;

    NANDInitBanner(&save_banner, 1, (const u16*)L"Monster Hunter Tri", save_banner_comment);
    memcpy(save_banner.bannerTexture, (u8*)sys_tex_image_get(0), 0x6000);
    for (i = 0; i < 8; i++) {
        memcpy(save_banner.iconTexture[i], (u8*)sys_tex_image_get(save_banner_icon_anim[i * 2] + 1), 0x1200);
        BANNER_SET_ICON_SPEED(&save_banner, i, save_banner_icon_anim[i * 2 + 1]);
    }
    BANNER_SET_ICON_SPEED(&save_banner, i, save_banner_icon_anim[i * 2 + 1]);
}

/* 0x8044A2C4 (0x2D8): steps the banner file creation (create in /tmp, write, move into the home directory). */
s32 banner_file_create(void) {
    SaveNandTask* task = &save_nand_task;
    SaveCtl* ctl = &save_ctl;
    char path[64] = "";

    sprintf(path, "/tmp/%s", "banner.bin");
    switch (ctl->step) {
    case 0:
        save_nand_create_init(path);
        ctl->step++;
    case 1:
        switch (save_nand_create_step()) {
        case 1:
            save_nand_open_init(path);
            ctl->step++;
            break;
        case 0:
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            if (ctl->error == -4) {
                return -1;
            }
            return -2;
        }
        break;
    case 2:
        switch (save_nand_open_step()) {
        case 1:
            banner_build();
            save_nand_write_init((u8*)&save_banner, sizeof(NANDBanner));
            ctl->step++;
            break;
        case 0:
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 3:
        switch (save_nand_write_step()) {
        case 1:
            save_nand_close_init();
            ctl->step++;
            break;
        case 0:
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 4:
        switch (save_nand_close_step()) {
        case 1:
            save_nand_move_init(path);
            ctl->step++;
            break;
        case 0:
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 5:
        switch (save_nand_move_step(1)) {
        case 1:
            return 1;
        case 0:
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    }
    return 0;
}

/* 0x8044A59C (0xA4): arms the banner file probe. */
s32 banner_file_check_init(void) {
    SaveCtl* ctl = &save_ctl;
    u8* buffer;

    save_ctl.step = 0;
    ctl->scratch_0x04 = 0;
    ctl->scratch_0x08 = 0;
    ctl->scratch_0x0C = 0;
    ctl->scratch_0x10 = 0;
    strcpy(ctl->file_name, "banner.bin");
    buffer = (u8*)system_w.layout_0x950;
    ctl->buffer = buffer;
    if (buffer == NULL) {
        return 0;
    }
    ctl->size = 0x10000;
    ctl->kind = 4;
    memset(buffer, 0, ctl->size);
    ctl->verify_error = 0;
    ctl->error = 0;
    ctl->banner_exists = 0;
    return 1;
}

/* 0x8044A640 (0x208): steps the banner file probe (open and close banner.bin). */
s32 banner_file_check(void) {
    SaveNandTask* task = &save_nand_task;
    SaveCtl* ctl = &save_ctl;
    char home[64] = "";
    char path[64] = "";

    sprintf(path, "%s", ctl->file_name);
    switch (ctl->step) {
    case 0:
        NANDGetHomeDir(home);
        sprintf(ctl->file_name, "%s/%s", home, path);
        sprintf(path, "%s", ctl->file_name);
        save_nand_open_init(path);
        ctl->step++;
    case 1:
        switch (save_nand_open_step()) {
        case 1:
            save_nand_close_init();
            ctl->step++;
            ctl->banner_exists = 1;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 2:
        switch (save_nand_close_step()) {
        case 1:
            if (ctl->verify_error != 0) {
                ctl->error = -15;
                return -2;
            }
            return 1;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    }
    return 0;
}

/* 0x8044A848 (0x90): arms the account file write. */
s32 account_file_create_init(void) {
    SaveCtl* ctl = &save_ctl;
    u8* buffer;

    save_ctl.step = 0;
    ctl->scratch_0x04 = 0;
    ctl->scratch_0x08 = 0;
    ctl->scratch_0x0C = 0;
    ctl->scratch_0x10 = 0;
    strcpy(ctl->file_name, "network");
    buffer = (u8*)system_w.layout_0x950;
    ctl->buffer = buffer;
    if (buffer == NULL) {
        return 0;
    }
    ctl->size = 0x4000;
    ctl->kind = 1;
    memset(buffer, 0, 0x4000);
    return 1;
}

/* 0x8044A8D8 (0x130): fills the account file from the network work record and its checksum. */
void account_file_build(void) {
    SaveCtl* ctl = &save_ctl;
    NetSaveRecord* record = (NetSaveRecord*)ctl->buffer;
    u8* bytes;
    s32 i;

    memset(record, 0, sizeof(NetSaveRecord));
    exportNetworkSave(record);
    bytes = ctl->buffer;
    record->checksum_0x00 = 0;
    bytes += 4;
    for (i = 4; i < 0x4000; i++) {
        record->checksum_0x00 += *bytes;
        bytes++;
    }
}

/* 0x8044AA08 (0x2B0): steps the account file write (create in /tmp, write, move into the home directory). */
s32 account_file_create(void) {
    SaveNandTask* task = &save_nand_task;
    SaveCtl* ctl = &save_ctl;
    char path[64] = "";

    sprintf(path, "/tmp/%s", ctl->file_name);
    switch (ctl->step) {
    case 0:
        save_nand_create_init(path);
        ctl->step++;
        break;
    case 1:
        switch (save_nand_create_step()) {
        case 1:
            save_nand_open_init(path);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            if (task->result == -4) {
                return -1;
            }
            ctl->error = task->result;
            return -2;
        }
        break;
    case 2:
        switch (save_nand_open_step()) {
        case 1:
            account_file_build();
            save_nand_write_init(ctl->buffer, 0x4000);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 3:
        switch (save_nand_write_step()) {
        case 1:
            save_nand_close_init();
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 4:
        switch (save_nand_close_step()) {
        case 1:
            save_nand_move_init(path);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 5:
        switch (save_nand_move_step(0)) {
        case 1:
            return 1;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    }
    return 0;
}

} /* extern "C" */

/* 0x8044ACB8 (0xC4): arms the read of player `player`'s data file (`slot` picks what the read applies). */
s32 readDataFile_init(long player, long slot) {
    SaveCtl* ctl = &save_ctl;
    u8* buffer;

    save_ctl.step = 0;
    ctl->scratch_0x04 = 0;
    ctl->scratch_0x08 = 0;
    ctl->scratch_0x0C = 0;
    ctl->player = player;
    ctl->slot = slot;
    ctl->scratch_0x10 = 0;
    strcpy(ctl->file_name, save_data_file_names[ctl->player]);
    buffer = (u8*)system_w.layout_0x950;
    ctl->buffer = buffer;
    if (buffer == NULL) {
        return 0;
    }
    ctl->size = 0x14000;
    ctl->kind = 5;
    memset(buffer, 0, 0x14000);
    ctl->verify_error = 0;
    ctl->error = 0;
    return 1;
}

extern "C" {

/* 0x8044AD7C (0x248): checks the read data file's checksum and applies it (name, in-use word, options, the slot's
 * user data); 0 on a checksum mismatch. */
s32 data_file_apply(void) {
    SaveCtl* ctl = &save_ctl;
    u8* bytes = ctl->buffer;
    DataFile* file;
    u32 sum;
    s32 i;

    bytes += 8;
    sum = 0;
    for (i = 8; i < 0x14000; i++) {
        sum += *bytes;
        bytes++;
    }
    system_w.save_name_0x7ea[ctl->player][0] = 0;
    file = (DataFile*)ctl->buffer;
    if (file->checksum != sum) {
        return 0;
    }
    strcpy(system_w.save_name_0x7ea[ctl->player], file->name);
    system_w.save_used_0x859[ctl->player] = file->in_use;
    if (ctl->slot != 5) {
        system_w.save_keep_0x910[1] = file->last_slot;
    }
    if (file->in_use == 0) {
        return 1;
    }
    if (ctl->slot == -1) {
        return 1;
    }
    if (ctl->slot != 6) {
        memcpy(option_w, file->options, 0x1F);
        set_option_mode();
    }
    if (ctl->slot < 0) {
        return 1;
    }
    if (ctl->slot == 4) {
        return 1;
    }
    if (ctl->slot == 5) {
        return 1;
    }
    if (ctl->slot == 6) {
        return 1;
    }
    if (file->in_use != 0) {
        userdata_save_import(file->slots[ctl->slot], SAVE_SLOT_SIZE);
    }
    return 1;
}

/* 0x8044AFC4 (0x18): copies the read data file's options to `options`. */
void data_file_options_get(u8* options) {
    memcpy(options, ((DataFile*)save_ctl.buffer)->options, 0x1F);
}

} /* extern "C" */

/* 0x8044AFDC (0x28C): steps the data file read (open, read, check and apply, close). */
s32 readDataFile(void) {
    SaveNandTask* task = &save_nand_task;
    SaveCtl* ctl = &save_ctl;
    char home[64] = "";
    char path[64] = "";

    sprintf(path, "%s", ctl->file_name);
    switch (ctl->step) {
    case 0:
        NANDGetHomeDir(home);
        sprintf(ctl->file_name, "%s/nocopy/%s", home, path);
        sprintf(path, "%s", ctl->file_name);
        save_nand_open_init(path);
        ctl->step++;
    case 1:
        switch (save_nand_open_step()) {
        case 1:
            save_nand_read_init(ctl->buffer, ctl->size);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 2:
        switch (save_nand_read_step()) {
        case 1:
            if (ctl->slot != -2 && data_file_apply() == 0) {
                ctl->verify_error = -1;
            }
            save_nand_close_init();
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 3:
        switch (save_nand_close_step()) {
        case 1:
            if (ctl->verify_error != 0) {
                ctl->error = -15;
                return -2;
            }
            return 1;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    }
    return 0;
}

/* 0x8044B268 (0x148): copies slot `slot` of the read data file into the user data; 0 on a checksum mismatch or an
 * unused file. */
s32 setPlayerSave2Userdata(u8 slot) {
    SaveCtl* ctl = &save_ctl;
    u8* bytes = ctl->buffer;
    DataFile* file;
    u32 sum;
    s32 i;

    bytes += 8;
    sum = 0;
    for (i = 8; i < 0x14000; i++) {
        sum += *bytes;
        bytes++;
    }
    file = (DataFile*)ctl->buffer;
    if (file->checksum != sum) {
        return 0;
    }
    if (file->in_use != 0) {
        userdata_save_import(file->slots[slot], SAVE_SLOT_SIZE);
        return 1;
    }
    return 0;
}

extern "C" {

/* 0x8044B3B0 (0xA0): arms the system file read. */
s32 system_file_read_init(void) {
    SaveCtl* ctl = &save_ctl;
    u8* buffer;

    save_ctl.step = 0;
    ctl->scratch_0x04 = 0;
    ctl->scratch_0x08 = 0;
    ctl->scratch_0x0C = 0;
    ctl->scratch_0x10 = 0;
    strcpy(ctl->file_name, "system");
    buffer = (u8*)system_w.layout_0x950;
    ctl->buffer = buffer;
    if (buffer == NULL) {
        return 0;
    }
    ctl->size = 0x4000;
    ctl->kind = 1;
    memset(buffer, 0, 0x4000);
    ctl->verify_error = 0;
    ctl->error = 0;
    return 1;
}

/* 0x8044B450 (0xCC): checks the read system file's checksum and applies it; 0 on a mismatch or a wrong format. */
s32 system_file_apply(void) {
    SaveCtl* ctl = &save_ctl;
    u8* bytes = ctl->buffer;
    SystemFile* file;
    u32 sum = 0;
    s32 i;

    bytes += 8;
    for (i = 8; i < 0x4000; i++) {
        sum += *bytes;
        bytes++;
    }
    file = (SystemFile*)ctl->buffer;
    if (file->checksum != sum) {
        return 0;
    }
    if (file->version == 1) {
        if (file->format != 4) {
            return 0;
        }
        system_w.unlock_seen_0x888[0] = file->unlock_seen[0];
        system_w.unlock_seen_0x888[1] = file->unlock_seen[1];
        system_w.save_keep_0x910[0] = file->save_keep[0];
        system_w.save_keep_0x910[1] = file->save_keep[1];
    }
    return 1;
}

/* 0x8044B51C (0x288): steps the system file read (open, read, check and apply, close). */
s32 system_file_read(void) {
    SaveNandTask* task = &save_nand_task;
    SaveCtl* ctl = &save_ctl;
    u32 blocks;
    u32 inodes;
    char home[64] = "";
    char path[64] = "";

    sprintf(path, "%s", ctl->file_name);
    switch (ctl->step) {
    case 0:
        NANDGetHomeUsage(&blocks, &inodes);
        NANDGetHomeDir(home);
        sprintf(ctl->file_name, "%s/nocopy/%s", home, path);
        sprintf(path, "%s", ctl->file_name);
        save_nand_open_init(path);
        ctl->step++;
    case 1:
        switch (save_nand_open_step()) {
        case 1:
            save_nand_read_init(ctl->buffer, ctl->size);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 2:
        switch (save_nand_read_step()) {
        case 1:
            if (system_file_apply() == 0) {
                ctl->verify_error = -1;
            }
            save_nand_close_init();
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 3:
        switch (save_nand_close_step()) {
        case 1:
            if (ctl->verify_error != 0) {
                ctl->error = -15;
                return -2;
            }
            return 1;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    }
    return 0;
}

/* 0x8044B7A4 (0xA0): arms the system file delete. */
s32 system_file_delete_init(void) {
    SaveCtl* ctl = &save_ctl;
    u8* buffer;

    save_ctl.step = 0;
    ctl->scratch_0x04 = 0;
    ctl->scratch_0x08 = 0;
    ctl->scratch_0x0C = 0;
    ctl->scratch_0x10 = 0;
    strcpy(ctl->file_name, "system");
    buffer = (u8*)system_w.layout_0x950;
    ctl->buffer = buffer;
    if (buffer == NULL) {
        return 0;
    }
    ctl->size = 0x4000;
    ctl->kind = 1;
    memset(buffer, 0, 0x4000);
    ctl->verify_error = 0;
    ctl->error = 0;
    return 1;
}

/* 0x8044B844 (0x198): steps the system file delete. */
s32 system_file_delete(void) {
    SaveNandTask* task = &save_nand_task;
    SaveCtl* ctl = &save_ctl;
    u32 blocks;
    u32 inodes;
    char home[64] = "";
    char path[64] = "";

    sprintf(path, "%s", ctl->file_name);
    switch (ctl->step) {
    case 0:
        NANDGetHomeUsage(&blocks, &inodes);
        NANDGetHomeDir(home);
        sprintf(ctl->file_name, "%s/nocopy/%s", home, path);
        sprintf(path, "%s", ctl->file_name);
        save_nand_delete_init(path);
        ctl->step++;
    case 1:
        switch (save_nand_delete_step()) {
        case 1:
            return 1;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    }
    return 0;
}

/* 0x8044B9DC (0xA0): arms the account file read. */
s32 account_file_read_init(void) {
    SaveCtl* ctl = &save_ctl;
    u8* buffer;

    save_ctl.step = 0;
    ctl->scratch_0x04 = 0;
    ctl->scratch_0x08 = 0;
    ctl->scratch_0x0C = 0;
    ctl->scratch_0x10 = 0;
    strcpy(ctl->file_name, "network");
    buffer = (u8*)system_w.layout_0x950;
    ctl->buffer = buffer;
    if (buffer == NULL) {
        return 0;
    }
    ctl->size = 0x4000;
    ctl->kind = 1;
    memset(buffer, 0, 0x4000);
    ctl->verify_error = 0;
    ctl->error = 0;
    return 1;
}

/* 0x8044BA7C (0x30): hands the read account file to the network work record. */
s32 account_file_apply(void) {
    importNetworkSave((NetSaveRecord*)save_ctl.buffer);
    return 1;
}

/* 0x8044BAAC (0x288): steps the account file read (open, read, apply, close). */
s32 account_file_read(void) {
    SaveNandTask* task = &save_nand_task;
    SaveCtl* ctl = &save_ctl;
    u32 blocks;
    u32 inodes;
    char home[64] = "";
    char path[64] = "";

    sprintf(path, "%s", ctl->file_name);
    switch (ctl->step) {
    case 0:
        NANDGetHomeUsage(&blocks, &inodes);
        NANDGetHomeDir(home);
        sprintf(ctl->file_name, "%s/nocopy/%s", home, path);
        sprintf(path, "%s", ctl->file_name);
        save_nand_open_init(path);
        ctl->step++;
    case 1:
        switch (save_nand_open_step()) {
        case 1:
            save_nand_read_init(ctl->buffer, ctl->size);
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 2:
        switch (save_nand_read_step()) {
        case 1:
            if (account_file_apply() == 0) {
                ctl->verify_error = -1;
            }
            save_nand_close_init();
            ctl->step++;
            break;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    case 3:
        switch (save_nand_close_step()) {
        case 1:
            if (ctl->verify_error != 0) {
                ctl->error = -15;
                return -2;
            }
            return 1;
        case -1:
            ctl->error = task->result;
            return -1;
        case -2:
            ctl->error = task->result;
            return -2;
        }
        break;
    }
    return 0;
}

/* 0x8044BD34 (0x10): the NAND result the last failed file step left. */
s32 save_error_get(void) {
    return save_ctl.error;
}

} /* extern "C" */

/* 0x8044BD44 (0x8C): the message id for the last NAND error. */
s32 chg_nand_err2msgcode(void) {
    switch (save_error_get()) {
    case -4:
        return 23;
    case -6:
        return 19;
    case -17:
        return 17;
    case -16:
        return 16;
    case -15:
        return 23;
    case -3:
    case -2:
        return 21;
    default:
        return 22;
    }
}

extern "C" {

/* 0x8044BDD0 (0x8): the blocks the save files need. */
u32 save_check_blocks(void) {
    return 0x24;
}

/* 0x8044BDD8 (0x8): the inodes the save files need. */
u32 save_check_inodes(void) {
    return 10;
}

/* 0x8044BDE0 (0x8): the number of data files a player can choose. */
s32 save_data_slot_max(void) {
    return 5;
}

} /* extern "C" */

/* ---------------------------------------------------------------------------------------------------------------- */
/* The lobby message menu (`menu_message.cpp`'s run): compose a mail, pick its address, send it. */

/* size: 0x11BC - the message menu's work: its step bytes, the cursor and dialog bytes, the frame's input, the mail
 * being written and the friend picker the address step runs. */
typedef struct PlMessageWork {
    /* +0x000 */ u8 step;
    /* +0x001 */ u8 sub;
    /* +0x002 */ u8 active;              /* 1 once the menu was opened */
    /* +0x003 */ s8 cursor;              /* the top menu's row (0 address, 1 card, 2 text, 3 send) */
    /* +0x004 */ s8 address_cursor;      /* the address step's row (0 friend list, 1 typed id) */
    /* +0x005 */ s8 choice;              /* the dialog's yes/no row */
    /* +0x006 */ u8 dialog_kind;         /* 1 a notice, 2 a yes/no question */
    /* +0x007 */ u8 dialog_msg;          /* the dialog's message row */
    /* +0x008 */ u16 input;              /* the frame's pressed buttons with the held d-pad */
    /* +0x00A */ u16 reserved_0x0A;      /* cleared every frame; nothing reads it */
    /* +0x00C */ LobbyMailRow mail;      /* the mail being written (`id_0x14`'s last byte is the card flag) */
    /* +0x0EC */ FriendPickWork pick;
} PlMessageWork; /* size: 0x11BC */

/* The mail's card flag: the byte after its id text. */
#define PLMSG_CARD_FLAG(mail) (*(u8*)&(mail)->id_0x14.text_0x00[10])

PlMessageWork plsearch_message_work;
char plsearch_kbd_text[0x14];

extern "C" {

s32 plsearch_address_step(PlMessageWork* work);
void plsearch_main_draw(PlMessageWork* work);
void plsearch_window_draw(s8* text);
void plsearch_choice_draw(u8 kind, s8 cursor);

/* 0x8044E2CC (0x64): clears the message work, stores `active` and copies the draft `mail` when there is one. */
void plsearch_work_init(PlMessageWork* work, u8 active, const LobbyMailRow* mail) {
    memset(work, 0, sizeof(PlMessageWork));
    work->active = active;
    if (mail != NULL) {
        work->mail.assign(mail);
    }
}

/* 0x8044E330 (0x10): clears the mail's body. */
void plsearch_mail_body_clear(PlMessageWork* work) {
    memset(&work->mail.body_0x1F, 0, sizeof(LobbyMailBody));
}

/* 0x8044E340 (0x650): steps the message menu; 2 when it closes, 4 when it fails. */
s32 plsearch_step(PlMessageWork* work) {
    s32 ret = 0;
    LobbyMailRow* mail = &work->mail;
    PlayerPad* pad;
    _PLW* plw;

    pad = Psw;
    if (chk_pointer() == 0) {
        work->input = pad->button_0x2C0.pressed_0x04 | (pad->button_0x2C0.held_0x14 & 0xF);
    } else {
        work->input = 0;
    }
    work->reserved_0x0A = 0;
    switch (work->step) {
    case 0:
        if (work->active == 1) {
            work->dialog_msg = 0;
            work->dialog_kind = 0;
            work->step = 1;
            work->sub = 0;
        }
        break;
    case 1:
        if (work->input & 0x10) {
            sysSE_req(0);
            switch (work->cursor) {
            case 0:
                work->step = 10;
                break;
            case 1:
                work->choice = PLMSG_CARD_FLAG(mail);
                work->step = 20;
                break;
            case 2:
                work->step = 30;
                break;
            case 3:
                if (flfntStrLen(mail->id_0x14.text_0x00) == 0 || flfntStrLen(mail->body_0x1F.text_0x00) == 0) {
                    work->dialog_msg = 12;
                    work->choice = 0;
                    work->dialog_kind = 1;
                    work->step = 9;
                } else {
                    work->step = 40;
                }
                break;
            }
            work->sub = 0;
        } else if (work->input & 0x20) {
            sysSE_req(1);
            work->dialog_msg = 15;
            work->dialog_kind = 2;
            work->choice = 1;
            work->step = 8;
        } else {
            work->cursor = menu_cursor_step(work->cursor, 4, work->input, 1, 2);
        }
        break;
    case 8:
        if (work->input & 0x10) {
            sysSE_req(0);
            if (work->choice == 0) {
                ret = 2;
            } else {
                work->step = 0;
            }
        } else if (work->input & 0x20) {
            sysSE_req(1);
            work->step = 0;
        } else {
            work->choice = menu_cursor_step(work->choice, 2, work->input, 4, 8);
        }
        break;
    case 9:
        if (work->input & 0x10) {
            sysSE_req(0);
            work->step = 0;
        } else if (work->input & 0x20) {
            sysSE_req(1);
            work->step = 0;
        }
        break;
    case 10:
        if (plsearch_address_step(work) != 0) {
            work->step = 1;
            work->sub = 0;
        }
        break;
    case 20:
        if (work->input & 0x10) {
            sysSE_req(0);
            work->step = 0;
        } else if (work->input & 0x20) {
            PLMSG_CARD_FLAG(mail) = work->choice;
            sysSE_req(1);
            work->step = 0;
        } else if (work->input & 4) {
            if (PLMSG_CARD_FLAG(mail) != 0) {
                sysSE_req(3);
                PLMSG_CARD_FLAG(mail) = 0;
            }
        } else if (work->input & 8) {
            if (PLMSG_CARD_FLAG(mail) != 1) {
                sysSE_req(3);
                PLMSG_CARD_FLAG(mail) = 1;
            }
        }
        break;
    case 30:
        switch (work->sub) {
        case 0:
            if (kbd_open(0) == 1) {
                menu_work.slot[0].field_0x32D = 1;
                set_kbd_param(mail->body_0x1F.text_0x00, 0xC0);
                menu_slot0_lock_set(1);
                work->sub++;
            }
            break;
        case 1:
            switch (kbd_move()) {
            case 1:
            case -1:
                menu_slot0_lock_set(0);
                menu_work.slot[0].field_0x32D = 0;
                work->step = 1;
                work->sub = 0;
                break;
            }
            break;
        }
        break;
    case 40:
        plw = (_PLW*)get_move_work_adrs(2);
        if (plw == NULL) {
            return 4;
        }
        if (strcmp(mail->id_0x14.text_0x00, plw->hunter_0x5CA.id_0x5DB) == 0) {
            work->dialog_msg = 16;
            work->dialog_kind = 1;
        } else if (PLMSG_CARD_FLAG(mail) == 0) {
            work->dialog_msg = 13;
            work->dialog_kind = 2;
        } else if (lobby_friend_has_row((const LobbyRecentRow*)mail) == 1) {
            work->dialog_msg = 14;
            work->dialog_kind = 2;
        } else if (lobby_friend_list_full() == 1) {
            work->dialog_msg = 17;
            work->dialog_kind = 2;
        } else {
            work->dialog_msg = 13;
            work->dialog_kind = 2;
        }
        work->choice = 0;
        work->step++;
        break;
    case 41:
        if (work->input & 0x10) {
            sysSE_req(0);
            if (work->dialog_kind == 1) {
                work->step = 0;
            } else if (work->choice == 0) {
                work->step = 42;
            } else {
                work->step = 0;
            }
        } else if (work->input & 0x20) {
            sysSE_req(1);
            work->step = 0;
        } else if (work->dialog_kind == 2) {
            work->choice = menu_cursor_step(work->choice, 2, work->input, 4, 8);
        }
        break;
    case 42:
        if (work->dialog_msg == 14 && lobby_friend_list_full() == 1) {
            work->dialog_msg = 17;
            work->dialog_kind = 2;
            work->choice = 0;
            work->step = 41;
        } else {
            work->step = 50;
        }
        break;
    case 50:
        sendPeerMessage(mail->id_0x14.text_0x00, mail->body_0x1F.text_0x00, PLMSG_CARD_FLAG(mail),
                        &lobby_state_block.result_0x1695);
        menu_slot0_lock_set(1);
        work->step++;
        break;
    case 51:
        if (lobby_state_block.result_0x1695 != 0) {
            if (lobby_state_block.result_0x1695 == 1) {
                sysSE_stop(52);
                lobby_state_block.mail_0x1FC4.rows_0x014[16].assign(mail);
                ret = 2;
            } else {
                ret = 4;
            }
            menu_slot0_lock_set(0);
        }
        break;
    default:
#line 1199
        nw4r::db::Panic("menu_message.cpp", __LINE__, "NW4R:Failed assertion 0");
        ret = 4;
        break;
    }
    return ret;
}

/* 0x8044E990 (0x278): steps the address picker (the friend list or a typed id); 1 once it is done, 2 to close. */
s32 plsearch_address_step(PlMessageWork* work) {
    s32 ret = 0;
    FriendPickWork* pick = &work->pick;
    LobbyMailRow* mail = &work->mail;

    switch (work->sub) {
    case 0:
        work->sub = 1;
    case 1:
        if (work->input & 0x10) {
            sysSE_req(0);
            switch (work->address_cursor) {
            case 0:
                work->sub = 10;
                break;
            case 1:
                work->sub = 20;
                break;
            }
        } else if (work->input & 0x20) {
            sysSE_req(1);
            ret = 2;
        } else {
            work->address_cursor = menu_cursor_step(work->address_cursor, 2, work->input, 1, 2);
        }
        break;
    case 10:
        friend_pick_init(pick, 1);
        work->sub++;
        break;
    case 11:
        switch (friend_pick_step(pick)) {
        case 2:
            work->sub = 1;
            break;
        case 1:
            work->sub++;
            break;
        case 3:
        case 4:
            ret = 1;
            break;
        }
        break;
    case 12:
        sprintf(mail->name_0x00.text_0x00, "%s", pick->picked_0x018->name_0x00);
        sprintf(mail->id_0x14.text_0x00, "%s", pick->picked_0x018->id_0x14);
        ret = 1;
        break;
    case 20:
        if (kbd_open(2) == 1) {
            menu_work.slot[0].field_0x32D = 1;
            memset(plsearch_kbd_text, 0, 0xE);
            strcpy(plsearch_kbd_text, mail->id_0x14.text_0x00);
            set_kbd_param(plsearch_kbd_text, 0xC);
            sprintf(mail->name_0x00.text_0x00, (char*)get_str_tbl(0x58)[4]);
            menu_slot0_lock_set(1);
            work->sub++;
        }
        break;
    case 21:
        switch (kbd_move()) {
        case 1:
            strcpy(mail->id_0x14.text_0x00, plsearch_kbd_text);
        case -1:
            menu_slot0_lock_set(0);
            menu_work.slot[0].field_0x32D = 0;
            work->sub++;
            break;
        }
        break;
    case 22:
        ret = 1;
        break;
    }
    return ret;
}

/* 0x8044EC08 (0x4C): draws the message menu (the friend picker while the address step runs it). */
void plsearch_draw(PlMessageWork* work) {
    u8 sub;

    switch (work->step) {
    default:
        plsearch_main_draw(work);
        break;
    case 10:
        sub = work->sub;
        switch (sub) {
        case 0:
        case 1:
        case 20:
        case 21:
        case 22:
            plsearch_main_draw(work);
            break;
        case 11:
        case 12:
            friend_pick_draw(&work->pick);
            break;
        }
        break;
    }
}

/* 0x8044EC54 (0x5F8): draws the message menu's page: the rows (address, card, text), the cursor, the address
 * choice, the help line and the dialog. */
void plsearch_main_draw(PlMessageWork* work) {
    s8 text[0xC4];
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    LobbyMailRow* mail;
    u32 closed;
    u32 prompt;
    u32 addr_open;
    u32 card_edit;
    bool cursor_on;
    bool hot;
    bool lit;
    s32 i;
    s32 ofs;
    u16 help;

    mail = &work->mail;
    closed = 0;
    if (work->active == 0 && work->step == 0) {
        closed = 1;
    }
    prompt = 0;
    if (closed == 1) {
        prompt = 2;
    }
    addr_open = 0;
    if (work->step == 10 && work->sub == 1) {
        addr_open = 1;
    }
    card_edit = 0;
    if (work->step == 20) {
        card_edit = 1;
    }
    cursor_on = 1;
    if (addr_open == 1 || (u32)chk_pointer() == 1) {
        cursor_on = 0;
    } else if (work->step == 30) {
        cursor_on = 0;
    }
    get_lsp_data(0x29DF, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x11C), &pos);
    switch (work->active) {
    case 0:
        draw_sprite_idx(0x29E0, &pos);
        break;
    case 1:
        draw_sprite_idx(0x29E1, &pos);
        break;
    }
    draw_font_align_idx(0x29EA, (s8*)mail->name_0x00.text_0x00, 0, &pos);

    hot = 1;
    lit = 0;
    if (closed == 1 && (u32)chk_pointer() == 1) {
        hot = 0;
    }
    if (closed == 0) {
        if (work->cursor == 0) {
            hot = (u32)chk_pointer() != 1;
            lit = addr_open == 1;
        } else {
            hot = 0;
            lit = 0;
        }
    }
    spr_data_copy(&spr, get_lsp_data(0x29EB, NULL));
    spr.color = GetMenuFontColor(1, hot, cursor_on, lit);
    draw_font(spr, (s8*)mail->id_0x14.text_0x00, 1, &pos);

    if (closed == 0) {
        if (work->cursor == 1) {
            hot = (u32)chk_pointer() != 1;
            lit = addr_open == 1;
        } else {
            hot = 0;
            lit = 0;
        }
    }
    spr_data_copy(&spr, get_lsp_data(0x29FE, NULL));
    spr.color = GetMenuFontColor(1, hot, cursor_on, lit);
    draw_font(spr, (s8*)get_menu_msg(0x38, PLMSG_CARD_FLAG(mail)), 1, &pos);

    if (closed == 0) {
        if (work->cursor == 2) {
            hot = (u32)chk_pointer() != 1;
            lit = addr_open == 1;
        } else {
            hot = 0;
            lit = 0;
        }
    }
    knj_msg_str_build((s8*)mail->body_0x1F.text_0x00, text, 24, 4);
    spr_data_copy(&spr, get_lsp_data(0x29F2, NULL));
    spr.color = GetMenuFontColor(1, hot, cursor_on, lit);
    draw_font(spr, text, 0, &pos);

    if (closed == 0 && cursor_on == 1) {
        switch (work->cursor) {
        case 0:
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x11D), &pos);
            break;
        case 1:
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x11F), &pos);
            if (card_edit == 1) {
                if (PLMSG_CARD_FLAG(mail) == 1) {
                    draw_sprite_idx(0x2A04, &pos);
                }
                if (PLMSG_CARD_FLAG(mail) == 0) {
                    draw_sprite_idx(0x2A05, &pos);
                }
            }
            break;
        case 2:
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x11E), &pos);
            break;
        case 3:
            prompt = 1;
            break;
        }
    }
    put_lsp_anchor_offset(0x2A1C, &at, pos);
    lb_cursor_prompt_draw(prompt, (s16*)&at);

    if (addr_open == 1) {
        get_lsp_data(0x2A1D, &pos);
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x120), 2, &pos);
        for (i = 0, ofs = 0; i < 2; ofs += 2, i++) {
            u16* rows = (u16*)get_menu_lsp_tbl(0x121);
            put_lsp_anchor_offset(rows[i], &at, pos);
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x122), &at);
            spr_data_copy(&spr, get_lsp_data(0x2A2B, NULL));
            spr.color = GetMenuFontColor(1, work->address_cursor == i, 1, 0);
            draw_font(spr, (s8*)get_menu_msg(0x38, i + 10), 5, &at);
            if (work->address_cursor == i && chk_pointer() == 0) {
                draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x123), &at);
            }
        }
    }

    if (closed == 0) {
        help = 0xFFFF;
        switch (work->cursor) {
        case 0:
            if (addr_open == 1) {
                help = work->address_cursor + 7;
            } else {
                help = 2;
            }
            break;
        case 1:
            help = PLMSG_CARD_FLAG(mail) + 3;
            break;
        case 2:
            help = 5;
            break;
        case 3:
            help = 6;
            break;
        }
        if (help != 0xFFFF) {
            get_lsp_data(0x29A3, &pos);
            draw_window_frame_preset(pos.x, pos.y, 0);
            font_set_size(16, 16);
            font_print_ex(pos.x + 24, pos.y + 16, 0, (s8*)get_menu_msg(0x38, help));
        }
    }

    if (work->dialog_msg != 0 && work->dialog_kind != 0) {
        plsearch_window_draw((s8*)get_menu_msg(0x38, work->dialog_msg));
        plsearch_choice_draw(work->dialog_kind, work->choice);
    }
}

/* 0x8044F24C (0x70): draws the dialog window and its message `text` (none when NULL). */
void plsearch_window_draw(s8* text) {
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;

    spr_data_copy(&spr, get_lsp_data(0x2A34, &pos));
    draw_layout_window(pos.x, pos.y, spr.width, spr.height);
    if (text != NULL) {
        draw_font_idx(0x2A38, text, 4, &pos);
    }
}

/* 0x8044F2BC (0x264): draws the dialog's buttons: one centred OK (`kind` 1) or the yes/no pair (`kind` 2) with
 * `cursor` lit (none while the pointer is in use). */
void plsearch_choice_draw(u8 kind, s8 cursor) {
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    s16 width;

    if ((u32)chk_pointer() == 1) {
        cursor = -1;
    }
    switch (kind) {
    case 1:
        get_lsp_data(0x2A33, &pos);
        spr_data_copy(&spr, get_lsp_data(0x1B7, NULL));
        width = spr.pos.x;
        spr_data_copy(&spr, get_lsp_data(0x1AF, NULL));
        width -= spr.pos.x;
        pos.x += (s16)(width / 2);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(10), &pos);
        if (cursor == 0) {
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(13), &pos);
        }
        spr_data_copy(&spr, get_lsp_data(0x1AE, NULL));
        spr.color = GetMenuFontColor(1, cursor == 0, 1, 0);
        draw_font(spr, (s8*)get_menu_msg(13, 0), 5, &pos);
        break;
    case 2:
        get_lsp_data(0x2A33, &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(10), &pos);
        if (cursor == 0) {
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(13), &pos);
        }
        spr_data_copy(&spr, get_lsp_data(0x1AE, NULL));
        spr.color = GetMenuFontColor(1, cursor == 0, 1, 0);
        draw_font(spr, (s8*)get_menu_msg(13, 0), 5, &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(11), &pos);
        if (cursor == 1) {
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(12), &pos);
        }
        spr_data_copy(&spr, get_lsp_data(0x1B6, NULL));
        spr.color = GetMenuFontColor(1, cursor == 1, 1, 0);
        draw_font(spr, (s8*)get_menu_msg(13, 1), 5, &pos);
        break;
    }
}

/* 0x8044F520 (0x10): the lobby mailbox. */
NetMailBox* getLobbyMailBox(void) {
    return (NetMailBox*)&lobby_state_block.mail_0x1FC4;
}

void lobby_card_view_draw(void);

/* 0x8044F530 (0x1F4): opens the hunter card of the lobby player `plw` stands for: fills the lobby's card record from the
 * player's message entry and this console's server settings, and opens the card view. */
void lb_plsearch_open(_PLW* plw) {
    FriendCardView* view = &friend_card_view;
    NetPeerCard* card = (NetPeerCard*)lobby_state_block.block_0x11D4;
    NetPeerRec* peer;
    NetUserProfile* profile;
    u8 index;
    s32 i;

    memset(card, 0, sizeof(NetPeerCard));
    index = hud_key_lookup((u8*)plw->hunter_0x5CA.id_0x5DB);
    if (index == 0xFF) {
        return;
    }
    peer = (NetPeerRec*)getMessageEntry(index);
    if (peer == NULL) {
        return;
    }
    if (strcmp(peer->id_0x00.text_0x00, plw->hunter_0x5CA.id_0x5DB) != 0) {
        return;
    }
    profile = (NetUserProfile*)&peer->blob_0x20;
    strcpy(card->name_0x000, plw->hunter_0x5CA.name_0x5CA);
    strcpy(card->id_0x014, plw->hunter_0x5CA.id_0x5DB);
    card_equip_ids_build(plw, card->equip_ids_0x08E);
    card->id_0x01E = profile->id_0x00;
    card->level_0x020 = profile->level_0xF3;
    card->rank_0x021 = profile->rank_0xF2;
    equip_record_copy((_EQUIP*)card->equip_0x022[6], (const _EQUIP*)profile->equip_0x10[0]);
    equip_record_copy((_EQUIP*)card->equip_0x022[7], (const _EQUIP*)profile->equip_0x10[1]);
    equip_record_copy((_EQUIP*)card->equip_0x022[8], (const _EQUIP*)profile->equip_0x10[2]);
    equip_record_copy((_EQUIP*)card->equip_0x022[4], (const _EQUIP*)profile->equip_0x10[3]);
    equip_record_copy((_EQUIP*)card->equip_0x022[0], (const _EQUIP*)profile->equip_0x10[4]);
    equip_record_copy((_EQUIP*)card->equip_0x022[2], (const _EQUIP*)profile->equip_0x10[5]);
    equip_record_copy((_EQUIP*)card->equip_0x022[1], (const _EQUIP*)profile->equip_0x10[6]);
    equip_record_copy((_EQUIP*)card->equip_0x022[3], (const _EQUIP*)profile->equip_0x10[7]);
    equip_record_copy((_EQUIP*)card->equip_0x022[5], (const _EQUIP*)profile->equip_0x10[8]);
    strcpy(card->comment_0x09E, (const char*)profile->record_0x9C);
    card->server_0x20A = NetCtrlWk::getSelectedServerTypeIndex();
    for (i = 0; i < 4; i++) {
        card->settings_0x20B[i] = NetCtrlWk::getLayerSetting(i);
    }
    card->value_0x20F = NetCtrlWk::getLayerSetting(4);
    card->value_0x210 = NetCtrlWk::getLayerSetting(5);
    strcpy(card->names_0x14A[0], NetCtrlWk::getSelectedServerName());
    strcpy(card->names_0x14A[1], NetCtrlWk::getAccountNameOrBlank());
    strcpy(card->names_0x14A[2], NetCtrlWk::getNicknameOrBlank());
    card->status_0x211 = profile->status_0x04;
    menu_friend_list_init(view, card, 1, 1);
    lobby_w.state_0x000 = 35;
    sysSE_req(5);
}

/* 0x8044F724 (0x60): steps the hunter card view; queues its draw while it runs, closes the lobby panel once done. */
void lb_plsearch_screen_step(void) {
    if (menu_friend_list_step(&friend_card_view) == 0) {
        subTransSet((u32)lobby_card_view_draw, 0, NULL);
    } else {
        lobby_w.state_0x000 = 0;
        lobby_w.active_0x008 = 0;
        lb_panel_close();
    }
}

/* 0x8044F784 (0xC): draws the hunter card view. */
void lobby_card_view_draw(void) {
    menu_friend_list_draw(&friend_card_view);
}


} /* extern "C" */

/* ---------------------------------------------------------------------------------------------------------------- */
/* The player search menus (`menu_plsearch.cpp`'s run): search by name or id, the friend list, the mailbox. */

extern "C" {

void plsearch_search_draw(MenuSlot* slot);
void plsearch_friend_list_draw(MenuSlot* slot);
void plsearch_empty_draw(MenuSlot* slot);
s32 mailbox_step(MenuSlot* slot);
u32 mailbox_cursor_valid(MenuScroll* scroll);
void mailbox_draw(MenuSlot* slot, LobbyMailList* box);
void mailbox_mail_draw(LobbyMailRow* mail, u16 msg_row, s8 cursor);
void mailbox_reply_draw(MenuSlot* slot);

/* 0x8044BDE8 (0x150): opens the player search page `slot` shows (19 by name, 20 by id, 21 the friend list). */
void plsearch_page19_open(MenuSlot* slot) {
    s32 i;
    s32 mode;

    menu_scroll_init(&slot->scroll, 0, 100, 8);
    slot->field_0x23A = 0;
    for (i = 0; i < 0x28; i++) {
        slot->text_0x2F6[i] = 0;
    }
    slot->page_index = 0;
    slot->field_0x01D = 1;
    mode = slot->field_0x014;
    switch (mode) {
    case 19:
    case 20:
        slot->field_0x001 = 0;
        slot->field_0x002 = 0;
        break;
    case 21:
        slot->field_0x001 = 20;
        slot->field_0x002 = 0;
        break;
    default:
#line 136
        nw4r::db::Panic("menu_plsearch.cpp", __LINE__, "NW4R:Failed assertion 0");
        slot->field_0x001 = 0xFF;
        break;
    }
}

/* 0x8044BF38 (0x85C): steps the player search page; 2 when it closes, 3 on a request failure, 4 when it ends. */
s32 plsearch_page19_step(MenuSlot* slot) {
    s32 ret = 0;
    MenuScroll* scroll = &slot->scroll;
    FriendCardView* view = &friend_card_view;
    LobbyTimerBlock* lobby = &lobby_state_block;
    NetFriendListView* friends = (NetFriendListView*)lobby->block_0x0488;
    u16 input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    NetPeerCard* card;
    const s32* args;

    slot->move_result_0x238 = 0;
    switch (slot->field_0x001) {
    case 0:
        slot->page_index = 0;
        slot->field_0x01D = 1;
        slot->field_0x001 = 1;
    case 1:
        if (input & 0x10) {
            switch (slot->page_index) {
            case 0:
                slot->field_0x001 = 10;
                slot->field_0x002 = 0;
                break;
            case 1:
                sysSE_req(0);
                slot->field_0x001 = 20;
                slot->field_0x002 = 0;
                break;
            }
        } else if (input & 0x20) {
            sysSE_req(1);
            ret = 2;
        } else {
            slot->page_index = menu_cursor_step(slot->page_index, (s8)slot->field_0x01D, input, 1, 2);
        }
        break;
    case 9:
        if (input & 0x10) {
            sysSE_req(0);
            slot->field_0x001 = 1;
        } else if (input & 0x20) {
            sysSE_req(1);
            slot->field_0x001 = 1;
        }
        break;
    case 10:
        switch (slot->field_0x002) {
        case 0:
            switch (slot->field_0x014) {
            case 19:
                if (kbd_open(3) == 1) {
                    slot->field_0x32D = 1;
                    set_kbd_param(slot->text_0x2F6, 16);
                    menu_slot0_lock_set(1);
                    slot->field_0x002 = 1;
                }
                break;
            case 20:
                if (kbd_open(2) == 1) {
                    slot->field_0x32D = 1;
                    set_kbd_param(slot->text_0x2F6, 12);
                    menu_slot0_lock_set(1);
                    slot->field_0x002 = 1;
                }
                break;
            }
            break;
        case 1:
            switch (kbd_move()) {
            case 1:
                slot->field_0x32D = 0;
                menu_slot0_lock_set(0);
                slot->field_0x002++;
                break;
            case -1:
                slot->field_0x32D = 0;
                menu_slot0_lock_set(0);
                slot->field_0x001 = 1;
                slot->field_0x002 = 0;
                break;
            }
            break;
        case 2:
            if (slot->text_0x2F6[0] != 0) {
                slot->field_0x01D = 2;
            } else {
                slot->field_0x01D = 1;
            }
            slot->field_0x001 = 1;
            slot->field_0x002 = 0;
            break;
        }
        break;
    case 20:
        switch (slot->field_0x002) {
        case 0:
            menu_slot0_lock_set(1);
            switch (slot->field_0x014) {
            case 19:
                if (NetCtrlWk::submitTextSelect(slot->text_0x2F6, &lobby->result_0x1695) == 0) {
                    ret = 3;
                } else {
                    slot->field_0x002++;
                }
                break;
            case 20:
                if (NetCtrlWk::submitIdSelect((const NetId*)slot->text_0x2F6, &lobby->result_0x1695) == 0) {
                    ret = 3;
                } else {
                    slot->field_0x002++;
                }
                break;
            case 21:
                if (NetCtrlWk::startLayerRefresh(&lobby->result_0x1695) == 0) {
                    ret = 3;
                } else {
                    slot->field_0x002++;
                }
                break;
            }
            break;
        case 1:
            if (lobby->result_0x1695 != 0) {
                menu_slot0_lock_set(0);
                if (lobby->result_0x1695 < 0) {
                    ret = 3;
                } else {
                    if (slot->field_0x014 == 21) {
                        NetCtrlWk::copyFriendList(friends, 1);
                    } else {
                        NetCtrlWk::copyFriendList(friends, 0);
                    }
                    slot->field_0x001++;
                    slot->field_0x002 = 0;
                }
            }
            break;
        }
        break;
    case 21:
        sysSE_req(5);
        slot->field_0x001++;
        if (friends->count_0x00 == 0) {
            if (slot->field_0x014 == 21) {
                slot->field_0x001 = 29;
            } else {
                slot->field_0x001 = 9;
            }
        }
        break;
    case 22:
        menu_scroll_init(scroll, 0, friends->count_0x00, 8);
        slot->field_0x001++;
    case 23:
        if (input & 0x10) {
            if (strcmp(friends->rows_0x04[(u8)(scroll->row + scroll->rows_per_page * scroll->col)].id_text_0x14,
                       slot->worker->hunter_0x5CA.id_0x5DB) == 0) {
                sysSE_req(2);
            } else {
                sysSE_req(5);
                slot->field_0x001 = 30;
                slot->field_0x002 = 0;
            }
        } else if (input & 0x20) {
            slot->page_index = 0;
            slot->field_0x01D = 2;
            sysSE_req(1);
            if (slot->field_0x014 == 21) {
                ret = 2;
            } else {
                slot->field_0x001 = 1;
            }
        } else {
            menu_scroll_step(scroll, input, 6);
            slot->move_result_0x238 = scroll->move_flags;
        }
        break;
    case 29:
        if (input & 0x10) {
            sysSE_req(0);
            ret = 2;
        } else if (input & 0x20) {
            sysSE_req(1);
            ret = 2;
        }
        break;
    case 30:
        switch (slot->field_0x002) {
        case 0:
            requestPeerProfileById(friends->rows_0x04[(u8)(scroll->row + scroll->rows_per_page * scroll->col)].id_text_0x14,
                                   &slot->net_result_0x320, 0, 0);
            menu_slot0_lock_set(1);
            slot->field_0x002++;
            break;
        case 1:
            if (slot->net_result_0x320 != 0) {
                menu_slot0_lock_set(0);
                if (slot->net_result_0x320 != 1) {
                    slot->field_0x001 = 23;
                } else {
                    card = (NetPeerCard*)lobby->block_0x11D4;
                    memset(card, 0, sizeof(lobby->block_0x11D4));
                    copyPeerProfileCard(card);
                    menu_friend_list_init(view, card, 2, 1);
                    slot->field_0x001++;
                    slot->field_0x002 = 0;
                }
            }
            break;
        }
        break;
    case 31:
        switch (menu_friend_list_step(view)) {
        case 0:
            break;
        case 2:
            menu_slot0_lock_set(1);
            slot->field_0x001 = 40;
            slot->field_0x002 = 0;
            break;
        case 3:
            slot->field_0x001 = 23;
            break;
        default:
            ret = 4;
            break;
        }
        break;
    case 40:
        lobby_hidden_flag = 1;
        switch (slot->field_0x002) {
        case 0:
            slot->field_0x002 = 1;
            if (menu_work.slot[0].field_0x31F != get_option_cfg(27)) {
                requestFriendSync(&slot->net_result_0x320);
                break;
            }
            slot->field_0x002 = 2;
            if (menu_work.slot[0].field_0x31E != 0) {
                resendProfileRecord(&slot->net_result_0x320);
                break;
            }
            args = (const s32*)&lobby->block_0x11D4[0x14];
            if (NetCtrlWk::requestCommand18((const s32*)&args, &slot->net_result_0x320) == 0) {
                ret = 3;
            } else {
                slot->field_0x002 = 3;
            }
            break;
        case 1:
            if (slot->net_result_0x320 != 0) {
                if (slot->net_result_0x320 != 1) {
                    set_option_cfg(27, menu_work.slot[0].field_0x31F);
                }
                slot->field_0x002 = 2;
                if (menu_work.slot[0].field_0x31E != 0) {
                    resendProfileRecord(&slot->net_result_0x320);
                    break;
                }
                args = (const s32*)&lobby->block_0x11D4[0x14];
                if (NetCtrlWk::requestCommand18((const s32*)&args, &slot->net_result_0x320) == 0) {
                    ret = 3;
                } else {
                    slot->field_0x002 = 3;
                }
            }
            break;
        case 2:
            if (slot->net_result_0x320 != 0) {
                menu_work.slot[0].field_0x31E = 0;
                args = (const s32*)&lobby->block_0x11D4[0x14];
                if (NetCtrlWk::requestCommand18((const s32*)&args, &slot->net_result_0x320) == 0) {
                    ret = 3;
                } else {
                    slot->field_0x002 = 3;
                }
            }
            break;
        case 3:
            if (slot->net_result_0x320 != 0) {
                if ((slot->net_result_0x320 < 0 || lobby_net_leave_select(slot->net_result_0x320) == 0) &&
                    lobby_net_jump_msg_get(slot->net_result_0x320) != NULL) {
                    slot->field_0x002 = 4;
                    slot->wait_0x32E = 15;
                } else {
                    menu_slot0_lock_set(0);
                    menu_item_close();
                    ret = 4;
                }
            }
            break;
        case 4:
            if (slot->wait_0x32E > 0) {
                slot->wait_0x32E--;
            } else if (input & 0x10) {
                sysSE_req(0);
                menu_slot0_lock_set(0);
                menu_item_close();
                ret = 4;
            }
            break;
        }
        break;
    default:
#line 575
        nw4r::db::Panic("menu_plsearch.cpp", __LINE__, "NW4R:Failed assertion 0");
        menu_slot0_lock_set(0);
        menu_item_close();
        ret = 4;
        break;
    }
    return ret;
}

/* 0x8044C794 (0xF8): draws the player search page (the search rows, the friend list, the card, the notices). */
void plsearch_page19_draw(MenuSlot* slot) {
    FriendCardView* view = &friend_card_view;

    if (slot->field_0x001 > 40) {
        if (slot->field_0x001 == 40) {
            put_message((s8*)lobby_net_jump_msg_get(slot->net_result_0x320), 20, 0xC0C0C0FF);
        }
    } else {
        if (slot->field_0x001 < 23) {
            plsearch_search_draw(slot);
        }
        if (slot->field_0x001 == 23 || slot->field_0x001 == 30) {
            plsearch_friend_list_draw(slot);
        }
        if (slot->field_0x001 > 30) {
            menu_friend_list_draw(view);
        }
        font_flush();
        if (slot->field_0x001 == 9 || slot->field_0x001 == 29) {
            plsearch_empty_draw(slot);
        }
        if (slot->field_0x001 == 40 && slot->field_0x002 == 4) {
            put_message((s8*)lobby_net_jump_msg_get(slot->net_result_0x320), 20, 0xC0C0C0FF);
        }
    }
}

/* 0x8044C88C (0x32C): draws the search entry page (the name or id box, its cursor, the prompt and the help line). */
void plsearch_search_draw(MenuSlot* slot) {
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    u32 cursor;
    u32 prompt;
    u32 typed;
    s32 help;

    if (slot->field_0x014 == 19 || slot->field_0x014 == 20) {
        help = 0;
        if (slot->field_0x001 < 10) {
            if (slot->field_0x014 == 19) {
                help = 2;
            }
            if (slot->field_0x014 == 20) {
                help = 3;
            }
            if ((s8)slot->field_0x01D == 2 && slot->page_index == 1) {
                help = 4;
            }
            typed = 0;
            if ((s8)slot->field_0x01D == 2) {
                typed = 1;
                if (slot->page_index == 1) {
                    prompt = 1;
                    cursor = 0;
                } else {
                    prompt = 0;
                    cursor = 1;
                }
            } else {
                prompt = 2;
                cursor = 1;
            }
        } else if (slot->field_0x001 < 20) {
            cursor = 1;
            if (slot->field_0x014 == 19) {
                help = 2;
            }
            if (slot->field_0x014 == 20) {
                help = 3;
            }
            prompt = 2;
            typed = 0;
        } else if (slot->field_0x001 < 23) {
            cursor = 0;
            prompt = 1;
            help = 4;
            typed = 1;
        } else {
            return;
        }
        get_lsp_data(0x2A3A, &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x124), &pos);
        get_lsp_data(0x2A4B, &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x125), &pos);
        if (slot->field_0x014 == 19) {
            draw_font_idx(0x2A4C, (s8*)get_menu_msg(0x37, 0), 4, &pos);
            spr_data_copy(&spr, get_lsp_data(0x2A4D, NULL));
            if (typed == 1 && slot->page_index == 1) {
                spr.color = 0xE8A40FFF;
            }
            draw_font(spr, (s8*)slot->text_0x2F6, 1, &pos);
        } else {
            draw_font_idx(0x2A4C, (s8*)get_menu_msg(0x37, 1), 4, &pos);
            spr_data_copy(&spr, get_lsp_data(0x2A4D, NULL));
            if (typed == 1 && slot->page_index == 1) {
                spr.color = 0xE8A40FFF;
            }
            draw_font(spr, (s8*)slot->text_0x2F6, 1, &pos);
        }
        if (cursor == 1 && chk_pointer() == 0) {
            put_lsp_anchor_offset(0x2A58, &at, pos);
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x126), &at);
        }
        if (prompt != 0xFF) {
            get_lsp_data(0x2A5E, &pos);
            lb_cursor_prompt_draw(prompt, (s16*)&pos);
        }
        if (help != 0) {
            get_lsp_data(0x2A5F, &pos);
            draw_window_frame_preset(pos.x, pos.y, 0);
            font_set_size(16, 16);
            font_print_ex(pos.x + 24, pos.y + 16, 0, (s8*)get_menu_msg(0x37, help));
        }
    }
}

/* 0x8044CBB8 (0x3B4): draws the found players / friends list (name, id, area and status per row), the page arrows
 * and the help line. */
void plsearch_friend_list_draw(MenuSlot* slot) {
    _SPR_DATA_ spr;
    s8 text[0x10];
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    NetFriendListView* friends = (NetFriendListView*)lobby_state_block.block_0x0488;
    u16* rows;
    s32 ofs;
    bool lit;
    bool hot;
    NetFriendView* row;
    bool other;
    s32 i;

    get_lsp_data(0x2A6F, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x127), &pos);
    get_lsp_data(0x2A92, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x12B), &pos);
    get_lsp_data(0x2A7F, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xE), &pos);
    get_lsp_data(0x2A81, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x128), &pos);
    lit = chk_pointer() == 0;
    for (i = 0, ofs = 0; i < slot->scroll.row_limit; ofs += 2, i++) {
        if (i == slot->scroll.row && lit == 1) {
            hot = 1;
        } else {
            hot = 0;
        }
        row = &friends->rows_0x04[(u8)(i + slot->scroll.col * slot->scroll.rows_per_page)];
        other = 1;
        if (strcmp(row->id_text_0x14, slot->worker->hunter_0x5CA.id_0x5DB) == 0) {
            other = 0;
        }
        get_lsp_data(0x2A7F, &pos);
        rows = (u16*)get_menu_lsp_tbl(0xF);
        put_lsp_anchor_offset(rows[i], &at, pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x10), &at);
        spr_data_copy(&spr, get_lsp_data(0x1DC, NULL));
        spr.color = GetMenuFontColor(other, hot, lit, 0);
        draw_font_align(&spr, (s8*)row->name_0x00, 4, &at);
        spr_data_copy(&spr, get_lsp_data(0x1DD, NULL));
        spr.color = GetMenuFontColor(other, hot, lit, 0);
        draw_font(spr, (s8*)row->id_text_0x14, 4, &at);
        get_lsp_data(0x2A81, &pos);
        rows = (u16*)get_menu_lsp_tbl(0x129);
        put_lsp_anchor_offset(rows[i], &at, pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x12A), &at);
        sprintf((char*)text, "%d", row->area_0x1E);
        spr_data_copy(&spr, get_lsp_data(0x2A8F, NULL));
        spr.color = GetMenuFontColor(other, hot, lit, 0);
        draw_font(spr, text, 5, &at);
        get_lsp_data(0x2A92, &pos);
        rows = (u16*)get_menu_lsp_tbl(0x12C);
        put_lsp_anchor_offset(rows[i], &at, pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x12D), &at);
        draw_sprite_uv_color_idx(0x2A9E, row->status_0x20,
                                 other == 1 ? get_rare_color(row->rare_0x21) : GetMenuFontColor(other, hot, lit, 0),
                                 &at);
        if (hot == 1) {
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x12E), &at);
        }
    }
    get_lsp_data(0x2A6E, &pos);
    PutPageArrow((u16*)get_menu_lsp_tbl(1), slot->scroll.col, slot->scroll.col_count, slot->move_result_0x238, &pos, 1);
    get_lsp_data(0x2AA6, &pos);
    draw_window_frame_preset(pos.x, pos.y, 0);
    font_set_size(16, 16);
    font_print_ex(pos.x + 24, pos.y + 16, 0, (s8*)get_menu_msg(0x37, 5));
}

/* 0x8044CF6C (0xE4): draws the "nobody found" notice with its OK button. */
void plsearch_empty_draw(MenuSlot* slot) {
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    _SPR_DATA_* spr;

    spr = get_lsp_data(0x2A60, &pos);
    draw_layout_window(pos.x, pos.y, spr->width, spr->height);
    get_lsp_data(0x2A62, &pos);
    draw_font_idx(0x299F, (s8*)get_menu_msg(0x37, 6), 1, &pos);
    spr = get_lsp_data(0x29A2, NULL);
    at.x = pos.x + spr->pos.x;
    at.y = pos.y + spr->pos.y;
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xE6), &at);
    draw_font_idx(0xAB9, (s8*)get_menu_msg(13, 0), 1, &at);
    put_menu_cursor((u16*)get_menu_lsp_tbl(0xE7), 0, &at);
}

/* 0x8044D050 (0xBC): opens the lobby mail page `slot` shows (22 a new message, 23 the mailbox, 24 a reply). */
void plsearch_page22_open(MenuSlot* slot) {
    PlMessageWork* work = &plsearch_message_work;

    switch (slot->field_0x014) {
    case 22:
        plsearch_work_init(work, 1, NULL);
        break;
    case 23:
        menu_scroll_init(&slot->scroll, 0, 16, 8);
        slot->field_0x32C = 0;
        slot->move_result_0x238 = 0;
        slot->scroll.light_colour = 0;
        slot->scroll.dark_colour = 0;
        slot->page_index = 0;
        slot->field_0x23A = 0;
        slot->field_0x001 = 0;
        slot->field_0x002 = 0;
        break;
    case 24:
        plsearch_work_init(work, 1, &lobby_state_block.mail_0x1FC4.rows_0x014[16]);
        break;
    }
}

/* 0x8044D10C (0x68): steps the lobby mail page. */
s32 plsearch_page22_step(MenuSlot* slot) {
    PlMessageWork* work = &plsearch_message_work;
    s32 ret = 0;

    switch (slot->field_0x014) {
    case 22:
    case 24:
        ret = plsearch_step(work);
        break;
    case 23:
        ret = mailbox_step(slot);
        break;
    }
    return ret;
}

/* 0x8044D174 (0x870): steps the lobby mailbox: read a mail, reply, invite its sender, delete it or ask the sender for
 * friendship; 2 when it closes, 4 when it ends.  Clears the HUD's mail notice once nothing is unread. */
s32 mailbox_step(MenuSlot* slot) {
    s32 ret = 0;
    MenuScroll* scroll = &slot->scroll;
    PlMessageWork* msg = &plsearch_message_work;
    LobbyMailList* box = &lobby_state_block.mail_0x1FC4;
    _PLW* plw = (_PLW*)get_move_work_adrs(2);
    u16 input;
    u8 index;
    s32 unread;
    s32 i;

    if (plw == NULL) {
        return 4;
    }
    input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    switch (slot->field_0x001) {
    case 0:
        if (mailbox_cursor_valid(scroll) == 0) {
            menu_scroll_init(scroll, 15, 16, 8);
        } else {
            menu_scroll_init(scroll, (u8)(slot->field_0x32C + scroll->row + scroll->rows_per_page * scroll->col), 16, 8);
        }
        slot->field_0x001 = 1;
    case 1:
        slot->field_0x32C = 0;
        if (input & 0x10) {
            index = scroll->row + scroll->rows_per_page * scroll->col;
            if (index < box->count_0x000) {
                index = box->count_0x000 - 1 - index;
                plsearch_work_init(msg, 0, &lobby_state_block.mail_0x1FC4.rows_0x014[index]);
                msg->mail.assign(&box->rows_0x014[index]);
                if (PLMSG_CARD_FLAG(&msg->mail) == 1 && box->flags_0x004[index] == 0) {
                    slot->field_0x001 = 10;
                } else {
                    slot->page_index = 0;
                    slot->field_0x001 = 20;
                }
                menu_slot0_lock_set(1);
                box->flags_0x004[index] = 1;
                sysSE_req(27);
            } else {
                sysSE_req(2);
            }
        } else if (input & 0x20) {
            sysSE_req(1);
            ret = 2;
        } else {
            menu_scroll_step(scroll, input, 6);
        }
        break;
    case 10:
        menu_slot0_lock_set(1);
        slot->field_0x18C = 0;
        slot->field_0x001++;
    case 11:
        if (input & 0x10) {
            if ((u32)NetCtrlWk::inviteFriend((const NetId*)msg->mail.id_0x14.text_0x00, slot->field_0x18C == 0,
                                             &lobby_state_block.result_0x1695) == 1) {
                sysSE_req(0);
                slot->field_0x001++;
            } else {
                menu_slot0_lock_set(0);
                sysSE_req(2);
                slot->field_0x001 = 0;
            }
        } else if (input & 0x20) {
            if ((u32)NetCtrlWk::inviteFriend((const NetId*)msg->mail.id_0x14.text_0x00, 0,
                                             &lobby_state_block.result_0x1695) == 1) {
                sysSE_req(1);
                slot->field_0x001++;
            } else {
                menu_slot0_lock_set(0);
                sysSE_req(2);
                slot->field_0x001 = 0;
            }
        } else {
            slot->field_0x18C = (s8)menu_cursor_step(slot->field_0x18C, 2, input, 4, 8);
        }
        plsearch_step(msg);
        break;
    case 12:
        if (lobby_state_block.result_0x1695 != 0) {
            if (lobby_state_block.result_0x1695 == 1) {
                slot->field_0x001++;
            } else {
                sysSE_req(2);
                slot->field_0x001 = 0;
            }
            menu_slot0_lock_set(0);
        }
        plsearch_step(msg);
        break;
    case 13:
        slot->page_index = 0;
        slot->field_0x001 = 20;
        plsearch_step(msg);
        break;
    case 20:
        menu_slot0_lock_set(0);
        slot->field_0x001 = 21;
    case 21:
        if (input & 0x10) {
            switch (slot->page_index) {
            case 0:
                msg->active = 1;
                plsearch_mail_body_clear(msg);
                sysSE_req(0);
                slot->field_0x001 = 30;
                break;
            case 1:
                if (mailbox_cursor_valid(scroll) == 1) {
                    slot->field_0x18C = 1;
                    sysSE_req(0);
                    slot->field_0x001 = 40;
                } else {
                    sysSE_req(2);
                }
                break;
            case 2:
                if (strcmp(msg->mail.id_0x14.text_0x00, plw->hunter_0x5CA.id_0x5DB) == 0) {
                    sysSE_req(2);
                } else {
                    slot->field_0x18C = 1;
                    sysSE_req(0);
                    slot->field_0x001 = 50;
                }
                break;
            }
        } else if (input & 0x20) {
            sysSE_req(1);
            slot->field_0x001 = 0;
        } else {
            slot->page_index = menu_cursor_step(slot->page_index, 3, input, 1, 2);
            plsearch_step(msg);
        }
        break;
    case 30:
        switch (plsearch_step(msg)) {
        case 2:
            slot->field_0x001 = 0;
            break;
        case 4:
            ret = 2;
            break;
        }
        break;
    case 40:
        if (input & 0x10) {
            if (slot->field_0x18C == 0) {
                if (mailbox_cursor_valid(scroll) == 1) {
                    if ((u8)(slot->field_0x32C + scroll->row + scroll->rows_per_page * scroll->col) <
                        box->count_0x000) {
                        sysSE_req(0);
                        slot->field_0x001 = 41;
                    } else {
                        sysSE_req(2);
                        slot->field_0x001 = 0;
                    }
                } else {
                    sysSE_req(2);
                    slot->field_0x001 = 0;
                }
            } else {
                sysSE_req(0);
                slot->field_0x001 = 0;
            }
        } else if (input & 0x20) {
            sysSE_req(1);
            slot->field_0x001 = 0;
        } else {
            slot->field_0x18C = (s8)menu_cursor_step(slot->field_0x18C, 2, input, 4, 8);
        }
        break;
    case 41:
        if (mailbox_cursor_valid(scroll) == 0) {
            sysSE_req(2);
            slot->field_0x001 = 0;
        } else {
            index = slot->field_0x32C + scroll->row + scroll->rows_per_page * scroll->col;
            if (index < box->count_0x000) {
                index = box->count_0x000 - 1 - index;
            } else {
                sysSE_req(2);
                slot->field_0x001 = 0;
            }
            if (dropLobbyMail(index) != 0) {
                slot->field_0x001++;
            } else {
                sysSE_req(2);
                slot->field_0x001 = 0;
            }
        }
        break;
    case 42:
        if (input & 0x10) {
            sysSE_req(0);
            slot->field_0x001 = 0;
        } else if (input & 0x20) {
            sysSE_req(1);
            slot->field_0x001 = 0;
        }
        break;
    case 50:
        if (input & 0x10) {
            sysSE_req(0);
            if (slot->field_0x18C == 0) {
                slot->field_0x001 = 51;
                slot->field_0x002 = 0;
            } else {
                slot->field_0x001 = 0;
            }
        } else if (input & 0x20) {
            sysSE_req(1);
            slot->field_0x001 = 0;
        } else {
            slot->field_0x18C = (s8)menu_cursor_step(slot->field_0x18C, 2, input, 4, 8);
        }
        break;
    case 51:
        switch (slot->field_0x002) {
        case 0:
            switch ((u8)lobby_friend_request_send((const LobbyRecentRow*)&msg->mail)) {
            case 0:
                menu_slot0_lock_set(1);
                slot->field_0x002 = 1;
                break;
            case 1:
                slot->field_0x001 = 53;
                break;
            case 2:
                slot->field_0x001 = 54;
                break;
            case 3:
                slot->field_0x001 = 55;
                break;
            }
            break;
        case 1:
            if (lobby_state_block.result_0x1695 != 0) {
                if (lobby_state_block.result_0x1695 == 1) {
                    lobby_recent_add((const LobbyRecentRow*)&msg->mail);
                    slot->field_0x001 = 52;
                } else {
                    slot->field_0x001 = 55;
                }
                menu_slot0_lock_set(0);
            }
            break;
        }
        break;
    case 52:
    case 53:
    case 54:
    case 55:
        if (input & 0x10) {
            sysSE_req(0);
            slot->field_0x001 = 0;
        } else if (input & 0x20) {
            sysSE_req(1);
            slot->field_0x001 = 1;
        }
        break;
    default:
        ret = 4;
        break;
    }
    unread = 0;
    for (i = 0; i < box->count_0x000; i++) {
        if (box->flags_0x004[i] == 0) {
            unread = 1;
        }
    }
    if (unread == 0) {
        hud_mail_notice_clear();
    }
    return ret;
}

/* 0x8044D9E4 (0x80): whether the lobby mailbox holds an unread mail (lobby mode only). */
s32 mailbox_unread_ck(void) {
    LobbyMailList* box = &lobby_state_block.mail_0x1FC4;
    s32 unread = 0;
    s32 i;

    if (GameMode_ck() == 2) {
        for (i = 0; i < box->count_0x000; i++) {
            if (box->flags_0x004[i] == 0) {
                unread = 1;
            }
        }
    }
    return unread;
}

/* 0x8044DA64 (0x20): moves the mailbox list's row offset down one (up to 255). */
void mailbox_row_offset_inc(void) {
    if (menu_work.slot[0].field_0x32C < 255) {
        menu_work.slot[0].field_0x32C++;
    }
}

/* 0x8044DA84 (0x40): whether the mailbox cursor is on one of the 16 rows. */
u32 mailbox_cursor_valid(MenuScroll* scroll) {
    s32 row = menu_work.slot[0].field_0x32C + (u8)(scroll->row + scroll->rows_per_page * scroll->col);
    return row < 16;
}

/* 0x8044DAC4 (0x1B4): draws the lobby mail page (the message menu, or the mailbox with its dialogs). */
void plsearch_page22_draw(MenuSlot* slot) {
    PlMessageWork* msg = &plsearch_message_work;
    LobbyMailList* box = &lobby_state_block.mail_0x1FC4;
    u8 sub;

    switch (slot->field_0x014) {
    case 22:
    case 24:
        plsearch_draw(msg);
        break;
    case 23:
        sub = slot->field_0x001;
        if (sub < 10) {
            mailbox_draw(slot, box);
            break;
        }
        if (sub > 10 && sub < 20) {
            mailbox_mail_draw(&msg->mail, 9, (s8)slot->field_0x18C);
        } else if (sub >= 20 && sub < 30) {
            mailbox_reply_draw(slot);
        } else {
            switch (sub) {
            case 40:
                plsearch_window_draw((s8*)get_menu_msg(0x39, 10));
                plsearch_choice_draw(2, (s8)slot->field_0x18C);
                break;
            case 42:
                plsearch_window_draw((s8*)get_menu_msg(0x39, 11));
                break;
            case 50:
            case 51:
                plsearch_window_draw((s8*)get_menu_msg(0x39, 12));
                plsearch_choice_draw(2, (s8)slot->field_0x18C);
                break;
            case 52:
                plsearch_window_draw((s8*)get_menu_msg(0x39, 13));
                break;
            case 53:
                plsearch_window_draw((s8*)get_menu_msg(0x39, 15));
                plsearch_choice_draw(1, 0);
                break;
            case 54:
                plsearch_window_draw((s8*)get_menu_msg(0x39, 14));
                plsearch_choice_draw(1, 0);
                break;
            case 55:
                plsearch_window_draw((s8*)get_menu_msg(0x39, 18));
                break;
            }
        }
        plsearch_draw(msg);
        break;
    }
}

/* 0x8044DC78 (0x35C): draws the mailbox list (eight rows of sender and preview, the unread marks, the page arrows
 * and the help line). */
void mailbox_draw(MenuSlot* slot, LobbyMailList* box) {
    _SPR_DATA_ spr;
    s8 text[0x20];
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    LobbyMailRow* row;
    u16* rows;
    s32 i;
    u16 mark;
    bool lit;
    s32 ofs;
    u8 index;
    bool hot;

    get_lsp_data(0x29A5, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x117), &pos);
    get_lsp_data(0x29B7, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x118), &pos);
    for (i = 0, ofs = 0; i < 8; ofs += 2, i++) {
        rows = (u16*)get_menu_lsp_tbl(0x119);
        put_lsp_anchor_offset(rows[i], &at, pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x11A), &at);
        if (slot->field_0x001 != 0) {
            if (chk_pointer() == 0) {
                lit = 1;
                hot = slot->scroll.row == i;
            } else {
                lit = 0;
                hot = 0;
            }
            index = i + slot->scroll.rows_per_page * slot->scroll.col;
            if (index < box->count_0x000) {
                index = box->count_0x000 - 1 - index;
                row = &box->rows_0x014[index];
                str_tbl_utf16_build((s8*)row->body_0x1F.text_0x00, text, 9);
                spr_data_copy(&spr, get_lsp_data(0x29CD, NULL));
                spr.color = GetMenuFontColor(1, hot, lit, 0);
                draw_font_align(&spr, text, 4, &at);
                spr_data_copy(&spr, get_lsp_data(0x29CE, NULL));
                spr.color = GetMenuFontColor(1, hot, lit, 0);
                if (get_option_cfg(12) == 0) {
                    draw_font_align(&spr, (s8*)row->name_0x00.text_0x00, 5, &at);
                } else {
                    draw_font(spr, (s8*)row->id_0x14.text_0x00, 5, &at);
                }
                mark = 0x29D0;
                if (box->flags_0x004[index] == 0) {
                    mark = 0x29CF;
                }
                draw_sprite_idx(mark, &at);
            } else {
                spr_data_copy(&spr, get_lsp_data(0x29CD, NULL));
                spr.color = GetMenuFontColor(0, hot, lit, 0);
                draw_font_idx(0x29CD, (s8*)get_menu_msg(0x39, 16), 4, &at);
                spr_data_copy(&spr, get_lsp_data(0x29CE, NULL));
                spr.color = GetMenuFontColor(0, hot, lit, 0);
                draw_font_idx(0x29CE, (s8*)get_menu_msg(0x32, 6), 5, &at);
            }
            if (hot == 1 && lit == 1) {
                draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x11B), &at);
            }
        }
    }
    get_lsp_data(0x29A4, &pos);
    PutPageArrow((u16*)get_menu_lsp_tbl(1), slot->scroll.col, slot->scroll.col_count, slot->move_result_0x238, &pos, 0);
    get_lsp_data(0x29A3, &pos);
    draw_window_frame_preset(pos.x, pos.y, 0);
    font_set_size(16, 16);
    font_print_ex(pos.x + 24, pos.y + 16, 0, (s8*)get_menu_msg(0x39, 6));
}

/* 0x8044DFD4 (0x134): draws the mail dialog: the sender and its id over message row `msg_row`, with the yes/no
 * buttons (`cursor` lit). */
void mailbox_mail_draw(LobbyMailRow* mail, u16 msg_row, s8 cursor) {
    _SPR_DATA_ spr;
    s8 text[0x40];
    _mh_ivec2_ pos;

    plsearch_window_draw(NULL);
    plsearch_choice_draw(2, cursor);
    get_lsp_data(0x2A34, &pos);
    sprintf((char*)text, "%s%s", (char*)get_menu_msg(0x39, 8), mail->name_0x00.text_0x00);
    spr_data_copy(&spr, get_lsp_data(0x2A35, NULL));
    spr.color = 0xF3D73EFF;
    draw_font_align(&spr, text, 4, &pos);
    sprintf((char*)text, "%s%s", (char*)get_menu_msg(0x39, 7), mail->id_0x14.text_0x00);
    spr_data_copy(&spr, get_lsp_data(0x2A36, NULL));
    spr.color = 0xF3D73EFF;
    draw_font(spr, text, 4, &pos);
    draw_font_idx(0x2A37, (s8*)get_menu_msg(0x39, msg_row), 4, &pos);
}

/* 0x8044E108 (0x1C4): draws the read mail's action menu (reply, delete, ask for friendship) and its help line. */
void mailbox_reply_draw(MenuSlot* slot) {
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    bool hot;
    bool lit;
    u16* rows;
    s32 i;
    s32 ofs;
    bool usable;
    u16 help;

    lit = (u32)chk_pointer() != 1;
    get_lsp_data(0x2A1D, &pos);
    draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x120), 3, &pos);
    for (i = 0, ofs = 0; i < 3; ofs += 2, i++) {
        rows = (u16*)get_menu_lsp_tbl(0x121);
        put_lsp_anchor_offset(rows[i], &at, pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x122), &at);
        hot = 0;
        if (slot->page_index == i && chk_pointer() == 0) {
            hot = 1;
        }
        usable = 1;
        if (i == 1 && mailbox_cursor_valid(&slot->scroll) == 0) {
            usable = 0;
        }
        spr_data_copy(&spr, get_lsp_data(0x2A2B, NULL));
        spr.color = GetMenuFontColor(usable, hot, lit, 0);
        draw_font(spr, (s8*)get_menu_msg(0x39, i), 5, &at);
        if (hot) {
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x123), &at);
        }
    }
    help = slot->page_index + 3;
    get_lsp_data(0x29A3, &pos);
    draw_window_frame_preset(pos.x, pos.y, 0);
    font_set_size(16, 16);
    font_print_ex(pos.x + 24, pos.y + 16, 0, (s8*)get_menu_msg(0x39, help));
}


} /* extern "C" */

/* 0x8044F790 (0x6C): opens the gesture page of `slot` (eight rows a page over sixteen gestures). */
extern "C" void plsearch_page11_open(MenuSlot* slot) {
    slot->field_0x014 = 11;
    slot->field_0x001 = 0;
    slot->field_0x19E = 0;
    slot->entry_count_b = 8;
    slot->field_0x1A3 = 8;
    slot->field_0x1A0 = 0;
    slot->field_0x1A1 = menu_page_count(16, 8);
    slot->field_0x1A2 = 0;
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* The gallery demo task. */

extern "C" {

/* 0x8044FB98 (0x44): 1 while the local move work plays a gallery demo. */
u32 demo_play_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work != NULL && work->demo_play_0x22E4 != 0) {
        return 1;
    }
    return 0;
}

/* 0x8044FBDC (0x50): 1 while the gallery demo runs its scene (step 2, scene state 2). */
u32 gallery_demo_running_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work != NULL && work->demo_state_0x0000 == 2 && work->demo_scene_0x0003 == 2) {
        return 1;
    }
    return 0;
}

/* 0x8044FEF8 (0x20): the demo's start step: moves on to loading once its sub-state is clear. */
void gallery_demo_start_step(Q_MoveWork* work) {
    if (work->demo_sub_0x0001 == 0) {
        work->demo_state_0x0000 = 1;
        work->demo_sub_0x0001 = 0;
    }
}

} /* extern "C" */

/* ---------------------------------------------------------------------------------------------------------------- */
/* The P-Mic terms object. */

PatTerms* sPatTerms;

extern "C" {

void setPatTermsResult(PatTerms* terms, s32 result);
void clearPatTerms(PatTerms* terms);
void resetPatTermsState(PatTerms* terms);

/* 0x804504A0 (0x10): the P-Mic request completion: stores `result` in the terms object passed as `arg`. */
void patTermsPmicCallback(s32 result, s32 arg) {
    setPatTermsResult((PatTerms*)arg, result);
}

/* 0x804504B0 (0x8): stores a P-Mic request's result. */
void setPatTermsResult(PatTerms* terms, s32 result) {
    terms->async_result_0x14 = result;
}

/* 0x804504B8 (0x3C): the shutdown thread: ends the P-Mic library and stores its result. */
/* untyped: opaque handle - the thread argument is the terms object */
void* patTermsEndThread(void* arg) {
    setPatTermsResult((PatTerms*)arg, PMICEnd());
    return NULL;
}

} /* extern "C" */

/* 0x804504F4 (0x40): publishes the terms object and clears it. */
PatTerms::PatTerms() {
    sPatTerms = this;
    clearPatTerms(this);
}

/* 0x80450534 (0x4C): unpublishes the terms object. */
PatTerms::~PatTerms() {
    sPatTerms = NULL;
}

extern "C" {

/* 0x80450580 (0xA0C): steps the terms object's P-Mic state machine (bring-up, search, start, gain, recording, stop,
 * shutdown); 1 once it rests idle, -1 once it failed. */
s32 updatePatTerms(PatTerms* terms) {
    s32 ret;

    switch (terms->state_0x0C) {
    case 0:
        if (terms->ready_0x0D != 0) {
            terms->state_0x0C = 1;
            break;
        }
        return 1;
    case 1:
        ret = terms->buffer_size_0x08;
        if ((u32)ret < 0x3E00) {
            getNetworkLogger()->warn_10("insufficient memory to call PMICInitEx(%d)\n", ret);
        }
        ret = PMICInit(terms->buffer_0x04, terms->buffer_size_0x08 - 0x3E00);
        if (ret == 0) {
            getNetworkLogger()->signal_0C(3, "ok to call PMICInit() -> %ld\n", ret);
            terms->state_0x0C = 2;
        } else {
            getNetworkLogger()->warn_10("fail to call PMICInit() -> %ld\n", ret);
            terms->state_0x0C = 19;
        }
        break;
    case 2:
        terms->state_0x0C = 3;
        break;
    case 3:
        if (terms->check_request_0x0E != 0) {
            terms->state_0x0C = 17;
            break;
        }
        ret = PMICStartSearch();
        if (ret == 0) {
            getNetworkLogger()->signal_0C(3, "P-Mic is inserted -> %ld\n", ret);
            terms->state_0x0C = 4;
        } else if (ret != -2) {
            getNetworkLogger()->warn_10("fail to call PMICProbe() -> %ld\n", ret);
            terms->state_0x0C = 19;
        }
        break;
    case 4:
        terms->async_result_0x14 = 1;
        ret = PMICStartAsync(patTermsPmicCallback, (s32)terms);
        if (ret == 0) {
            getNetworkLogger()->signal_0C(3, "ok to call PMICOpenAsync() -> %ld\n", ret);
            terms->state_0x0C = 5;
        } else {
            getNetworkLogger()->warn_10("fail to call PMICOpenAsync() -> %ld\n", ret);
            terms->state_0x0C = 2;
        }
        break;
    case 5:
        ret = terms->async_result_0x14;
        if (ret <= 0) {
            if (ret == 0) {
                getNetworkLogger()->signal_0C(3, "ok to open P-Mic -> %ld.\n", ret);
                terms->state_0x0C = 6;
            } else {
                getNetworkLogger()->warn_10("fail to open P-Mic -> %ld.\n", ret);
                terms->state_0x0C = 2;
            }
        }
        break;
    case 6:
        terms->async_result_0x14 = 1;
        ret = PMICSetGain(0, patTermsPmicCallback, (s32)terms);
        if (ret == 0) {
            getNetworkLogger()->signal_0C(3, "ok to call PMICSetFreqAsync() -> %ld.\n", ret);
            terms->state_0x0C = 7;
        } else {
            getNetworkLogger()->warn_10("fail to call PMICSetFreqAsync() -> %ld.\n", ret);
            terms->state_0x0C = 2;
        }
        break;
    case 7:
        ret = terms->async_result_0x14;
        if (ret <= 0) {
            if (ret == 0) {
                getNetworkLogger()->signal_0C(3, "ok to set P-Mic freq -> %ld.\n", ret);
                terms->state_0x0C = 8;
            } else {
                getNetworkLogger()->warn_10("fail to set P-Mic freq -> %ld.\n", ret);
                terms->state_0x0C = 2;
            }
        }
        break;
    case 8:
        if (terms->update_request_0x0F != 0) {
            terms->state_0x0C = 9;
            break;
        }
        if (terms->check_request_0x0E != 0) {
            terms->state_0x0C = 14;
            break;
        }
        ret = PMICStartSearch();
        if (ret == -2) {
            getNetworkLogger()->signal_0C(3, "P-Mic is removed -> %ld\n", ret);
            terms->state_0x0C = 2;
        } else if (ret != 0) {
            getNetworkLogger()->warn_10("fail to call PMICProbe() -> %ld\n", ret);
            terms->state_0x0C = 19;
        }
        break;
    case 9:
        terms->async_result_0x14 = 1;
        ret = PMICStartRecordingAsync(patTermsPmicCallback, (s32)terms);
        if (ret == 0) {
            getNetworkLogger()->signal_0C(3, "ok to call PMICStartAsync() -> %ld\n", ret);
            terms->state_0x0C = 10;
        } else {
            getNetworkLogger()->warn_10("fail to call PMICStartAsync() -> %ld\n", ret);
            terms->state_0x0C = 2;
        }
        break;
    case 10:
        ret = terms->async_result_0x14;
        if (ret <= 0) {
            if (ret == 0) {
                getNetworkLogger()->signal_0C(3, "ok to start P-Mic -> %ld.\n", ret);
                terms->state_0x0C = 11;
            } else {
                getNetworkLogger()->warn_10("fail to start P-Mic -> %ld.\n", ret);
                terms->state_0x0C = 2;
            }
        }
        break;
    case 11:
        if (terms->async_result_0x14 > 0) {
            break;
        }
        if (terms->cancel_request_0x10 != 0) {
            terms->state_0x0C = 12;
            terms->progress_0xE0 = 0;
            break;
        }
        if (terms->check_request_0x0E != 0) {
            terms->state_0x0C = 14;
            terms->progress_0xE0 = 0;
            break;
        }
        ret = PMICStartSearch();
        if (ret == -2) {
            getNetworkLogger()->signal_0C(3, "P-Mic is removed -> %ld\n", ret);
            terms->state_0x0C = 2;
            terms->progress_0xE0 = 0;
        } else if (ret != 0) {
            getNetworkLogger()->warn_10("fail to call PMICProbe() -> %ld\n", ret);
            terms->state_0x0C = 19;
            terms->progress_0xE0 = 0;
        } else {
            terms->async_result_0x14 = 1;
            if (PMICGetParam(1, (u32*)&terms->progress_0xE0, patTermsPmicCallback, (s32)terms) != 0) {
                terms->async_result_0x14 = 0;
            }
        }
        break;
    case 12:
        terms->async_result_0x14 = 1;
        ret = PMICStopRecordingAsync(patTermsPmicCallback, (s32)terms);
        if (ret == 0) {
            getNetworkLogger()->signal_0C(3, "ok to call PMICStopAsync() -> %ld\n", ret);
            terms->state_0x0C = 13;
        } else {
            getNetworkLogger()->warn_10("fail to call PMICStopAsync() -> %ld\n", ret);
            terms->state_0x0C = 2;
        }
        break;
    case 13:
        ret = terms->async_result_0x14;
        if (ret <= 0) {
            if (ret == 0) {
                getNetworkLogger()->signal_0C(3, "ok to stop P-Mic -> %ld.\n", ret);
                terms->state_0x0C = 8;
            } else {
                getNetworkLogger()->warn_10("fail to stop P-Mic -> %ld.\n", ret);
                terms->state_0x0C = 2;
            }
        }
        break;
    case 14:
        terms->async_result_0x14 = 1;
        ret = PMICStopAsync(patTermsPmicCallback, (s32)terms);
        if (ret == 0) {
            getNetworkLogger()->signal_0C(3, "ok to call PMICCloseAsync() -> %ld\n", ret);
            terms->state_0x0C = 15;
        } else if (ret != -5) {
            getNetworkLogger()->warn_10("fail to call PMICCloseAsync() -> %ld\n", ret);
            terms->state_0x0C = 16;
        }
        break;
    case 15:
        ret = terms->async_result_0x14;
        if (ret <= 0) {
            if (ret == 0) {
                getNetworkLogger()->signal_0C(3, "ok to close P-Mic -> %ld.\n", ret);
                terms->state_0x0C = 16;
            } else {
                getNetworkLogger()->warn_10("fail to close P-Mic -> %ld.\n", ret);
                if (terms->async_result_0x14 == -6) {
                    terms->state_0x0C = 14;
                } else {
                    terms->state_0x0C = 16;
                }
            }
        }
        break;
    case 16:
        if (terms->ready_0x0D != 0) {
            terms->state_0x0C = 4;
        } else if (terms->check_request_0x0E != 0) {
            terms->state_0x0C = 17;
        }
        break;
    case 17:
        if (OSCreateThread(terms->thread_0xE8, (void*)patTermsEndThread, terms, terms->stack_0x400 + 0x8000, 0x8000,
                           14, 1) == 1) {
            terms->async_result_0x14 = 1;
            OSResumeThread(terms->thread_0xE8);
            terms->state_0x0C = 18;
        } else {
            terms->state_0x0C = 19;
        }
        break;
    case 18:
        if (OSIsThreadTerminated(terms->thread_0xE8)) {
            ret = terms->async_result_0x14;
            if (ret == 0) {
                getNetworkLogger()->signal_0C(3, "ok to call PMICQuit() -> %ld\n", ret);
                terms->buffer_0x04 = NULL;
                terms->state_0x0C = 0;
            } else {
                getNetworkLogger()->warn_10("fail to call PMICQuit() -> %ld\n", ret);
                switch (terms->async_result_0x14) {
                case -5:
                    terms->state_0x0C = 17;
                    break;
                case -4:
                    terms->state_0x0C = 14;
                    break;
                default:
                    terms->state_0x0C = 19;
                    break;
                }
            }
        }
        break;
    case 19:
        getNetworkLogger()->warn_10("die of fail P-Mic -> %ld\n", terms->async_result_0x14);
        terms->async_result_0x14 = -100;
        terms->state_0x0C = 20;
    case 20:
        return -1;
    }
    return 0;
}

/* 0x80450F8C (0x40): hands the terms object its work area once and marks it ready. */
/* untyped: byte range - the P-Mic work area */
void initPatTerms(PatTerms* terms, void* buffer, u32 size) {
    if (terms->buffer_0x04 == NULL) {
        if (buffer != NULL) {
            terms->buffer_0x04 = (u8*)buffer;
            terms->buffer_size_0x08 = size;
            terms->ready_0x0D = 1;
            terms->check_request_0x0E = 0;
            terms->update_request_0x0F = 0;
            terms->cancel_request_0x10 = 0;
            resetPatTermsState(terms);
        }
    }
}

/* 0x80450FCC (0x24): clears the terms object and resets its state. */
void clearPatTerms(PatTerms* terms) {
    terms->buffer_0x04 = NULL;
    terms->buffer_size_0x08 = 0;
    terms->state_0x0C = 0;
    terms->ready_0x0D = 0;
    terms->check_request_0x0E = 0;
    terms->update_request_0x0F = 0;
    terms->cancel_request_0x10 = 0;
    resetPatTermsState(terms);
}

/* 0x80450FF0 (0x34): resets the result (-100 once failed), the echo suppressor and the progress. */
void resetPatTermsState(PatTerms* terms) {
    s32 result = 0;

    if (terms->state_0x0C == 20) {
        result = -100;
    }
    terms->async_result_0x14 = result;
    terms->reserved_0xD8 = 1;
    terms->echo_reset_0xDC = 1;
    terms->progress_0xE0 = 0;
    terms->flag_0xE2 = 0;
}

/* 0x80451024 (0x1C): asks for the terms check (drops the ready byte and any update). */
void requestPatTermsCheck(PatTerms* terms) {
    terms->ready_0x0D = 0;
    terms->check_request_0x0E = 1;
    terms->update_request_0x0F = 0;
    terms->cancel_request_0x10 = 0;
}

/* 0x80451040 (0x2C): asks for the terms update while the object is ready. */
void requestPatTermsUpdate(PatTerms* terms) {
    if (terms->ready_0x0D == 0) {
        return;
    }
    terms->reserved_0xD8 = 1;
    terms->echo_reset_0xDC = 1;
    terms->progress_0xE0 = 0;
    terms->update_request_0x0F = 1;
    terms->cancel_request_0x10 = 0;
}

/* 0x8045106C (0x20): cancels the terms update while the object is ready. */
void cancelPatTermsUpdate(PatTerms* terms) {
    if (terms->ready_0x0D == 0) {
        return;
    }
    terms->update_request_0x0F = 0;
    terms->cancel_request_0x10 = 1;
}

/* 0x8045108C (0x70): reads `size` bytes of voice samples into `out` once the update finished; the byte count. */
s32 readPatTermsVoice(PatTerms* terms, u8* out, s32 size) {
    s32 count;

    if (!isTermsUpdateFinished(terms)) {
        return 0;
    }
    count = PMICRead((s16*)out, size / 2);
    return (count < 0) ? count : count * 2;
}

/* 0x804510FC (0x1EC): writes up to 0x180 bytes of `in` (every other sample pair) as playback, silence while the
 * terms flag is up; the byte count. */
s32 writePatTermsVoice(PatTerms* terms, const s16* in, s32 size) {
    s32 count;
    s32 i;
    s16* out;

    count = (size / 2 < 192) ? size / 2 : 192;
    if (!isTermsUpdateFinished(terms)) {
        return 0;
    }
    count = (count / 2 < 96) ? count / 2 : 96;
    if (getPatTermsFlag(terms)) {
        memset(terms->voice_out_0x18, 0, sizeof(terms->voice_out_0x18));
    } else {
        out = terms->voice_out_0x18;
        for (i = 0; i < count / 2; i++) {
            out[0] = in[0];
            out[1] = in[1];
            out += 2;
            in += 4;
        }
    }
    count = PMICWrite(terms->voice_out_0x18, count);
    return (count < 0) ? count : count * 2;
}

/* 0x804512E8 (0xC0): runs the echo suppressor over `size` bytes of `in` into `out` once the update finished. */
s32 suppressPatTermsEcho(PatTerms* terms, const u8* in, u8* out, s32 size) {
    s32 ret;

    if (!isTermsUpdateFinished(terms)) {
        return 0;
    }
    ret = PMICFilterSamples((s16*)in, (s16*)out, size / 2, terms->echo_reset_0xDC, NULL);
    if (ret == 0) {
        terms->echo_reset_0xDC = 0;
    } else {
        getNetworkLogger()->warn_10("fail to suppress echo P-Mic -> %ld.\n", ret);
    }
    return ret;
}

/* 0x804513A8 (0x8): the terms progress. */
u16 getPatTermsProgress(PatTerms* terms) {
    return terms->progress_0xE0;
}

} /* extern "C" */
