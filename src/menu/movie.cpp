/*
 * menu/movie.cpp - movie playback (`Movie_open`, `Movie_draw_sub`, `Movie_close`, `Check_movie_finish`, `Movie_cut`)
 *   over a copy of the SDK's THPSimple player and THPDraw helpers, the disc-error screen (`movie_drive_wait`), the
 *   friend picker and hunter card view (`menu_friendlist.cpp`), the option pages 4/17/18, the character make
 *   (`charmake_init`, `charmake_move`, `get_charmake_param`, `set_charmake_r_no`) and the demo equipment
 *   (`set_demo_equip_data`, `demoEquip`).
 * RANGE. .text 0x8043D524-0x804459DC (112 functions); extab 0x8001DBEC-0x8001DE9C, extabindex
 *   0x8003E718-0x8003EB20, .ctors 0x8056F3CC, .data 0x806047D0-0x80604D2C, .bss 0x806E26E0-0x806E3E10, .sdata
 *   0x80793A80-0x80793B28, .sbss 0x80794D08-0x80794D18, .sdata2 0x8079C8D0-0x8079C960.  The left edge is the
 *   `movie.cpp` `__FILE__` anchor, the right one `quest/arenatask.cpp`.
 * FLAGS. `cflags_menu` (configure.py); file-scope `#pragma pool_data off` (MWCC otherwise addresses the strings and
 *   tables through one pooled base); `#pragma optimization_level 4` over the friend-list part only (its range compares
 *   and register colouring are -O4's: `friend_pick_draw` matches only under it and five other rows gain; the movie
 *   part loses rows under it and the character make is unchanged).
 * NAMES. Module `menu` and the file from the `__FILE__` string "movie.cpp".  The THPSimple/THPDraw functions carry the
 *   SDK sample's names (the panic strings "THPSimpleOpen fail"/"THPSimplePreLoad fail" and the map's
 *   `THPSimpleGetTotalFrame` name the copy); its record layout is the sample's minus the audio fields.
 *   GUESS (from each body and its callers): `movie_drive_wait`, `movie_change_request`, `menu_friend_list_init`,
 *   `menu_friend_list_step`, `menu_friend_list_draw`, `movie_frame_draw`, `movie_playing_ck`, `movie_work`,
 *   `movie_table`, `dvd_error_*`, `dvd_error`, `system_shutdown_prepare`, `thp_gx_*`, `page18_pick_work`,
 *   `friend_pick_list_draw`, `friend_card_*`, `card_equip_ids_build`, `page04_*`, `page17_*`, `option_value_step`,
 *   `option_value_sprite_draw`, `option_cursor_draw`, `arena_value_step`, `terms_progress_draw`, `charmake_*`,
 *   `demo_equip_*`, the types `MovieWork`, `DvdError`, `FriendCardView`, `CharmakeWork`, `DemoEquipSet`, and every
 *   foreign name this file spells from the name registry (lane menu-movie).
 * RESIDUALS. The range is three TUs (`.data` V->S seam at 0x806048B0, the one-copy `__FILE__` strings "movie.cpp" and
 *   "menu_friendlist.cpp", `.sdata` "%s" twice (0x80793AE8, 0x80793AF0), `.sdata2` 1.0f/0.0f/the int-to-float double
 *   re-pooled at 0x8079C900 and 0x8079C930): `movie.cpp` 0x8043D524-0x8043F2FC, `menu_friendlist.cpp`
 *   0x8043F2FC-0x80443554 (.data 0x806048B0-0x806049D8, .bss 0x806E28B0-0x806E3AC8, .sdata 0x80793AF0-0x80793B08,
 *   .sdata2 0x8079C900-0x8079C930) and the character make 0x80443554-0x804459DC; the unit cannot flip until it is
 *   re-cut (request menu-movie#1), and the data sections are emitted in this one file's order.
 *  - `NetworkFriendInfo`'s inline destructor is emitted here as a weak function (retail's table points at the copy
 *    `Network/NetworkCommunityPat.cpp` emits), so `.text` is 0x44 bytes longer than retail's.
 *  - `THPSimpleOpen`: the header's component offset takes r29 where retail splits it (r30, then r29 after `+= 0x14`).
 *  - `THPSimpleCalcNeedMemory`, `THPSimpleSetBuffer`: `mullw` takes the size operands in the other order.
 *  - `VideoDecode`: `THP/THPDec.h` returns `u8`, so the result gets a `clrlwi` retail lacks (request menu-movie#2).
 *  - `friend_pick_step`: retail's friend-request step jumps from sub-steps 0 and 2 into the tails of sub-steps 1 and 2;
 *    the source repeats the tails (rule 8), so the function is 0x80 bytes longer.
 *  - `menu_friend_list_step`, `movie_page04_draw`, `movie_page04_split_draw`, the `*_rows_build` rows: retail lowers
 *    the case ranges in another order (range compares first) than MWCC builds from these switches.
 *  - `option_value_sprite_draw`: retail copies the `_SPR_DATA_` record by words, MWCC by fields (the record's layout).
 *  - `page17_transfer_get`: retail keeps the 0/1 returns as branches where MWCC folds them branch-free.
 *  - The disc-error, THPDraw and character-make rows below 100 differ only on relocation names (registry names not in
 *    the map yet) or one register.
 *  - `.sdata2`: retail pools 0.0f before the four TEV colours, ours after them.
 */

#include "types.h"
#include "gx.h"
#include "gx/GXRenderModeObj.h"
#include "MTX/mtx.h"
#include "MTX/mtx44.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/OSReset.h"
#include "OS/DCInvalidateRange.h"
#include "DVD/dvd.h"
#include "DVD/dvdfs.h"
#include "VI/vi.h"
#include "AI_SDK/ai.h"
#include "MIX/mix.h"
#include "SEQ/seq.h"
#include "SYN/syn.h"
#include "WPAD/wpad.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXMisc.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXFrameBuf.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTexture.h"
#include "RVLGX/GXTransform.h"
#include "THP/THPDec.h"
#include "MSL_C/string.h"
#include "MSL_C/strcat.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "main.h"
#include "mh3_pad.h"
#include "mh3_pad/system_w.h"
#include "mh3_pad/lb_param_w.h"
#include "ef/system_core.h"
#include "ef/fn_800CDB2C.h"
#include "font/flfnt.h"
#include "fn_8004CAD8.h"
#include "menu/menu_item.h"
#include "lobby/lb_quest_screen.h"
#include "Network/network_pat_control.h"
#include "sound/fn_800D7F54.h"
#include "sound/fn_800F2A94.h"
#include "menu/Movie_open.h"
#include "dvd_drive_status.h"
#include "Rmode.h"
#include "sound/snd_master_volume_set.h"
#include "sound/snd_track_pause.h"
#include "pmic_stop.h"
#include "font/flfnt_frame_reset.h"
#include "font/flfntGetSizeX.h"
#include "font/dbg_drawgraph.h"
#include "draw_rect_set.h"
#include "draw_rect_fill.h"
#include "ef/dvd_read_wait.h"
#include "ef/hbm_enable.h"
#include "kbd_close_call.h"
#include "enemy/lobby_state_block.h"
#include "enemy/em020_prog.h"
#include "Network/net_session_close.h"
#include "hud/layout.h"
#include "hud/put_lsp_anchor_offset.h"
#include "hud/cockpit.h"
#include "menu/menu_message.h"
#include "menu/get_pop_dat_ptr.h"
#include "lobby/lb_npc.h"
#include "mh3_pad/Psw.h"
#include "nw4r/db_assert.h"
#include "MSL_C/printf.h"
#include "menu/menu_friend_list_init.h"
#include "menu/friend_card_view.h"
#include "menu/movie_page18_open.h"
#include "menu/movie_page18_step.h"
#include "menu/movie_page18_draw.h"
#include "menu/plsearch_work_init.h"
#include "lobby/lb_player_menu_area_ck.h"
#include "enemy/lobby_friend_remove.h"
#include "menu/info_gun_parts_draw.h"
#include "Pl/pl_skill_tree_get.h"
#include "menu/placeinfo_server_rows_draw.h"
#include "menu/chara_icon_tex_tbl.h"
#include "menu/card_equip_ids_build.h"
#include "menu/put_menu_cursor.h"
#include "menu/Put_equip_dtl_syo_item1.h"
#include "menu/Set_equip_column_arrangement.h"
#include "Pl/fn_8027D684.h"
#include "Pl/pl_status_sheet_get.h"
#include "userdata_item.h"
#include "hud/draw_window_box.h"
#include "hud/draw_layout_window.h"
#include "ef/get_move_work_adrs.h"
#include "menu/ck_cfg.h"
#include "menu/option_cfg_max.h"
#include "hud/draw_sprite_blink_idx.h"
#include "disp_beta.h"
#include "color_black.h"
#include "mh3_pad/control.h"
#include "menu/movie_page04_open.h"
#include "menu/movie_page04_step.h"
#include "menu/movie_page04_draw.h"
#include "menu/movie_page04_split_open.h"
#include "menu/movie_page04_split_step.h"
#include "menu/movie_page04_split_draw.h"
#include "menu/movie_page17_open.h"
#include "menu/movie_page17_step.h"
#include "menu/movie_page17_draw.h"
#include "quest/quest_types.h"
#include "quest/quest_entry.h"
#include "unsplit/stage.h"
#include "stage/stg_w.h"
#include "light/light.h"
#include "g3d/g3d_scnroot.h"
#include "g3d/g3d_camera_types.h"
#include "g3d/g3d_anmscn.h"
#include "ef/pRoot.h"
#include "ef/eft_rot_vec_copy.h"
#include "ef/work_mem_free.h"
#include "mh3_pad/Screen_w.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_80273B14.h"
#include "Pl/fn_80262940.h"
#include "Pl/fn_80288CEC.h"
#include "Pl/pl_yure.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_net_slot_ready_ck.h"
#include "sound/snd_bank_loader.h"
#include "sound/fn_800EF7D8.h"
#include "sound/fn_800E3CBC.h"
#include "sound/fn_800E46E8.h"
#include "nw_resource.h"
#include "fn_80056F24.h"
#include "fn_80040598.h"
#include "fn_80047398.h"
#include "MSL/strlen.h"
#include "face_work_init.h"
#include "userdata_clear.h"
#include "g3d/camera_posture_get.h"
#include "sound/pl_voice_bank_load.h"
#include "Pl/pl_obj_ready_ck.h"
#include "Pl/pl_parts_set.h"
#include "sound/snd_stream_reloc_busy_ck.h"
#include "sound/se_bank_req.h"
#include "lobby/lb_cursor_box_step.h"
#include "lobby/get_change_hair_color.h"
#include "menu/movie.h"

#pragma pool_data off

/* ---------------------------------------------------------------------------------------------------------------- */
/* The THPSimple player's records (the SDK sample's, without its audio track). */

/* size: 0x30 - the THP file header. */
typedef struct THPHeader {
    /* +0x00 */ char magic[4];
    /* +0x04 */ u32 version;
    /* +0x08 */ u32 bufSize;
    /* +0x0C */ u32 audioMaxSamples;
    /* +0x10 */ f32 frameRate;
    /* +0x14 */ u32 numFrames;
    /* +0x18 */ u32 firstFrameSize;
    /* +0x1C */ u32 movieDataSize;
    /* +0x20 */ u32 compInfoDataOffsets;
    /* +0x24 */ u32 offsetDataOffsets;
    /* +0x28 */ u32 movieDataOffsets;
    /* +0x2C */ u32 finalFrameDataOffsets;
} THPHeader;

/* size: 0x14 - the frame's component list. */
typedef struct THPFrameCompInfo {
    /* +0x00 */ u32 numComponents;
    /* +0x04 */ u8 frameComp[16];
} THPFrameCompInfo;

/* size: 0xC - the video component's description. */
typedef struct THPVideoInfo {
    /* +0x0 */ u32 xSize;
    /* +0x4 */ u32 ySize;
    /* +0x8 */ u32 videoType;
} THPVideoInfo;

/* size: 0xC - one prefetched frame. */
typedef struct THPReadBuffer {
    /* +0x0 */ u8* ptr;
    /* +0x4 */ s32 frameNumber;
    /* +0x8 */ BOOL isValid;
} THPReadBuffer;

/* size: 0x10 - the decoded planes. */
typedef struct THPTextureSet {
    /* +0x0 */ u8* ytexture;
    /* +0x4 */ u8* utexture;
    /* +0x8 */ u8* vtexture;
    /* +0xC */ s32 frameNumber;
} THPTextureSet;

/* size: 0xC - the head of one frame in the file: the next frame's size, this one's, then one size per component
 * (an approximation: the size list runs to `numComponents`). */
typedef struct THPFrameHeader {
    /* +0x0 */ u32 nextFrameSize;
    /* +0x4 */ u32 prevFrameSize;
    /* +0x8 */ u32 compSize[1];
} THPFrameHeader;

#define THP_READ_BUFFER_NUM 10
/* The ring index after `i`. */
#define THP_NEXT_INDEX(i) ((i) + 1 >= THP_READ_BUFFER_NUM ? 0 : (i) + 1)
/* The SDK's round-up to the next 32-byte boundary. */
#define OSRoundUp32B(x) (((u32)(x) + 31) & ~31)

/* size: 0x13C - the player's state (the sample's `SimpleControl`). */
typedef struct THPSimpleControl {
    /* +0x000 */ DVDFileInfo fileInfo;
    /* +0x03C */ THPHeader header;
    /* +0x06C */ THPFrameCompInfo compInfo;
    /* +0x080 */ THPVideoInfo videoInfo;
    /* +0x08C */ u8* thpWork;            /* the decoder's work area, after the read buffers */
    /* +0x090 */ BOOL open;
    /* +0x094 */ u8 preFetchState;
    /* +0x095 */ u8 loop;
    /* +0x096 */ u8 pad_0x096[2];
    /* +0x098 */ s32 curOffset;
    /* +0x09C */ BOOL readError;
    /* +0x0A0 */ BOOL readBusy;
    /* +0x0A4 */ s32 nextDecodeIndex;
    /* +0x0A8 */ s32 readIndex;
    /* +0x0AC */ s32 readSize;
    /* +0x0B0 */ s32 readFrame;
    /* +0x0B4 */ THPReadBuffer readBuffer[THP_READ_BUFFER_NUM];
    /* +0x12C */ THPTextureSet textureSet;
} THPSimpleControl;

/* size: 0x1C0 - the movie player: the shown movie's size and timing, the THPSimple state, its read scratch. */
typedef struct MovieWork {
    /* +0x000 */ THPVideoInfo video;      /* the open movie's size */
    /* +0x00C */ u8* buffer;              /* the frame memory the player was given */
    /* +0x010 */ u32 start_retrace;       /* the retrace count of the last drawn frame */
    /* +0x014 */ s32 frame;               /* the last drawn frame */
    /* +0x018 */ s32 first_frame;         /* 1 until the first frame is drawn (`Movie_cut` waits for it) */
    /* +0x01C */ s32 letterbox;           /* the rows trimmed from the top and bottom on a 4:3 screen */
    /* +0x020 */ THPSimpleControl control;
    /* +0x15C */ u8 pad_0x15C[4];
    /* +0x160 */ u8 read_buffer[64];      /* the header and component reads (the sample's `WorkBuffer`) */
    /* +0x1A0 */ BOOL initialized;
    /* +0x1A4 */ u8 playing;
    /* +0x1A5 */ u8 pad_0x1A5[7];
    /* +0x1AC */ u8* frame_memory;
    /* +0x1B0 */ u8 pad_0x1B0[0x10];
} MovieWork;

/* size: 0x8 - one movie: its file stem and whether a 4:3 screen letterboxes it. */
typedef struct MovieEntry {
    /* +0x0 */ char* name;
    /* +0x4 */ u8 letterbox;
} MovieEntry;

/* size: 0x10 - the disc-error screen: its step, the message, and the state it saved. */
typedef struct DvdError {
    /* +0x0 */ s32 step;
    /* +0x4 */ s32 message;              /* the system message shown (-1: none) */
    /* +0x8 */ u8 saved_pad_lock;        /* `system_w`'s +0x05 before the screen */
    /* +0x9 */ u8 saved_dimming;         /* VI dimming before the screen */
    /* +0xA */ u8 active;
    /* +0xB */ u8 saved_hbm_disabled;
    /* +0xC */ u8 pad_0x0C;
    /* +0xD */ u8 change_request;
    /* +0xE */ u8 pad_0x0E[2];
} DvdError;

MovieEntry movie_table[11] = {
    { "movie01", 0 }, { "movie02_eur", 1 }, { "movie03", 1 }, { "movie04", 1 }, { "movie05", 1 }, { "movie06", 1 },
    { "movie07", 1 }, { "movie08", 1 },     { "movie09", 1 }, { "movie10", 1 }, { "demo39", 0 },
};

MovieWork movie_work;
DvdError dvd_error;

extern "C" {
BOOL THPSimpleInit(void);
void THPSimpleQuit(void);
BOOL THPSimpleOpen(char* fileName);
BOOL THPSimpleClose(void);
u32 THPSimpleCalcNeedMemory(void);
BOOL THPSimpleSetBuffer(u8* buffer);
BOOL THPSimplePreLoad(s32 loop);
BOOL THPSimpleLoadStop(void);
s32 THPSimpleDecode(void);
s32 THPSimpleDrawCurrentFrame(GXRenderModeObj* rmode, u32 x, u32 y, u32 polygonW, u32 polygonH);
BOOL THPSimpleGetVideoInfo(THPVideoInfo* videoInfo);
static void dvd_error_message_draw(void);
}

/* 0x8043D524 (0x1B4): opens movie `index`, preloads it and starts playback. */
void Movie_open(MovieIndex index) {
    char path[32];

    hbm_disable();
    if (index >= 11) {
        OSPanic(__FILE__, 210, "Over Movie Index");
    }
    strcpy(path, "17/");
    strcat(path, movie_table[index].name);
    if (index == 10 && lb_param_w.rank_sel_0x09 == 1) {
        strcat(path, "n");
    }
    if ((index == 0 || index == 10) && ck_WideMode() == 0) {
        strcat(path, "a");
    }
    strcat(path, ".thp");
    movie_work.letterbox = 0;
    if (ck_WideMode() == 0 && movie_table[index].letterbox == 1) {
        movie_work.letterbox = 60;
    }
    movie_work.frame = 0;
    movie_work.first_frame = 1;
    movie_work.playing = 0;
    THPSimpleInit();
    if (THPSimpleOpen(path) == FALSE) {
        OSPanic(__FILE__, 250, "THPSimpleOpen fail");
    }
    THPSimpleGetVideoInfo(&movie_work.video);
    THPSimpleCalcNeedMemory();
    movie_work.frame_memory = (u8*)0x90348000;
    movie_work.buffer = (u8*)0x90348000;
    THPSimpleSetBuffer((u8*)0x90348000);
    if (THPSimplePreLoad(0) == FALSE) {
        OSPanic(__FILE__, 265, "THPSimplePreLoad fail");
    }
}

/* 0x8043D6D8 (0xC8): decodes the next frame and draws it centred on the screen. */
static void movie_frame_draw(void) {
    MovieWork* work;

    movie_work.start_retrace = VIGetRetraceCount();
    if (system_w.field_0x05 == 0 && THPSimpleDecode() == 1) {
        OSPanic(__FILE__, 282, "Failed to decode video data");
    }
    work = &movie_work;
    work->frame = THPSimpleDrawCurrentFrame(Rmode, (Rmode->fbWidth - movie_work.video.xSize) >> 1,
                                            (Rmode->efbHeight - work->video.ySize) >> 1, movie_work.video.xSize,
                                            work->video.ySize);
    if (work->first_frame != 0) {
        work->first_frame = 0;
    }
    movie_work.playing = 1;
}

/* 0x8043D7A0 (0x4): the movie's draw callback. */
void Movie_draw_sub(void) {
    movie_frame_draw();
}

/* 0x8043D7A4 (0x40): stops and closes the movie. */
void Movie_close(void) {
    THPSimpleLoadStop();
    THPSimpleClose();
    THPSimpleQuit();
    VIWaitForRetrace();
    hbm_enable();
    movie_work.playing = 0;
}

/* 0x8043D7E4 (0x44): 1 once the last frame has been drawn. */
u32 Check_movie_finish(void) {
    return movie_work.frame >= (u32)(THPSimpleGetTotalFrame() - 1);
}

/* 0x8043D828 (0x40): closes the movie once its first frame is up; 1 when it closed. */
BOOL Movie_cut(void) {
    if (movie_work.first_frame != 0) {
        return FALSE;
    }
    Movie_close();
    return TRUE;
}

/* 0x8043D868 (0x10): 1 while a movie is on screen. */
u8 movie_playing_ck(void) {
    return movie_work.playing;
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* THPSimple. */

extern "C" {

/* 0x8043D878 (0x64): clears the player and starts the decoder. */
BOOL THPSimpleInit(void) {
    BOOL old;

    memset(&movie_work.control, 0, sizeof(THPSimpleControl));
    if (!THPInit()) {
        return FALSE;
    }
    old = OSDisableInterrupts();
    OSRestoreInterrupts(old);
    movie_work.initialized = TRUE;
    return TRUE;
}

/* 0x8043D8DC (0x34): marks the player shut down. */
void THPSimpleQuit(void) {
    BOOL old;

    old = OSDisableInterrupts();
    OSRestoreInterrupts(old);
    movie_work.initialized = FALSE;
}

/* 0x8043D910 (0x258): opens `fileName` and reads its header and component descriptions. */
BOOL THPSimpleOpen(char* fileName) {
    s32 offset;
    u32 i;

    if (!movie_work.initialized) {
        return FALSE;
    }
    if (movie_work.control.open) {
        return FALSE;
    }
    memset(&movie_work.control.videoInfo, 0, sizeof(THPVideoInfo));
    if (!dvd_open_wait(fileName, &movie_work.control.fileInfo)) {
        return FALSE;
    }
    if (dvd_read_wait(&movie_work.control.fileInfo, movie_work.read_buffer, 64, 0) < 0) {
        DVDClose(&movie_work.control.fileInfo);
        return FALSE;
    }
    memcpy(&movie_work.control.header, movie_work.read_buffer, sizeof(THPHeader));
    if (strcmp(movie_work.control.header.magic, "THP") != 0) {
        DVDClose(&movie_work.control.fileInfo);
        return FALSE;
    }
    if (movie_work.control.header.version != 0x11000) {
        DVDClose(&movie_work.control.fileInfo);
        return FALSE;
    }
    offset = movie_work.control.header.compInfoDataOffsets;
    if (dvd_read_wait(&movie_work.control.fileInfo, movie_work.read_buffer, 32, offset) < 0) {
        DVDClose(&movie_work.control.fileInfo);
        return FALSE;
    }
    memcpy(&movie_work.control.compInfo, movie_work.read_buffer, sizeof(THPFrameCompInfo));
    offset += sizeof(THPFrameCompInfo);
    for (i = 0; i < movie_work.control.compInfo.numComponents; i++) {
        switch (movie_work.control.compInfo.frameComp[i]) {
        case 0:
            if (dvd_read_wait(&movie_work.control.fileInfo, movie_work.read_buffer, 32, offset) < 0) {
                DVDClose(&movie_work.control.fileInfo);
                return FALSE;
            }
            memcpy(&movie_work.control.videoInfo, movie_work.read_buffer, sizeof(THPVideoInfo));
            offset += sizeof(THPVideoInfo);
            break;
        case 1:
            if (dvd_read_wait(&movie_work.control.fileInfo, movie_work.read_buffer, 32, offset) < 0) {
                DVDClose(&movie_work.control.fileInfo);
                return FALSE;
            }
            offset += 16;
            break;
        default:
            return FALSE;
        }
    }
    movie_work.control.curOffset = movie_work.control.header.movieDataOffsets;
    movie_work.control.readSize = movie_work.control.header.firstFrameSize;
    movie_work.control.readIndex = 0;
    movie_work.control.readFrame = 0;
    movie_work.control.readError = FALSE;
    movie_work.control.textureSet.frameNumber = -1;
    movie_work.control.nextDecodeIndex = 0;
    movie_work.control.preFetchState = 0;
    movie_work.control.loop = 0;
    movie_work.control.open = TRUE;
    return TRUE;
}

/* 0x8043DB68 (0x64): closes the file unless a read is running. */
BOOL THPSimpleClose(void) {
    if (movie_work.control.open && !movie_work.control.preFetchState && !movie_work.control.readBusy) {
        movie_work.control.open = FALSE;
        DVDClose(&movie_work.control.fileInfo);
        return TRUE;
    }
    return FALSE;
}

/* 0x8043DBCC (0x60): the frame memory the open movie needs. */
u32 THPSimpleCalcNeedMemory(void) {
    u32 size;

    if (movie_work.control.open) {
        size = OSRoundUp32B(movie_work.control.header.bufSize) * THP_READ_BUFFER_NUM;
        size += OSRoundUp32B(movie_work.control.videoInfo.ySize * movie_work.control.videoInfo.xSize);
        size += OSRoundUp32B(movie_work.control.videoInfo.xSize * movie_work.control.videoInfo.ySize / 4);
        size += OSRoundUp32B(movie_work.control.videoInfo.xSize * movie_work.control.videoInfo.ySize / 4);
        size += 0x1000;
        return size;
    }
    return 0;
}

/* 0x8043DC2C (0x1B4): carves `buffer` into the three planes, the read buffers and the decoder's work area. */
BOOL THPSimpleSetBuffer(u8* buffer) {
    u32 i;
    u8* ptr;
    u32 ysize;
    u32 uvsize;

    if (movie_work.control.open && !movie_work.control.preFetchState) {
        ysize = OSRoundUp32B(movie_work.control.videoInfo.ySize * movie_work.control.videoInfo.xSize);
        uvsize = OSRoundUp32B(movie_work.control.videoInfo.xSize * movie_work.control.videoInfo.ySize / 4);
        movie_work.control.textureSet.ytexture = buffer;
        DCInvalidateRange(buffer, ysize);
        ptr = buffer + ysize;
        movie_work.control.textureSet.utexture = ptr;
        DCInvalidateRange(ptr, uvsize);
        ptr += uvsize;
        movie_work.control.textureSet.vtexture = ptr;
        DCInvalidateRange(ptr, uvsize);
        ptr += uvsize;
        for (i = 0; i < THP_READ_BUFFER_NUM; i++) {
            movie_work.control.readBuffer[i].ptr = ptr;
            ptr += OSRoundUp32B(movie_work.control.header.bufSize);
            movie_work.control.readBuffer[i].isValid = FALSE;
        }
        movie_work.control.thpWork = ptr;
    }
    return TRUE;
}

static void ReadCallback(s32 result, DVDFileInfo* fileInfo);

/* 0x8043DDE0 (0xDC): queues the read of the next frame (wrapping when the movie loops). */
static void ReadFrameAsync(void) {
    if (!movie_work.control.readError && movie_work.control.preFetchState == 1) {
        if (movie_work.control.readFrame > movie_work.control.header.numFrames - 1) {
            if (movie_work.control.loop == 1) {
                movie_work.control.readFrame = 0;
                movie_work.control.curOffset = movie_work.control.header.movieDataOffsets;
                movie_work.control.readSize = movie_work.control.header.firstFrameSize;
            } else {
                return;
            }
        }
        movie_work.control.readBusy = TRUE;
        if (DVDReadAsyncPrio(&movie_work.control.fileInfo,
                             movie_work.control.readBuffer[movie_work.control.readIndex].ptr,
                             movie_work.control.readSize, movie_work.control.curOffset, ReadCallback, 2) != TRUE) {
            movie_work.control.readBusy = FALSE;
            movie_work.control.readError = TRUE;
        }
    }
}

/* 0x8043DEBC (0xCC): marks the read frame valid and queues the next one. */
static void ReadCallback(s32 result, DVDFileInfo* fileInfo) {
    if (result == -1) {
        movie_work.control.readError = TRUE;
        return;
    }
    if (result == -3) {
        return;
    }
    movie_work.control.readBusy = FALSE;
    movie_work.control.readBuffer[movie_work.control.readIndex].frameNumber = movie_work.control.readFrame;
    movie_work.control.readFrame++;
    movie_work.control.readBuffer[movie_work.control.readIndex].isValid = TRUE;
    movie_work.control.curOffset += movie_work.control.readSize;
    movie_work.control.readSize = *(s32*)movie_work.control.readBuffer[movie_work.control.readIndex].ptr;
    movie_work.control.readIndex = THP_NEXT_INDEX(movie_work.control.readIndex);
    if (!movie_work.control.readBuffer[movie_work.control.readIndex].isValid) {
        ReadFrameAsync();
    }
}

/* 0x8043DF88 (0x18C): reads the first frames synchronously and starts prefetching. */
BOOL THPSimplePreLoad(s32 loop) {
    u32 i;
    u32 readNum;

    if (movie_work.control.open && !movie_work.control.preFetchState) {
        readNum = THP_READ_BUFFER_NUM;
        if (!loop && movie_work.control.header.numFrames < THP_READ_BUFFER_NUM) {
            readNum = movie_work.control.header.numFrames;
        }
        for (i = 0; i < readNum; i++) {
            if (dvd_read_wait(&movie_work.control.fileInfo,
                              movie_work.control.readBuffer[movie_work.control.readIndex].ptr,
                              movie_work.control.readSize, movie_work.control.curOffset) < 0) {
                movie_work.control.readError = TRUE;
                return FALSE;
            }
            movie_work.control.curOffset += movie_work.control.readSize;
            movie_work.control.readSize = *(s32*)movie_work.control.readBuffer[movie_work.control.readIndex].ptr;
            movie_work.control.readBuffer[movie_work.control.readIndex].isValid = TRUE;
            movie_work.control.readBuffer[movie_work.control.readIndex].frameNumber = movie_work.control.readFrame;
            movie_work.control.readIndex = THP_NEXT_INDEX(movie_work.control.readIndex);
            movie_work.control.readFrame++;
            if (movie_work.control.readFrame > movie_work.control.header.numFrames - 1 &&
                movie_work.control.loop == 1) {
                movie_work.control.readFrame = 0;
                movie_work.control.curOffset = movie_work.control.header.movieDataOffsets;
                movie_work.control.readSize = movie_work.control.header.firstFrameSize;
            }
        }
        movie_work.control.loop = loop;
        movie_work.control.preFetchState = 1;
        return TRUE;
    }
    return FALSE;
}

/* 0x8043E114 (0xC8): stops prefetching and rewinds the player. */
BOOL THPSimpleLoadStop(void) {
    s32 i;

    if (movie_work.control.open) {
        movie_work.control.preFetchState = 0;
        if (movie_work.control.readBusy) {
            DVDCancel(&movie_work.control.fileInfo.cb);
            movie_work.control.readBusy = FALSE;
        }
        for (i = 0; i < THP_READ_BUFFER_NUM; i++) {
            movie_work.control.readBuffer[i].isValid = FALSE;
        }
        movie_work.control.textureSet.frameNumber = -1;
        movie_work.control.curOffset = movie_work.control.header.movieDataOffsets;
        movie_work.control.readSize = movie_work.control.header.firstFrameSize;
        movie_work.control.readIndex = 0;
        movie_work.control.readFrame = 0;
        movie_work.control.readError = FALSE;
        movie_work.control.nextDecodeIndex = 0;
        return TRUE;
    }
    return FALSE;
}

static void CheckPrefetch(void);
static BOOL VideoDecode(u8* videoFrame);

/* 0x8043E1DC (0xFC): decodes the next prefetched frame: 0 done, 1 decode error, 2 nothing read yet. */
s32 THPSimpleDecode(void) {
    u32 i;
    u8* ptr;
    u32* compSizePtr;
    THPReadBuffer* readBuffer;

    readBuffer = &movie_work.control.readBuffer[movie_work.control.nextDecodeIndex];
    if (readBuffer->isValid) {
        compSizePtr = ((THPFrameHeader*)readBuffer->ptr)->compSize;
        ptr = readBuffer->ptr + movie_work.control.compInfo.numComponents * 4 + 8;
        for (i = 0; i < movie_work.control.compInfo.numComponents; i++) {
            switch (movie_work.control.compInfo.frameComp[i]) {
            case 0:
                if (!VideoDecode(ptr)) {
                    return 1;
                }
                break;
            }
            ptr += *compSizePtr;
            compSizePtr++;
        }
        movie_work.control.readBuffer[movie_work.control.nextDecodeIndex].isValid = FALSE;
        movie_work.control.nextDecodeIndex = THP_NEXT_INDEX(movie_work.control.nextDecodeIndex);
        CheckPrefetch();
        return 0;
    }
    return 2;
}

/* 0x8043E2D8 (0x64): restarts prefetching when the next read buffer is free. */
static void CheckPrefetch(void) {
    BOOL old;

    old = OSDisableInterrupts();
    if (!movie_work.control.readBuffer[movie_work.control.readIndex].isValid && !movie_work.control.readBusy) {
        ReadFrameAsync();
    }
    OSRestoreInterrupts(old);
}

/* 0x8043E33C (0x68): decodes `videoFrame` into the planes. */
static BOOL VideoDecode(u8* videoFrame) {
    MovieWork* work;

    work = &movie_work;
    if (THPVideoDecode(videoFrame, work->control.textureSet.ytexture, work->control.textureSet.utexture,
                       work->control.textureSet.vtexture, work->control.thpWork) == 0) {
        work->control.textureSet.frameNumber = work->control.readBuffer[work->control.nextDecodeIndex].frameNumber;
        return TRUE;
    }
    return FALSE;
}

void THPGXYuv2RgbSetup(GXRenderModeObj* rmode);
void THPGXYuv2RgbDraw(u8* yTexture, u8* uTexture, u8* vTexture, s16 x, s16 y, s16 textureWidth, s16 textureHeight,
                      s16 polygonWidth, s16 polygonHeight);
void THPGXRestore(void);

/* 0x8043E3A4 (0x98): draws the decoded frame; returns its number, or -1 before the first. */
s32 THPSimpleDrawCurrentFrame(GXRenderModeObj* rmode, u32 x, u32 y, u32 polygonW, u32 polygonH) {
    MovieWork* work;

    work = &movie_work;
    if (work->control.textureSet.frameNumber >= 0) {
        THPGXYuv2RgbSetup(rmode);
        THPGXYuv2RgbDraw(work->control.textureSet.ytexture, work->control.textureSet.utexture,
                         work->control.textureSet.vtexture, x, y, work->control.videoInfo.xSize,
                         work->control.videoInfo.ySize, polygonW, polygonH);
        THPGXRestore();
        return work->control.textureSet.frameNumber;
    }
    return -1;
}

/* 0x8043E43C (0x48): copies the open movie's video description to `videoInfo`. */
BOOL THPSimpleGetVideoInfo(THPVideoInfo* videoInfo) {
    if (movie_work.control.open) {
        memcpy(videoInfo, &movie_work.control.videoInfo, sizeof(THPVideoInfo));
        return TRUE;
    }
    return FALSE;
}

/* 0x8043E484 (0x24): the open movie's frame count. */
s32 THPSimpleGetTotalFrame(void) {
    if (movie_work.control.open) {
        return movie_work.control.header.numFrames;
    }
    return 0;
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* THPDraw. */

static void thp_gx_set_tex_coord_gen(s32 dst_coord, s32 func, s32 src_param, u32 mtx);
static void thp_gx_end(void);
static void thp_gx_tex_coord2u16(u16 s, u16 t);
static void thp_gx_position3s16(s16 x, s16 y, s16 z);

/* 0x8043E4A8 (0x118): restores the GX state the YUV draw changed. */
void THPGXRestore(void) {
    GXSetZMode(1, 7, 0);
    GXSetBlendMode(0, 1, 0, 15);
    GXSetNumTexGens(1);
    GXSetNumChans(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(0, 0, 0, 255);
    GXSetTevOp(0, 3);
    GXSetTevSwapMode(0, 0, 0);
    GXSetTevSwapMode(1, 0, 0);
    GXSetTevSwapMode(2, 0, 0);
    GXSetTevSwapMode(3, 0, 0);
    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
    GXSetTevSwapModeTable(1, 0, 0, 0, 3);
    GXSetTevSwapModeTable(2, 1, 1, 1, 3);
    GXSetTevSwapModeTable(3, 2, 2, 2, 3);
}

/* 0x8043E5C0 (0x4A8): sets up the orthographic view and the four TEV stages that turn Y/U/V planes into RGB. */
void THPGXYuv2RgbSetup(GXRenderModeObj* rmode) {
    s32 scrWidth;
    s32 scrHeight;
    Mtx44 pMtx;
    Mtx mMtx;

    scrWidth = rmode->fbWidth;
    scrHeight = rmode->efbHeight;
    GXSetPixelFmt(0, 0);
    C_MTXOrtho(pMtx, 0.0f, (f32)scrHeight, 0.0f, (f32)scrWidth, 0.0f, -1.0f);
    GXSetProjection(pMtx, 1);
    GXSetViewport(0.0f, 0.0f, (f32)scrWidth, (f32)scrHeight, 0.0f, 1.0f);
    GXSetScissor(0, 0, (u32)scrWidth, (u32)scrHeight);
    PSMTXIdentity(mMtx);
    GXLoadPosMtxImm(mMtx, 0);
    GXSetCurrentMtx(0);
    GXSetZMode(1, 7, 0);
    GXSetBlendMode(0, 1, 0, 0);
    GXSetColorUpdate(1);
    GXSetAlphaUpdate(0);
    GXSetDispCopyGamma(0);
    GXSetNumChans(0);
    GXSetNumTexGens(2);
    thp_gx_set_tex_coord_gen(0, 1, 4, 60);
    thp_gx_set_tex_coord_gen(1, 1, 4, 60);
    GXInvalidateTexAll();
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(13, 1);
    GXSetVtxAttrFmt(7, 9, 1, 3, 0);
    GXSetVtxAttrFmt(7, 13, 1, 2, 0);
    GXSetNumTevStages(4);
    GXSetTevOrder(0, 1, 1, 255);
    GXSetTevColorIn(0, 15, 8, 14, 2);
    GXSetTevColorOp(0, 0, 0, 0, 0, 0);
    GXSetTevAlphaIn(0, 7, 4, 6, 1);
    GXSetTevAlphaOp(0, 1, 0, 0, 0, 0);
    GXSetTevKColorSel(0, 12);
    GXSetTevKAlphaSel(0, 28);
    GXSetTevSwapMode(0, 0, 0);
    GXSetTevOrder(1, 1, 2, 255);
    GXSetTevColorIn(1, 15, 8, 14, 0);
    GXSetTevColorOp(1, 0, 0, 1, 0, 0);
    GXSetTevAlphaIn(1, 7, 4, 6, 0);
    GXSetTevAlphaOp(1, 1, 0, 0, 0, 0);
    GXSetTevKColorSel(1, 13);
    GXSetTevKAlphaSel(1, 29);
    GXSetTevSwapMode(1, 0, 0);
    GXSetTevOrder(2, 0, 0, 255);
    GXSetTevColorIn(2, 15, 8, 12, 0);
    GXSetTevColorOp(2, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(2, 4, 7, 7, 0);
    GXSetTevAlphaOp(2, 0, 0, 0, 1, 0);
    GXSetTevSwapMode(2, 0, 0);
    GXSetTevOrder(3, 255, 255, 255);
    GXSetTevColorIn(3, 1, 0, 14, 15);
    GXSetTevColorOp(3, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(3, 7, 7, 7, 7);
    GXSetTevAlphaOp(3, 0, 0, 0, 1, 0);
    GXSetTevSwapMode(3, 0, 0);
    GXSetTevKColorSel(3, 14);
    GXSetTevColorS10(1, (GXColorS10){ -90, 0, -114, 135 });
    GXSetTevKColor(0, (GXColor){ 0, 0, 226, 88 });
    GXSetTevKColor(1, (GXColor){ 179, 0, 0, 182 });
    GXSetTevKColor(2, (GXColor){ 255, 0, 255, 128 });
    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
}

/* 0x8043EA68 (0xC): the SDK's `GXSetTexCoordGen` (normalisation off, post-transform identity). */
static void thp_gx_set_tex_coord_gen(s32 dst_coord, s32 func, s32 src_param, u32 mtx) {
    GXSetTexCoordGen2(dst_coord, func, src_param, mtx, 0, 125);
}

/* 0x8043EA74 (0x228): draws the three planes as one quad, trimmed by the letterbox rows. */
void THPGXYuv2RgbDraw(u8* yTexture, u8* uTexture, u8* vTexture, s16 x, s16 y, s16 textureWidth, s16 textureHeight,
                      s16 polygonWidth, s16 polygonHeight) {
    GXTexObj tobj0;
    GXTexObj tobj1;
    GXTexObj tobj2;

    GXInitTexObj(&tobj0, yTexture, (u16)textureWidth, (u16)textureHeight, 1, 0, 0, 0);
    GXInitTexObjLOD(&tobj0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0);
    GXLoadTexObj(&tobj0, 0);
    GXInitTexObj(&tobj1, uTexture, (u16)(textureWidth >> 1), (u16)(textureHeight >> 1), 1, 0, 0, 0);
    GXInitTexObjLOD(&tobj1, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0);
    GXLoadTexObj(&tobj1, 1);
    GXInitTexObj(&tobj2, vTexture, (u16)(textureWidth >> 1), (u16)(textureHeight >> 1), 1, 0, 0, 0);
    GXInitTexObjLOD(&tobj2, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0);
    GXLoadTexObj(&tobj2, 2);
    GXBegin(0x80, 7, 4);
    thp_gx_position3s16(x, (s16)(y + movie_work.letterbox), 0);
    thp_gx_tex_coord2u16(0, 0);
    thp_gx_position3s16((s16)(x + polygonWidth), (s16)(y + movie_work.letterbox), 0);
    thp_gx_tex_coord2u16(1, 0);
    thp_gx_position3s16((s16)(x + polygonWidth), (s16)(y + polygonHeight - movie_work.letterbox), 0);
    thp_gx_tex_coord2u16(1, 1);
    thp_gx_position3s16(x, (s16)(y + polygonHeight - movie_work.letterbox), 0);
    thp_gx_tex_coord2u16(0, 1);
    thp_gx_end();
}

/* 0x8043EC9C (0x4): the SDK's `GXEnd` (writes nothing). */
static void thp_gx_end(void) {
}

/* 0x8043ECA0 (0x18): the SDK's `GXTexCoord2u16` (two halfwords to the FIFO). */
static void thp_gx_tex_coord2u16(u16 s, u16 t) {
    *(volatile u16*)0xCC008000 = s;
    *(volatile u16*)0xCC008000 = t;
}

/* 0x8043ECB8 (0x20): the SDK's `GXPosition3s16` (three halfwords to the FIFO). */
static void thp_gx_position3s16(s16 x, s16 y, s16 z) {
    *(volatile s16*)0xCC008000 = x;
    *(volatile s16*)0xCC008000 = y;
    *(volatile s16*)0xCC008000 = z;
}

}

/* ---------------------------------------------------------------------------------------------------------------- */
/* A friend entry's record. */

/* 0x8043ECD8 (0x48): stores the table and clears the record. */
NetworkFriendInfo::NetworkFriendInfo() {
    memset(data_04, 0, sizeof(data_04));
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* The disc-error screen. */

/* 0x8043ED20 (0x14): clears the disc-error state. */
void dvd_error_init(void) {
    memset(&dvd_error, 0, sizeof(DvdError));
}

/* 0x8043ED34 (0x230): watches the drive status; shows the error message and pauses the sound while it is bad. */
void dvd_error_step(void) {
    u32 ready;
    DvdError* err;

    if (system_w.field_0x873 != 0 || system_w.field_0x865 != 0) {
        return;
    }
    err = &dvd_error;
    switch (err->step) {
    case 0:
        err->active = 0;
        err->message = -1;
        switch (dvd_drive_status) {
        case 4:
            err->message = 0;
            break;
        case 6:
            err->message = 1;
            break;
        case 11:
            err->message = 2;
            break;
        case -1:
            err->message = 3;
            system_w.system_halt_0x871 = 1;
            break;
        }
        if (err->message != -1) {
            err->saved_hbm_disabled = system_w.hbm_disabled != 0;
            err->saved_dimming = VIEnableDimming(1) != 0;
            system_w.hbm_disabled = 1;
            err->saved_pad_lock = system_w.field_0x05;
            system_w.field_0x05 = 1;
            err->active = 1;
            snd_master_volume_set(0);
            snd_stream_reloc_pause();
            snd_track_pause();
            forceNetLinkError();
            kbd_close_call();
            err->step++;
        }
        break;
    case 1:
        if (err->message == 3) {
            break;
        }
        switch (dvd_drive_status) {
        case 4:
            err->message = 0;
            break;
        case 6:
            err->message = 1;
            break;
        case 11:
            err->message = 2;
            break;
        case -1:
            err->message = 3;
            system_w.system_halt_0x871 = 1;
            return;
        }
        ready = FALSE;
        if (dvd_drive_status == 0) {
            ready = TRUE;
        }
        if (ready == TRUE) {
            VIEnableDimming(err->saved_dimming);
            system_w.hbm_disabled = err->saved_hbm_disabled;
            snd_master_volume_set(0x8000);
            snd_stream_reloc_resume();
            snd_track_resume();
            system_w.field_0x05 = err->saved_pad_lock;
            quest_clock_reset();
            err->active = 0;
            err->step = 0;
        } else {
            snd_track_pause();
        }
        break;
    }
}

/* 0x8043EF64 (0x38): draws the disc-error message while the screen is up. */
void dvd_error_draw(void) {
    if (dvd_error.active != 0) {
        dvd_error_message_draw();
        flfntFlush();
    }
}

/* 0x8043EF9C (0xEC): dims the screen and prints the current disc-error message. */
static void dvd_error_message_draw(void) {
    _MH_VEC2 size;
    DrawRect rect;
    char message[512];
    s16 size_x;
    s16 size_y;

    flfnt_frame_reset();
    dbg_drawgraph_init();
    get_ScreenSize(&size);
    draw_rect_set(&rect, 0, 0, size.x, size.y, 0xE0);
    draw_rect_fill(&rect);
    wii_sysmsg_gen(dvd_error.message, message, 0);
    size_x = flfntGetSizeX();
    size_y = flfntGetSizeY();
    font_set_size(28, 28);
    font_print_ex((u32)ck_WideMode() == 1 ? 166 : 59, 200, 0, (s8*)"%s", message);
    flfntSetSize(size_x, size_y);
}

/* 0x8043F088 (0x1E4): waits while the drive reports an error (`info` the file a read was queued on, or NULL),
 * showing the message; shuts down or restarts when the console asks for it. */
void movie_drive_wait(DVDFileInfo* info) {
    s32 status;
    DvdError* err;

    err = &dvd_error;
    err->saved_hbm_disabled = system_w.hbm_disabled != 0;
    system_w.hbm_disabled = 1;
    do {
        if (WPADGetStatus() == 3) {
            WPADDisconnect(2);
            WPADDisconnect(3);
        }
        if (info != NULL) {
            status = DVDGetCommandBlockStatus(&info->cb);
        } else {
            status = dvd_drive_status;
        }
        err->message = -1;
        switch (status) {
        case -1:
            err->message = 3;
            system_w.system_halt_0x871 = 1;
            break;
        case 4:
            err->message = 0;
            break;
        case 6:
            err->message = 1;
            break;
        case 11:
            err->message = 2;
            break;
        }
        if (err->message != -1) {
            snd_master_volume_set(0);
            snd_stream_reloc_pause();
            snd_track_pause();
            err->saved_dimming = VIEnableDimming(1) != 0;
            forceNetLinkError();
            kbd_close_call();
            dvd_error_message_draw();
            flfntFlush();
            frame_copy_display();
            frame_swap_wait();
            flfntStackReset();
        }
        if (status != -1 && (shutdown_request == 1 || reset_request == 1)) {
            if (WPADGetStatus() == 3) {
                pad_motor_stop_all();
            }
            sound_frame_entry();
            bgm_stop_all();
            system_shutdown_prepare();
            if (shutdown_request != 0) {
                OSShutdownSystem();
            }
            if (reset_request != 0) {
                OSRestart(0);
            }
        }
    } while (status != 0);
    snd_master_volume_set(0x8000);
    snd_stream_reloc_resume();
    snd_track_resume();
    system_w.hbm_disabled = err->saved_hbm_disabled;
    if (err->message != -1) {
        VIEnableDimming(err->saved_dimming);
        quest_clock_reset();
    }
}

/* 0x8043F26C (0x14): asks the disc-error code for a disc change. */
void movie_change_request(void) {
    dvd_error.change_request = 1;
}

/* 0x8043F280 (0x10): 1 while the disc-error screen is up. */
BOOL isMoviePlaying(void) {
    return dvd_error.active;
}

/* 0x8043F290 (0x6C): blanks the video and stops the audio and controller callbacks before a shutdown or restart. */
void system_shutdown_prepare(void) {
    VISetBlack(TRUE);
    VIFlush();
    VIWaitForRetrace();
    pmic_stop();
    AIRegisterDMACallback(NULL);
    SYNQuit();
    SEQQuit();
    MIXQuit();
    GXSetDrawSyncCallback(NULL);
    GXSetDrawDoneCallback(NULL);
    WPADSetSyncDeviceCallback(NULL);
    WPADSetClearDeviceCallback(NULL);
    VISetPostRetraceCallback(NULL);
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* The friend picker (`menu_friendlist.cpp`). */

#pragma optimization_level 4
/* The picker menu page 18 shows. */
FriendPickWork page18_pick_work;

/* 0x8043F2FC (0x18): opens menu page 18 on the friend picker. */
void movie_page18_open(MenuSlot* slot) {
    slot->field_0x014 = 18;
    friend_pick_init(&page18_pick_work, 0);
}

/* 0x8043F314 (0x78): clears `work`, stores `mode` and points it at the lobby's friend list. */
void friend_pick_init(FriendPickWork* work, u8 mode) {
    memset(work, 0, sizeof(FriendPickWork));
    work->mode_0x003 = mode;
    work->list_0x014 = &lobby_state_block.friends_0x16D0;
    work->step_0x000 = 0;
    work->unused_0x001 = 0;
    menu_scroll_init(&work->scroll_0x008, 0, 50, 8);
    work->moved_0x006 = 0;
}

/* 0x8043F38C (0xC): steps menu page 18's picker. */
s32 movie_page18_step(MenuSlot* slot) {
    return friend_pick_step(&page18_pick_work);
}

/* 0x8043F398 (0x6CC): fetches the page's cards one by one, lets the player pick a friend, shows its card and sends
 * a friend request; 1 picked (mode 1), 2 back, 3 nothing to fetch or the request failed, 4 closed. */
s32 friend_pick_step(FriendPickWork* work) {
    s32 ret;
    MenuScroll* scroll;
    FriendCardView* view;
    PlayerPad* pad;
    LobbyFriendList* list;
    NetPeerCard* card;
    u8 index;
    u8 col;
    s32 count;
    u8 fetched;
    s32 row;
    const char* id;
    char* args;

    ret = 0;
    scroll = &work->scroll_0x008;
    view = &friend_card_view;
    pad = Psw;
    if (chk_pointer() == 0) {
        work->input_0x004 = pad->button_0x2C0.pressed_0x04 | (pad->button_0x2C0.held_0x14 & 0xF);
    } else {
        work->input_0x004 = 0;
    }
    work->moved_0x006 = 0;
    switch (work->step_0x000) {
    case 0:
        menu_slot0_lock_set(1);
        work->page_top_0x10AC = scroll->rows_per_page * scroll->col;
        work->fetched_0x10AD = 0;
        work->step_0x000 = 1;
    case 1:
        list = work->list_0x014;
        fetched = work->fetched_0x10AD;
        index = work->page_top_0x10AC + fetched;
        if (index >= list->count_0x000) {
            menu_slot0_lock_set(0);
            work->step_0x000 = 3;
            break;
        }
        id = list->rows_0x004[index].id_0x14;
        if (*id != 0) {
            card = &work->cards_0x01C[fetched];
            memset(card, 0, sizeof(NetPeerCard));
            sprintf(card->id_0x014, "%s", id);
            requestPeerProfileById(id, &work->result_0x10AE, 1, index);
            work->step_0x000 = 2;
        } else {
            ret = 3;
        }
        break;
    case 2:
        if (work->result_0x10AE == 0) {
            break;
        }
        list = work->list_0x014;
        index = work->page_top_0x10AC + work->fetched_0x10AD;
        if (work->result_0x10AE == 1) {
            row = index;
            list->rows_0x004[row].state_0x1E = 1;
        } else {
            row = index;
            list->rows_0x004[row].state_0x1E = 0;
        }
        card = &work->cards_0x01C[work->fetched_0x10AD];
        if (strcmp(card->id_0x014, list->rows_0x004[row].id_0x14) == 0) {
            sprintf(card->name_0x000, "%s", list->rows_0x004[row].name_0x00);
            if (work->result_0x10AE == 1) {
                copyPeerProfileCard(card);
            }
            if (++work->fetched_0x10AD >= scroll->rows_per_page) {
                menu_slot0_lock_set(0);
                work->step_0x000 = 3;
            } else {
                work->step_0x000 = 1;
            }
        } else {
            work->step_0x000 = 0;
        }
        break;
    case 3:
        list = work->list_0x014;
        col = scroll->col;
        count = list->count_0x000;
        if (count < col * 8) {
            fetched = 0;
        } else if (count - (col + 1) * 8 < 0) {
            fetched = count % 8;
        } else {
            fetched = 8;
        }
        if (work->fetched_0x10AD != fetched) {
            work->step_0x000 = 0;
            break;
        }
        if (work->input_0x004 & 0x10) {
            index = scroll->row + scroll->rows_per_page * col;
            if (index < count) {
                sysSE_req(5);
                work->picked_0x018 = &list->rows_0x004[index];
                if (work->mode_0x003 == 1) {
                    ret = 1;
                } else {
                    requestPeerProfileById(list->rows_0x004[index].id_0x14, &work->result_0x10AE, 2, index);
                    work->step_0x000 = 4;
                }
            } else {
                sysSE_req(2);
            }
        } else if (work->input_0x004 & 0x20) {
            ret = 2;
            sysSE_req(1);
        } else {
            menu_scroll_step(scroll, work->input_0x004, 6);
            work->moved_0x006 = scroll->move_flags;
            if (col != scroll->col) {
                work->step_0x000 = 0;
            }
        }
        break;
    case 4:
        if (work->result_0x10AE == 0) {
            break;
        }
        card = &work->cards_0x01C[scroll->row];
        memset(card, 0, sizeof(NetPeerCard));
        sprintf(card->name_0x000, "%s", work->picked_0x018->name_0x00);
        sprintf(card->id_0x014, "%s", work->picked_0x018->id_0x14);
        if (work->result_0x10AE != 1) {
            work->picked_0x018->state_0x1E = 0;
        } else {
            work->picked_0x018->state_0x1E = 1;
            copyPeerProfileCard(card);
        }
        menu_friend_list_init(view, card, 0, work->picked_0x018->state_0x1E);
        work->step_0x000 = 5;
        break;
    case 5:
        switch (menu_friend_list_step(view)) {
        case 4:
        case 5:
            ret = 4;
            break;
        case 2:
            strcpy(work->id_0x10B2, work->cards_0x01C[scroll->row].id_0x014);
            strcpy(work->name_0x10BC, work->cards_0x01C[scroll->row].name_0x000);
            menu_slot0_lock_set(1);
            work->step_0x000 = 6;
            work->request_step_0x002 = 0;
            break;
        case 1:
            work->step_0x000 = 0;
            break;
        case 3:
            work->step_0x000 = 3;
            break;
        }
        break;
    case 6:
        lobby_hidden_flag = 1;
        switch (work->request_step_0x002) {
        case 0:
            work->request_step_0x002 = 1;
            if (menu_work.slot[0].field_0x31F != get_option_cfg(27)) {
                requestFriendSync(&work->result_0x10AE);
                break;
            }
            work->request_step_0x002 = 2;
            if (menu_work.slot[0].field_0x31E != 0) {
                resendProfileRecord(&work->result_0x10AE);
                break;
            }
            args = work->id_0x10B2;
            if (NetCtrlWk::requestCommand18((const s32*)&args, &work->result_0x10AE) == 0) {
                ret = 3;
            } else {
                work->request_step_0x002 = 3;
            }
            break;
        case 1:
            if (work->result_0x10AE == 0) {
                break;
            }
            if (work->result_0x10AE != 1) {
                set_option_cfg(27, menu_work.slot[0].field_0x31F);
            }
            work->request_step_0x002 = 2;
            if (menu_work.slot[0].field_0x31E != 0) {
                resendProfileRecord(&work->result_0x10AE);
                break;
            }
            args = work->id_0x10B2;
            if (NetCtrlWk::requestCommand18((const s32*)&args, &work->result_0x10AE) == 0) {
                ret = 3;
            } else {
                work->request_step_0x002 = 3;
            }
            break;
        case 2:
            if (work->result_0x10AE == 0) {
                break;
            }
            menu_work.slot[0].field_0x31E = 0;
            args = work->id_0x10B2;
            if (NetCtrlWk::requestCommand18((const s32*)&args, &work->result_0x10AE) == 0) {
                ret = 3;
            } else {
                work->request_step_0x002 = 3;
            }
            break;
        case 3:
            if (work->result_0x10AE == 0) {
                break;
            }
            if (lobby_net_leave_select(work->result_0x10AE) == 0 && lobby_net_jump_msg_get(work->result_0x10AE) != NULL) {
                work->request_step_0x002 = 4;
                work->timer_0x10B0 = 15;
            } else {
                menu_slot0_lock_set(0);
                menu_item_close();
                ret = 4;
            }
            break;
        case 4:
            if (work->timer_0x10B0 > 0) {
                work->timer_0x10B0--;
            } else if (work->input_0x004 & 0x10) {
                sysSE_req(0);
                menu_slot0_lock_set(0);
                menu_item_close();
                ret = 4;
            }
            break;
        }
        break;
    default:
        nw4r::db::Panic("menu_friendlist.cpp", 516, "NW4R:Failed assertion 0");
        menu_slot0_lock_set(0);
        menu_item_close();
        ret = 4;
        break;
    }
    return ret;
}

/* 0x8043FA64 (0xC): draws menu page 18's picker. */
void movie_page18_draw(MenuSlot* slot) {
    friend_pick_draw(&page18_pick_work);
}

static void friend_pick_list_draw(FriendPickWork* work, u32 filled);

/* 0x8043FA70 (0x9C): draws the picker, the friend's card or the request's message. */
void friend_pick_draw(FriendPickWork* work) {
    FriendCardView* view;

    view = &friend_card_view;
    switch (work->step_0x000) {
    case 0:
        friend_pick_list_draw(work, 0);
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        friend_pick_list_draw(work, 1);
        break;
    case 5:
        menu_friend_list_draw(view);
        break;
    case 6:
        if (work->request_step_0x002 == 4) {
            put_message((s8*)lobby_net_jump_msg_get(work->result_0x10AE), 20, 0xC0C0C0FF);
        }
        break;
    }
}

/* 0x8043FB0C (0x420): draws the picker's page: one row per friend (name, id and online state) once its card is
 * fetched (`filled`), the page arrows and the title window. */
static void friend_pick_list_draw(FriendPickWork* work, u32 filled) {
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    _mh_ivec2_ icon;
    LobbyFriendList* list;
    u16* rows;
    s32 ofs;
    s32 i;
    bool lit;
    bool hot;
    u8 index;
    LobbyTimerRow* row;
    u8 state;

    list = work->list_0x014;
    get_lsp_data(0x28A6, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x12F), &pos);
    get_lsp_data(0x28B6, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xE), &pos);
    get_lsp_data(0x28B8, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x130), &pos);
    lit = chk_pointer() != 1;
    for (i = 0, ofs = 0; i < work->scroll_0x008.row_limit; ofs += 2, i++) {
        if (i == work->scroll_0x008.row) {
            hot = lit == 1;
        } else {
            hot = 0;
        }
        get_lsp_data(0x28B6, &pos);
        rows = (u16*)get_menu_lsp_tbl(0xF);
        put_lsp_anchor_offset(rows[i], &at, pos);
        get_lsp_data(0x28B8, &pos);
        rows = (u16*)get_menu_lsp_tbl(0x131);
        put_lsp_anchor_offset(rows[i], &icon, pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x10), &at);
        index = i + work->scroll_0x008.rows_per_page * work->scroll_0x008.col;
        if (index < list->count_0x000 && filled == 1 && i < work->fetched_0x10AD) {
            row = &list->rows_0x004[index];
            spr_data_copy(&spr, get_lsp_data(0x1DC, NULL));
            spr.color = GetMenuFontColor(1, hot, lit, 0);
            draw_font(spr, (s8*)row->name_0x00, 0, &at);
            spr_data_copy(&spr, get_lsp_data(0x1DD, NULL));
            spr.color = GetMenuFontColor(1, hot, lit, 0);
            draw_font(spr, (s8*)row->id_0x14, 0, &at);
            state = row->state_0x1E;
            switch (state) {
            case 1:
                draw_sprite_idx(0x28C8, &icon);
                break;
            case 0:
                draw_sprite_idx(0x28C9, &icon);
                break;
            }
            draw_font_idx(0x28C6, (s8*)get_menu_msg(0x33, state), 5, &icon);
        } else {
            spr_data_copy(&spr, get_lsp_data(0x1DC, NULL));
            spr.color = GetMenuFontColor(0, hot, lit, 0);
            draw_font(spr, (s8*)get_menu_msg(0x32, 6), 0, &at);
            spr_data_copy(&spr, get_lsp_data(0x1DD, NULL));
            spr.color = GetMenuFontColor(0, hot, lit, 0);
            draw_font(spr, (s8*)get_menu_msg(0x32, 6), 0, &at);
            draw_sprite_idx(0x28C9, &icon);
        }
        if (hot == 1 && lit == 1 && filled == 1) {
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x134), &at);
        }
    }
    get_lsp_data(0x28A5, &pos);
    PutPageArrow((u16*)get_menu_lsp_tbl(1), work->scroll_0x008.col, work->scroll_0x008.col_count, work->moved_0x006,
                 &pos, 0);
    get_lsp_data(0x28D0, &pos);
    draw_window_frame_preset(pos.x, pos.y, 0);
    font_set_size(16, 16);
    switch (work->mode_0x003) {
    case 0:
        font_print_ex(pos.x + 24, pos.y + 16, 0, (s8*)get_menu_msg(0x33, 2));
        break;
    case 1:
        font_print_ex(pos.x + 24, pos.y + 16, 0, (s8*)get_menu_msg(0x38, 9));
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* The hunter card view: a friend's card with its three pages and the command rows (mail, remove). */

/* size: 0x148 (the clear `menu_friend_list_init` runs) - the card view: its step, the command rows, the page scroll,
 * the card shown, the mail the message command opens and whether the card's owner is online. */
struct FriendCardView {
    /* +0x000 */ u8 step;
    /* +0x001 */ u8 unused_0x001;
    /* +0x002 */ u8 row_count;            /* the command rows (3 from the friend list, else 2) */
    /* +0x003 */ u8 page_count;           /* the card pages (3 while the owner is online, else 1) */
    /* +0x004 */ u8 mode;                 /* 0 the friend list, 1-3 the other callers */
    /* +0x005 */ u8 confirm;              /* the confirmation the step shows (0 mail, 6 remove) */
    /* +0x006 */ s8 confirm_cursor;
    /* +0x007 */ u8 confirm_choices;      /* 1 (OK) or 2 (yes/no) */
    /* +0x008 */ u16 input;               /* the pad's pressed word plus the held d-pad bits */
    /* +0x00A */ u16 moved;               /* the scroll's move flags of the frame */
    /* +0x00C */ MenuScroll scroll;       /* the command row (`row`) and the page (`col`) */
    /* +0x018 */ MenuEntry rows[3];
    /* +0x060 */ NetPeerCard* card;
    /* +0x064 */ LobbyMailRow mail;       /* the mail the message command writes to the card's owner */
    /* +0x144 */ u8 online;
    /* +0x145 */ u8 pad_0x145[3];
};

/* The hunter card view the friend picker, the player search and the lobby card menu share. */
FriendCardView friend_card_view;

static void friend_card_rows_build(FriendCardView* view, s8 cursor);
static u8 friend_card_row_enabled(FriendCardView* view, s8 row);
static void friend_card_rows_draw(FriendCardView* view);
static void friend_card_profile_draw(FriendCardView* view, bool lit);
static void friend_card_place_draw(FriendCardView* view);
static void friend_card_equip_draw(FriendCardView* view);
static void friend_card_equip_page_draw(FriendCardView* view, _EQUIP* equip, u8 dye);
static void friend_card_skills_draw(u16 id, u16* skills);
static void friend_card_confirm_draw(FriendCardView* view, u8 choices, u16 kind);

/* 0x8043FF2C (0x110): opens the view on `card` for caller `mode`; `online` adds the card's other pages. */
void menu_friend_list_init(FriendCardView* view, NetPeerCard* card, u8 mode, u8 online) {
    memset(view, 0, sizeof(FriendCardView));
    view->step = 0;
    view->unused_0x001 = 0;
    view->input = 0;
    view->moved = 0;
    view->mode = mode;
    switch (mode) {
    case 1:
    case 2:
    case 3:
        view->row_count = 2;
        break;
    case 0:
        view->row_count = 3;
        break;
    }
    view->online = online;
    view->card = card;
    if (online == 0) {
        view->page_count = 1;
    } else {
        view->page_count = 3;
    }
    menu_scroll_init(&view->scroll, 0, view->page_count * view->row_count, view->row_count);
    if (view->online == 0 && view->mode == 0) {
        view->scroll.row = 2;
    }
    friend_card_rows_build(view, view->scroll.row);
}

/* 0x8044003C (0x14C): fills the three command rows: their message, whether they can be picked and their colour. */
static void friend_card_rows_build(FriendCardView* view, s8 cursor) {
    s8 i;
    MenuEntry* row;
    u32 lit;
    s8 hot;
    s32 dim;

    i = 0;
    row = view->rows;
    do {
        if (i < view->row_count) {
            row->page_kind = i;
            row->field_0x02 = friend_card_row_enabled(view, i);
            row->field_0x08 = get_menu_msg(0x34, i);
            lit = 0;
            if (view->step < 10 && chk_pointer() == 0) {
                lit = 1;
            }
            if (i == cursor) {
                if (lit == 1) {
                    hot = 1;
                    dim = 0;
                } else if (chk_pointer() == 0) {
                    hot = 0;
                    dim = 1;
                } else {
                    hot = 0;
                    dim = 0;
                }
            } else {
                hot = 0;
                dim = 0;
            }
            row->selected = hot;
            row->field_0x0C = GetMenuFontColor(row->field_0x02 != 0, hot != 0, lit != 0, dim != 0);
        } else {
            row->page_kind = -1;
            row->selected = 0;
            row->field_0x02 = 0;
            row->field_0x08 = 0;
            row->field_0x0C = 0;
        }
        row++;
        i++;
    } while (i < 3);
}

/* 0x80440188 (0xE0): 1 when command `row` can be picked (not in the arena while the hunter is busy). */
static u8 friend_card_row_enabled(FriendCardView* view, s8 row) {
    u8 enabled;
    _PLW* plw;

    enabled = 1;
    switch (view->mode) {
    case 0:
        if (view->online == 0 && row != 2) {
            enabled = 0;
        }
        break;
    case 1:
        if (row != 1) {
            enabled = 0;
        }
        break;
    }
    if (row == 0 && GameMode_ck() == 2) {
        plw = (_PLW*)get_move_work_adrs(2);
        if (plw != NULL) {
            plw = &plw[(s8)my_player_no()];
            if (plw->field_0xB01 & 7) {
                enabled = 0;
            }
            if (lb_player_menu_area_ck(plw) == 1) {
                enabled = 0;
            }
        }
    }
    return enabled;
}

/* 0x80440268 (0x428): steps the view: the pages and commands, the mail page, the confirmation and the friend's
 * removal; 0 busy, 1 removed, 2 a request was picked, 3 back, 4 failed or unknown step. */
s32 menu_friend_list_step(FriendCardView* view) {
    s32 ret;
    MenuScroll* scroll;
    PlayerPad* pad;
    LobbyMailRow* mail;
    NetPeerCard* card;

    ret = 0;
    scroll = &view->scroll;
    pad = Psw;
    if (chk_pointer() == 0) {
        view->input = pad->button_0x2C0.pressed_0x04 | (pad->button_0x2C0.held_0x14 & 0xF);
    } else {
        view->input = 0;
    }
    view->moved = 0;
    switch (view->step) {
    case 0:
        friend_card_rows_build(view, scroll->row);
        view->step = 1;
        break;
    case 1:
        if (view->input & 0x10) {
            if (view->rows[scroll->row].field_0x02 == 0) {
                sysSE_req(2);
            } else {
                switch (scroll->row) {
                case 0:
                    sysSE_req(5);
                    view->step = 10;
                    break;
                case 1:
                    sysSE_req(27);
                    view->step = 20;
                    break;
                case 2:
                    sysSE_req(0);
                    view->step = 40;
                    break;
                }
                view->unused_0x001 = 0;
                friend_card_rows_build(view, scroll->row);
            }
        } else if (view->input & 0x20) {
            sysSE_req(1);
            ret = 3;
        } else {
            menu_scroll_step(scroll, view->input, 6);
            view->moved = scroll->move_flags;
            friend_card_rows_build(view, scroll->row);
        }
        break;
    case 10:
        view->confirm = 0;
        view->step = 50;
        break;
    case 20:
        mail = &view->mail;
        card = view->card;
        sprintf(mail->name_0x00.text_0x00, "%s", card->name_0x000);
        sprintf(mail->id_0x14.text_0x00, "%s", card->id_0x014);
        plsearch_work_init(&plsearch_message_work, 1, mail);
        view->step++;
        break;
    case 21:
        switch (plsearch_step(&plsearch_message_work)) {
        case 2:
            view->step = 0;
            break;
        case 4:
            ret = 3;
            break;
        }
        break;
    case 40:
        view->confirm = 6;
        view->step = 50;
        break;
    case 50:
        switch (view->confirm) {
        case 1:
        case 3:
        case 4:
        case 5:
            sysSE_req(2);
            view->confirm_cursor = 0;
            view->confirm_choices = 1;
            break;
        case 0:
        case 2:
        case 6:
            view->confirm_cursor = 1;
            view->confirm_choices = 2;
            break;
        }
        view->step++;
        break;
    case 51:
        if (view->input & 0x10) {
            sysSE_req(0);
            if (view->confirm_cursor == 0) {
                view->step = 52;
            } else {
                view->step = 0;
            }
        } else if (view->input & 0x20) {
            sysSE_req(1);
            view->step = 0;
        } else {
            view->confirm_cursor = menu_cursor_step(view->confirm_cursor, view->confirm_choices, view->input, 4, 8);
        }
        break;
    case 52:
        switch (view->confirm) {
        case 1:
        case 3:
        case 4:
        case 5:
            view->step = 0;
            break;
        case 0:
        case 2:
            ret = 2;
            break;
        case 6:
            switch (lobby_friend_remove((LobbyRecentRow*)view->card)) {
            case 1:
                menu_slot0_lock_set(1);
                view->step++;
                break;
            case 0:
            case 2:
                sysSE_req(2);
                ret = 3;
                break;
            }
            break;
        }
        break;
    case 53:
        if (lobby_state_block.result_0x1695 != 0) {
            if (lobby_state_block.result_0x1695 == 1) {
                lobby_friend_drop((LobbyRecentRow*)view->card);
                sysSE_req(0);
                ret = 1;
            } else {
                sysSE_req(2);
                ret = 4;
            }
            menu_slot0_lock_set(0);
        }
        break;
    default:
        ret = 4;
        break;
    }
    return ret;
}

/* 0x80440690 (0x108): draws the view: the mail page, or the card page, the command rows and the page arrows, then
 * the confirmation window. */
void menu_friend_list_draw(FriendCardView* view) {
    s32 drawn;
    u8 page;
    _mh_ivec2_ pos;

    drawn = 0;
    page = view->scroll.col;
    if (view->step == 21) {
        plsearch_draw(&plsearch_message_work);
        drawn = 1;
    }
    if (drawn == 0) {
        switch (page) {
        case 0:
            friend_card_profile_draw(view, 1);
            break;
        case 1:
            friend_card_place_draw(view);
            break;
        case 2:
            friend_card_equip_draw(view);
            break;
        }
        friend_card_rows_draw(view);
        if (view->page_count > 1) {
            get_lsp_data(0x2920, &pos);
            PutPageArrow((u16*)get_menu_lsp_tbl(1), view->scroll.col, view->page_count, view->moved, &pos, 0);
        }
        font_flush();
    }
    if (view->step == 51) {
        friend_card_confirm_draw(view, view->confirm_choices, view->confirm);
    }
}

/* 0x80440798 (0x188): draws the command rows' window, rows and cursor. */
static void friend_card_rows_draw(FriendCardView* view) {
    u16* anchors;
    u16 window;
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    s32 i;
    MenuEntry* row;

    anchors = NULL;
    window = 0xFFFF;
    switch (view->mode) {
    case 0:
        window = 0x296A;
        anchors = (u16*)get_menu_lsp_tbl(0x13F);
        break;
    case 1:
    case 2:
    case 3:
        window = 0x2A68;
        anchors = (u16*)get_menu_lsp_tbl(0x136);
        break;
    }
    if (anchors == NULL || window == 0xFFFF) {
        return;
    }
    spr_data_copy(&spr, get_lsp_data(window, &pos));
    spr.color = 0x926239FF;
    draw_window_box(pos.x, pos.y, spr.width, spr.height, 0x926239FF, 0);
    for (i = 0, row = view->rows; i < view->row_count; row++, anchors++, i++) {
        put_lsp_anchor_offset(*anchors, &at, pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(2), &at);
        spr_data_copy(&spr, get_lsp_data(0x77, NULL));
        spr.color = row->field_0x0C;
        draw_font(spr, (s8*)row->field_0x08, 1, &at);
        if (row->selected != 0 && chk_pointer() == 0) {
            put_menu_cursor((u16*)get_menu_lsp_tbl(3), 0, &at);
        }
    }
}

/* 0x80440920 (0x45C): draws the card's first page: name and id, then for an online owner the weapon kind, the id
 * number, the rank, the hunter rank and the comment, and the active skills. */
static void friend_card_profile_draw(FriendCardView* view, bool lit) {
    _mh_ivec2_ pos;
    char number[16];
    _SPR_DATA_ spr;
    s8 comment[0xAC];
    NetPeerCard* card;
    u16* anchors;
    s32 color;
    u8 kind;
    u8** names;
    u8 rank;

    card = view->card;
    anchors = NULL;
    color = GetMenuFontColor(1, lit, lit, 0);
    switch (view->mode) {
    case 2:
    case 3:
        anchors = (u16*)get_menu_lsp_tbl(0x135);
        break;
    case 0:
        anchors = (u16*)get_menu_lsp_tbl(0x140);
        break;
    case 1:
        anchors = (u16*)get_menu_lsp_tbl(0x137);
        break;
    }
    get_lsp_data(anchors[1], &pos);
    draw_sprite_idx(0x292C, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x138), &pos);
    get_lsp_data(anchors[2], &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x13B), &pos);
    draw_font_idx(0x2937, (s8*)get_menu_msg(0x35, 0), 4, &pos);
    draw_font_idx(0x2939, (s8*)get_menu_msg(0x35, 1), 4, &pos);
    spr_data_copy(&spr, get_lsp_data(0x2938, NULL));
    spr.color = color;
    draw_font(spr, (s8*)card->name_0x000, 0, &pos);
    spr_data_copy(&spr, get_lsp_data(0x293A, NULL));
    spr.color = color;
    draw_font(spr, (s8*)card->id_0x014, 0, &pos);
    if (view->online == 0) {
        get_lsp_data(anchors[2], &pos);
        draw_sprite_idx(0x2942, &pos);
        draw_font_idx(0x2941, (s8*)get_menu_msg(0x35, 7), 5, &pos);
    } else {
        get_lsp_data(anchors[3], &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x13C), &pos);
        draw_font_idx(0x2944, (s8*)get_menu_msg(0x35, 2), 4, &pos);
        draw_font_idx(0x2946, (s8*)get_menu_msg(0x35, 3), 4, &pos);
        draw_font_idx(0x2948, (s8*)get_menu_msg(0x35, 4), 4, &pos);
        kind = 0;
        if (card->equip_0x022[6][0] != 0) {
            kind = Get_pl_type((_EQUIP*)card->equip_0x022[6], (_EQUIP*)card->equip_0x022[7]);
        }
        names = get_str_tbl(0x2C);
        spr_data_copy(&spr, get_lsp_data(0x2945, NULL));
        spr.color = color;
        draw_font(spr, (s8*)names[kind], 0, &pos);
        sprintf(number, "%d", card->id_0x01E);
        spr_data_copy(&spr, get_lsp_data(0x2947, NULL));
        spr.color = color;
        draw_font(spr, (s8*)number, 0, &pos);
        kind = card->level_0x020;
        if (kind != 0) {
            draw_sprite_uv_idx(0x294A, chara_icon_tex_tbl[kind], &pos);
            spr_data_copy(&spr, get_lsp_data(0x2949, NULL));
            spr.color = color;
            draw_font(spr, (s8*)get_menu_msg(0x30, kind), 0, &pos);
        }
        get_lsp_data(anchors[4], &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x13D), &pos);
        draw_font_idx(0x2955, (s8*)get_menu_msg(0x35, 5), 4, &pos);
        draw_font_idx(0x2957, (s8*)get_menu_msg(0x35, 6), 4, &pos);
        rank = card->rank_0x021;
        names = get_str_tbl(0x56);
        spr_data_copy(&spr, get_lsp_data(0x2956, NULL));
        spr.color = color;
        draw_font(spr, (s8*)names[rank], 0, &pos);
        knj_msg_str_build((s8*)card->comment_0x09E, comment, 28, 3);
        spr_data_copy(&spr, get_lsp_data(0x2958, NULL));
        spr.color = color;
        draw_font(spr, comment, 0, &pos);
    }
    if (anchors[6] != 0xFFFF && view->online != 0) {
        friend_card_skills_draw(anchors[6], card->equip_ids_0x08E);
    }
}

/* 0x80440D7C (0x80): draws the card's second page: the owner's server and settings rows. */
static void friend_card_place_draw(FriendCardView* view) {
    _mh_ivec2_ pos;

    get_lsp_data(0x2921, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x139), &pos);
    placeinfo_server_rows_draw(0x2971, 1, view->card);
    placeinfo_settings_rows_draw(0x2972, 1, view->card);
    get_lsp_data(0x2921, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x138), &pos);
}

/* 0x80440DFC (0xBC): copies the card's nine equipment records and draws its equipment page. */
static void friend_card_equip_draw(FriendCardView* view) {
    _EQUIP equip[9];
    NetPeerCard* card;
    u8 dye;

    card = view->card;
    dye = card->status_0x211;
    equip_record_copy(&equip[0], (_EQUIP*)card->equip_0x022[0]);
    equip_record_copy(&equip[1], (_EQUIP*)card->equip_0x022[1]);
    equip_record_copy(&equip[2], (_EQUIP*)card->equip_0x022[2]);
    equip_record_copy(&equip[3], (_EQUIP*)card->equip_0x022[3]);
    equip_record_copy(&equip[4], (_EQUIP*)card->equip_0x022[4]);
    equip_record_copy(&equip[5], (_EQUIP*)card->equip_0x022[5]);
    equip_record_copy(&equip[6], (_EQUIP*)card->equip_0x022[6]);
    equip_record_copy(&equip[7], (_EQUIP*)card->equip_0x022[7]);
    equip_record_copy(&equip[8], (_EQUIP*)card->equip_0x022[8]);
    friend_card_equip_page_draw(view, equip, dye);
}

/* 0x80440EB8 (0x250): draws the equipment page: the gun parts, the seven pieces and, when some piece cannot be
 * dyed, the dye colour. */
static void friend_card_equip_page_draw(FriendCardView* view, _EQUIP* equip, u8 dye) {
    _mh_ivec2_ at0;
    _mh_ivec2_ at1;
    union {
        s32 word;
        GXColor rgba;
    } color;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    s32 dyeable;
    _SPR_DATA_* frame;
    s32 i;
    u16* anchors;
    _EQUIP* piece;

    dyeable = 1;
    get_lsp_data(0x2981, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x13A), &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x138), &pos);
    info_gun_parts_draw(0x2982, &equip[6], &equip[7], &equip[8]);
    frame = get_lsp_data(0x2983, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x141), &pos);
    i = 0;
    anchors = (u16*)get_menu_lsp_tbl(0x142);
    do {
        piece = NULL;
        switch (i) {
        case 0:
            piece = &equip[6];
            break;
        case 1:
            piece = &equip[4];
            break;
        case 2:
            piece = &equip[0];
            break;
        case 3:
            piece = &equip[1];
            break;
        case 4:
            piece = &equip[2];
            break;
        case 5:
            piece = &equip[3];
            break;
        case 6:
            piece = &equip[5];
            break;
        }
        if (piece != NULL) {
            if (i == 0) {
                if (piece->kind != 0) {
                    at1 = frame->pos;
                    Put_equip_dtl_syo_item1(piece, &equip[7], 0, 0, 0, at1, *anchors);
                }
            } else if (piece->kind != 0) {
                at0 = frame->pos;
                Put_equip_dtl_syo_item1(piece, NULL, 0, 0, 0, at0, *anchors);
            }
            if (i - 1 <= 4U && EnableChangeColor(piece) == 0) {
                dyeable = 0;
            }
        }
        anchors++;
        i++;
    } while (i < 7);
    if (dyeable == 0 && dye != 0) {
        get_lsp_data(0x299A, &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x83), &pos);
        color.word = equip_color_rgba_get(dye);
        spr_data_copy(&spr, get_lsp_data(0x7B9, NULL));
        spr.color = (color.rgba.r << 24) | (color.rgba.g << 16) | (color.rgba.b << 8) | 0xFF;
        draw_sprite(spr, &pos);
    }
}

/* 0x80441108 (0x1F8): draws the active-skill box at layout `id`: one row per skill (red when the tree has no
 * points, grey when the skill is invalid), or the "no skills" text. */
static void friend_card_skills_draw(u16 id, u16* skills) {
    _mh_ivec2_ origin;
    _mh_ivec2_ pos;
    s32 count;
    s32 i;
    s32 ofs;
    u16 skill;
    u32 color;
    u16* ids;

    ids = skills;
    count = 0;
    if (skills[0] != 0) {
        count = 1;
    }
    if (skills[1] != 0) {
        count++;
    }
    if (skills[2] != 0) {
        count++;
    }
    if (skills[3] != 0) {
        count++;
    }
    if (skills[4] != 0) {
        count++;
    }
    if (skills[5] != 0) {
        count++;
    }
    if (skills[6] != 0) {
        count++;
    }
    if (skills[7] != 0) {
        count++;
    }
    get_lsp_data(0x677, &origin);
    get_lsp_data(id, &pos);
    pos.x -= origin.x;
    pos.y -= origin.y;
    if (count != 0) {
        get_lsp_data(id, &origin);
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x80), 7, &origin);
        for (i = 0, ofs = 0; i < 8; ids++, ofs += 2, i++) {
            skill = *ids;
            if (skill != 0) {
                color = 0;
                if (skill & 0x4000) {
                    color = 0xE63737FF;
                } else if (skill & 0x8000) {
                    color = 0x878787FF;
                }
                skill_name_row_draw(((u16*)get_menu_lsp_tbl(0x19A))[i], skill & 0x3FFF, color, &pos);
            } else {
                skill_name_row_draw(((u16*)get_menu_lsp_tbl(0x19A))[i], 0xFFFF, 0, &pos);
            }
        }
    } else {
        get_lsp_data(id, &origin);
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x80), 7, &origin);
        draw_font_idx(0x678, (s8*)*get_str_tbl(0x28), 1, &origin);
    }
}

/* 0x80441300 (0xB8): writes the eight active-skill ids of `plw` to `ids`, flagged 0x4000 when the skill's tree has
 * no points and 0x8000 when the skill is not valid for the hunter. */
void card_equip_ids_build(_PLW* plw, u16* ids) {
    _EQUIP columns[10];
    s32 i;
    u16 flags;

    Set_equip_column_arrangement(plw, NULL, columns);
    for (i = 0; i < 8; ids++, i++) {
        flags = 0;
        if (plw->active_skill_id[i] != 0) {
            flags = (equip_columns_tree_points_get(columns, pl_skill_tree_get(plw->active_skill_id[i])) >> 31) & 0x4000;
            if (pl_skill_id_valid_ck(plw, plw->active_skill_id[i]) == 0) {
                flags = 0x8000;
            }
        }
        *ids = flags | plw->active_skill_id[i];
    }
}

/* 0x804413B8 (0x264): draws the confirmation window for `kind` with `choices` buttons and the cursor. */
static void friend_card_confirm_draw(FriendCardView* view, u8 choices, u16 kind) {
    _mh_ivec2_ at;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    _SPR_DATA_* rect;
    u16* anchors;
    u16 i;

    rect = get_lsp_data(0x299C, &pos);
    draw_layout_window(pos.x, pos.y, rect->width, rect->height);
    get_lsp_data(0x299E, &pos);
    if (kind == 2) {
        spr_data_copy(&spr, get_lsp_data(0x299F, NULL));
        draw_font(spr, (s8*)get_menu_msg(0x36, 0), 1, &pos);
        spr.color = 0xE63737FF;
        spr.pos.y += spr.height;
        draw_font(spr, (s8*)get_menu_msg(0x36, kind), 1, &pos);
    } else {
        draw_font_idx(0x299F, (s8*)get_menu_msg(0x36, kind), 8, &pos);
    }
    switch (choices) {
    case 1:
        rect = get_lsp_data(0x29A2, NULL);
        at.x = pos.x + rect->pos.x;
        at.y = pos.y + rect->pos.y;
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xE6), &at);
        draw_font_idx(0xAB9, (s8*)get_menu_msg(13, 0), 1, &at);
        if (chk_pointer() == 0) {
            put_menu_cursor((u16*)get_menu_lsp_tbl(0xE7), 0, &at);
        }
        break;
    case 2:
        i = 0;
        anchors = (u16*)get_menu_lsp_tbl(0x143);
        do {
            rect = get_lsp_data(*anchors, NULL);
            at.x = pos.x + rect->pos.x;
            at.y = pos.y + rect->pos.y;
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xE6), &at);
            draw_font_idx(0xAB9, (s8*)get_menu_msg(13, i), 1, &at);
            if (i == view->confirm_cursor && chk_pointer() == 0) {
                put_menu_cursor((u16*)get_menu_lsp_tbl(0xE7), 0, &at);
            }
            anchors++;
            i++;
        } while (i < 2);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* The option pages: page 4 (the game options), its split-screen form (one per player) and page 17 (the network
 * options with the terms progress bar). */

/* The option ids page 4's rows set, in row order. */
u8 page04_option_ids[16] = { 20, 4, 17, 18, 13, 14, 15, 3, 8, 9, 2, 11, 6, 10, 0, 1 };
/* The option ids the split-screen page's rows set, in row order. */
u8 page04_split_option_ids[13] = { 20, 13, 14, 15, 3, 8, 9, 2, 11, 6, 10, 0, 1 };

/* Page 17's rows: the option ids of its first three rows, the system message ids of the transfer rows' titles and
 * values, and the highest value of each transfer row. */
u8 page17_option_ids[3] = { 12, 26, 27 };
u8 page17_title_msg[3] = { 0x2D, 0x30, 0x32 };
u8 page17_value_msg[3] = { 0x2E, 0x31, 0x34 };
u8 page17_value_max[3] = { 1, 7, 1 };

static void page04_rows_build(MenuSlot* slot);
static void page04_cursor_step(MenuSlot* slot, u16 input);
static s16 option_value_step(u16 input, u8 option);
static s16 page17_transfer_step(u16 input, u8 row);
static u32 page04_control_row_ok(MenuSlot* slot);
static u32 page04_pointer_row_ok(MenuSlot* slot);
static void option_value_sprite_draw(u16 id, u32 color, _mh_ivec2_* pos);
static void option_cursor_draw(MenuSlot* slot, MenuEntry* row, _mh_ivec2_* pos);
static void page04_split_scroll_init(MenuSlot* slot);
static s16 arena_value_step(u16 input, u8 option, u8 player);
static void page04_split_rows_build(MenuSlot* slot);
static void page04_split_cursor_step(MenuSlot* slot, u16 input);
static void page17_scroll_init(MenuSlot* slot);
static u8 page17_transfer_get(u8 row);
static void page17_transfer_set(u8 row, u8 value);
static void page17_rows_build(MenuSlot* slot);
static void page17_cursor_step(MenuSlot* slot, u16 input);
static void terms_progress_draw(_mh_ivec2_* pos);
static void page04_scroll_init(MenuSlot* slot);

/* 0x8044161C (0x14): opens menu page 4. */
void movie_page04_open(MenuSlot* slot) {
    slot->field_0x014 = 4;
    slot->field_0x001 = 0;
    page04_scroll_init(slot);
}

/* 0x80441630 (0x44): sets page 4's scroll (16 rows, 7 a page) and fills its rows. */
static void page04_scroll_init(MenuSlot* slot) {
    menu_scroll_init(&slot->scroll, 0, 16, 7);
    page04_rows_build(slot);
}

/* 0x80441674 (0x1D4): steps page 4: pick a row, then change its value until confirmed (A) or cancelled (B). */
s32 movie_page04_step(MenuSlot* slot) {
    s32 ret;
    u16 input;

    ret = 0;
    input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    switch (slot->field_0x001) {
    case 0:
        if (input & 0x20) {
            ret = 2;
            sysSE_req(1);
        } else if (input & 0x10) {
            if (slot->entries_b[slot->scroll.row].field_0x02 != 0) {
                menu_slot0_lock_set(1);
                slot->option_value_0x234 = get_cfg(slot->worker->chunk_ofs, page04_option_ids[slot->scroll.index]);
                slot->field_0x001 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else {
            page04_cursor_step(slot, input);
        }
        break;
    case 1:
        slot->option_move_0x236 = 0;
        if (input & 0x20) {
            if (system_w.vs_mode_0x8b0 == 0) {
                set_option_cfg(page04_option_ids[slot->scroll.index], slot->option_value_0x234);
            } else {
                arena_cfg_set(slot->worker->chunk_ofs, page04_option_ids[slot->scroll.index], slot->option_value_0x234);
            }
            page04_rows_build(slot);
            slot->field_0x001 = 0;
            sysSE_req(1);
            menu_slot0_lock_set(0);
        } else if (input & 0x10) {
            slot->field_0x001 = 0;
            sysSE_req(0);
            menu_slot0_lock_set(0);
        } else if (input & 0xC) {
            slot->option_move_0x236 = option_value_step(input, page04_option_ids[slot->scroll.index]);
            page04_rows_build(slot);
        }
        break;
    }
    return ret;
}

/* 0x80441848 (0x58): moves page 4's cursor and refills the rows when it moved. */
static void page04_cursor_step(MenuSlot* slot, u16 input) {
    MenuScroll* scroll;

    scroll = &slot->scroll;
    menu_scroll_step(scroll, input, 5);
    if (scroll->move_flags != 0) {
        page04_rows_build(slot);
    }
}

/* 0x804418A0 (0x16C): raises (right) or lowers (left) option `option` by one; returns the arrow that moved it. */
static s16 option_value_step(u16 input, u8 option) {
    s16 moved;
    u8 value;
    u8 next;

    moved = 0;
    if (system_w.vs_mode_0x8b0 == 0) {
        value = get_option_cfg(option);
    } else {
        value = get_arena_cfg(0, option);
    }
    next = value;
    if (input & 8) {
        next = value + 1;
        moved = 8;
    } else if (input & 4) {
        if (value != 0) {
            next = value - 1;
        }
        moved = 4;
    }
    if (system_w.vs_mode_0x8b0 == 0) {
        if (next != ck_option_cfg(option)) {
            set_option_cfg(option, next);
            if (next == get_option_cfg(option)) {
                sysSE_req(3);
                return moved;
            }
        }
    } else if (next != ck_arena_cfg(0, option)) {
        arena_cfg_set(0, option, next);
        if (next == get_arena_cfg(0, option)) {
            sysSE_req(3);
            return moved;
        }
    }
    return 0;
}

/* 0x80441A0C (0xE0): raises or lowers transfer row `row` by one; returns the arrow that moved it. */
static s16 page17_transfer_step(u16 input, u8 row) {
    s16 moved;
    u8 value;
    u8 next;

    moved = 0;
    value = page17_transfer_get(row);
    next = value;
    if (input & 8) {
        next = value + 1;
        moved = 8;
    } else if (input & 4) {
        if (value != 0) {
            next = value - 1;
        }
        moved = 4;
    }
    if (next != page17_transfer_get(row)) {
        page17_transfer_set(row, next);
        if (next == page17_transfer_get(row)) {
            sysSE_req(3);
            return moved;
        }
    }
    return 0;
}

/* 0x80441AEC (0x78): 1 when the slot's controller can use the controller-type row. */
static u32 page04_control_row_ok(MenuSlot* slot) {
    u32 ok;

    ok = 0;
    switch (get_ControlType(slot->field_0x011)) {
    case 0:
        ok = 1;
        break;
    case 1:
        ok = 1;
        break;
    case 2:
        ok = 0;
        break;
    case 0xFF:
        ok = 1;
        break;
    }
    return ok;
}

/* 0x80441B64 (0x78): 1 when the slot's controller can use the pointer rows. */
static u32 page04_pointer_row_ok(MenuSlot* slot) {
    u32 ok;

    ok = 0;
    switch (get_ControlType(slot->field_0x011)) {
    case 0:
        ok = 1;
        break;
    case 1:
        ok = 1;
        break;
    case 2:
        ok = 0;
        break;
    case 0xFF:
        ok = 1;
        break;
    }
    return ok;
}

/* 0x80441BDC (0x1A4): fills page 4's rows of the page: enabled, colour, and whether the value is at an end. */
static void page04_rows_build(MenuSlot* slot) {
    MenuEntry* row;
    s16 index;
    s32 i;
    u8* option;
    u8 value;
    u32 color;

    row = slot->entries_b;
    index = slot->scroll.rows_per_page * slot->scroll.col;
    i = 0;
    option = &page04_option_ids[index];
    for (; i < slot->scroll.row_limit; option++, index++, row++, i++) {
        row->field_0x02 = 1;
        row->page_kind = index;
        row->selected = i == slot->scroll.row;
        switch (index) {
        case 14:
        case 15:
            if (page04_pointer_row_ok(slot) == 0) {
                row->field_0x02 = 0;
            }
            break;
        case 6:
            if (ck_cfg(slot->worker->chunk_ofs, 14) == 1) {
                row->field_0x02 = 0;
            }
            break;
        case 0:
            if (page04_control_row_ok(slot) == 0) {
                row->field_0x02 = 0;
            }
            break;
        }
        if (row->field_0x02 == 0) {
            row->field_0x0C = 0x8C8C8CFF;
        } else {
            color = 0x6E8C7EFF;
            if (row->selected == 1) {
                color = 0x7DE6BEFF;
            }
            row->field_0x0C = color;
        }
        value = get_cfg(slot->worker->chunk_ofs, *option);
        if (value == 0) {
            row->field_0x06 = 1;
        } else if (value >= option_cfg_max[*option]) {
            row->field_0x06 = 2;
        } else {
            row->field_0x06 = 0;
        }
    }
}

/* 0x80441D80 (0x3CC): draws page 4: the arrows, each row's name and value, the cursor and the help text. */
void movie_page04_draw(MenuSlot* slot) {
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    _SPR_DATA_ spr;
    u8 arrows;
    MenuEntry* row;
    u16* names;
    s32 i;
    u16* values;
    s8* text;
    s32 color;
    u16* sprites;
    u8** msgs;
    u16 index;

    get_lsp_data(0x74C, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xD7), &pos);
    arrows = slot->field_0x001 == 0 ? 0 : 0x80;
    PutPageArrow((u16*)get_menu_lsp_tbl(0xD9), slot->scroll.col, slot->scroll.col_count, slot->scroll.move_flags, &pos,
                 arrows);
    row = slot->entries_b;
    names = (u16*)get_menu_lsp_tbl(0xDA);
    i = 0;
    values = (u16*)get_menu_lsp_tbl(0xDB);
    for (; i < slot->scroll.row_limit; values++, i++, row++) {
        if (names[row->page_kind] == 0xFFFF) {
            continue;
        }
        spr_data_copy(&spr, get_lsp_data(names[row->page_kind], NULL));
        spr.color = row->field_0x0C;
        text = (s8*)get_menu_msg(0x1E, row->page_kind);
        draw_font(spr, text, 4, &pos);
        get_lsp_data(*values, &at);
        at.x += pos.x;
        at.y += pos.y;
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xDC), &at);
        color = GetMenuFontColor(row->field_0x02 == 1, row->selected == 1, 1, 0);
        if (row->selected != 0) {
            option_cursor_draw(slot, row, &at);
        }
        switch (row->page_kind) {
        case 2:
        case 3:
            switch (row->field_0x06) {
            case 0:
                draw_number_idx(0x78B, ck_cfg(slot->worker->chunk_ofs, page04_option_ids[row->page_kind]), color, &at);
                break;
            case 1:
                option_value_sprite_draw(0x789, color, &at);
                break;
            case 2:
                option_value_sprite_draw(0x78A, color, &at);
                break;
            }
            break;
        case 6:
        case 7:
        case 11:
        case 13:
            sprites = ((u16**)get_menu_lsp_tbl(0xE2))[row->page_kind];
            if (sprites != NULL) {
                option_value_sprite_draw(sprites[ck_cfg(slot->worker->chunk_ofs, page04_option_ids[row->page_kind])],
                                         color, &at);
            }
            break;
        default:
            sprites = ((u16**)get_menu_lsp_tbl(0xE2))[row->page_kind];
            if (sprites != NULL) {
                option_value_sprite_draw(sprites[ck_cfg(slot->worker->chunk_ofs, page04_option_ids[row->page_kind])],
                                         color, &at);
            }
            break;
        }
    }
    get_lsp_data(0x794, &pos);
    draw_window_frame_preset(pos.x, pos.y, 0);
    font_set_size(16, 16);
    switch (slot->field_0x001) {
    case 0:
        text = (s8*)get_menu_msg(0x21, slot->scroll.index);
        break;
    case 1:
        index = slot->scroll.index;
        msgs = ((u8***)get_menu_msg_tbl(0x23))[index];
        if ((u16)(index - 2) <= 1) {
            text = (s8*)msgs[0];
        } else {
            text = (s8*)msgs[ck_cfg(slot->worker->chunk_ofs, page04_option_ids[index])];
        }
        break;
    }
    font_print_ex(pos.x + 24, pos.y + 16, 0, text);
}

/* 0x8044214C (0x98): draws sprite record `id` in `color` at `pos`. */
static void option_value_sprite_draw(u16 id, u32 color, _mh_ivec2_* pos) {
    _SPR_DATA_ spr;

    spr = *get_lsp_data(id, NULL);
    spr.color = color;
    draw_sprite(spr, pos);
}

/* 0x804421E4 (0xB0): draws the row cursor and, while a value is being changed, its blinking arrows. */
static void option_cursor_draw(MenuSlot* slot, MenuEntry* row, _mh_ivec2_* pos) {
    s16 state;
    u16 left;
    u16 right;

    if (chk_pointer() == 0) {
        put_menu_cursor((u16*)get_menu_lsp_tbl(0xDD), 0, pos);
        if (slot->field_0x001 == 1) {
            state = row->field_0x06;
            left = 0xFFFF;
            if (state != 1) {
                left = 0x776;
            }
            right = 0xFFFF;
            if (state != 2) {
                right = 0x777;
            }
            draw_sprite_blink_idx(left, right, 0xFFFFC8FF, slot->option_move_0x236, pos);
        }
    }
}

/* 0x80442294 (0x14): opens the split-screen option page. */
void movie_page04_split_open(MenuSlot* slot) {
    slot->field_0x014 = 4;
    slot->field_0x001 = 0;
    page04_split_scroll_init(slot);
}

/* 0x804422A8 (0x44): sets the split page's scroll (13 rows, 5 a page) and fills its rows. */
static void page04_split_scroll_init(MenuSlot* slot) {
    menu_scroll_init(&slot->scroll, 0, 13, 5);
    page04_split_rows_build(slot);
}

/* 0x804422EC (0xE4): raises or lowers player `player`'s option `option` by one; returns the arrow that moved it. */
static s16 arena_value_step(u16 input, u8 option, u8 player) {
    s16 moved;
    u8 value;
    u8 next;

    moved = 0;
    value = get_arena_cfg(player, option);
    next = value;
    if (input & 8) {
        next = value + 1;
        moved = 8;
    } else if (input & 4) {
        if (value != 0) {
            next = value - 1;
        }
        moved = 4;
    }
    if (next != ck_arena_cfg(player, option)) {
        arena_cfg_set(player, option, next);
        if (next == get_arena_cfg(player, option)) {
            sysSE_req(3);
            return moved;
        }
    }
    return 0;
}

/* 0x804423D0 (0x19C): fills the split page's rows of the page. */
static void page04_split_rows_build(MenuSlot* slot) {
    MenuEntry* row;
    s16 index;
    s32 i;
    u8* option;
    u8 value;
    u32 color;

    row = slot->entries_b;
    index = slot->scroll.rows_per_page * slot->scroll.col;
    i = 0;
    option = &page04_split_option_ids[index];
    for (; i < slot->scroll.row_limit; option++, index++, row++, i++) {
        row->field_0x02 = 1;
        row->page_kind = index;
        row->selected = i == slot->scroll.row;
        switch (index) {
        case 11:
        case 12:
            if (page04_pointer_row_ok(slot) == 0) {
                row->field_0x02 = 0;
            }
            break;
        case 3:
            if (ck_arena_cfg(slot->field_0x011, 14) == 1) {
                row->field_0x02 = 0;
            }
            break;
        case 0:
            if (page04_control_row_ok(slot) == 0) {
                row->field_0x02 = 0;
            }
            break;
        }
        if (row->field_0x02 == 0) {
            row->field_0x0C = 0x8C8C8CFF;
        } else {
            color = 0x6E8C7EFF;
            if (row->selected == 1) {
                color = 0x7DE6BEFF;
            }
            row->field_0x0C = color;
        }
        value = get_arena_cfg(slot->field_0x011, *option);
        if (value == 0) {
            row->field_0x06 = 1;
        } else if (value >= option_cfg_max[*option]) {
            row->field_0x06 = 2;
        } else {
            row->field_0x06 = 0;
        }
    }
}

/* 0x8044256C (0x1B0): steps the split option page for the slot's player. */
s32 movie_page04_split_step(MenuSlot* slot) {
    s32 ret;
    u16 input;

    ret = 0;
    input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    switch (slot->field_0x001) {
    case 0:
        if (input & 0x20) {
            ret = 2;
            sysSE_req(1);
        } else if (input & 0x10) {
            if (slot->entries_b[slot->scroll.row].field_0x02 != 0) {
                menu_slot_lock_set(slot->pad_no, 1);
                slot->option_value_0x234 = get_arena_cfg(slot->field_0x011, page04_split_option_ids[slot->scroll.index]);
                slot->field_0x001 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else {
            page04_split_cursor_step(slot, input);
        }
        break;
    case 1:
        slot->option_move_0x236 = 0;
        if (input & 0x20) {
            arena_cfg_set(slot->field_0x011, page04_split_option_ids[slot->scroll.index], slot->option_value_0x234);
            page04_split_rows_build(slot);
            slot->field_0x001 = 0;
            sysSE_req(1);
            menu_slot_lock_set(slot->pad_no, 0);
        } else if (input & 0x10) {
            slot->field_0x001 = 0;
            sysSE_req(0);
            menu_slot_lock_set(slot->pad_no, 0);
        } else if (input & 0xC) {
            slot->option_move_0x236 =
                arena_value_step(input, page04_split_option_ids[slot->scroll.index], slot->field_0x011);
            page04_split_rows_build(slot);
        }
        break;
    }
    return ret;
}

/* 0x8044271C (0x58): moves the split page's cursor and refills the rows when it moved. */
static void page04_split_cursor_step(MenuSlot* slot, u16 input) {
    MenuScroll* scroll;

    scroll = &slot->scroll;
    menu_scroll_step(scroll, input, 5);
    if (scroll->move_flags != 0) {
        page04_split_rows_build(slot);
    }
}

/* 0x80442774 (0x268): draws the split option page in the slot's half of the screen. */
void movie_page04_split_draw(MenuSlot* slot) {
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    _mh_ivec2_ at;
    MenuEntry* row;
    u16* names;
    s32 i;
    u16* values;
    _SPR_DATA_* anchor;
    s32 color;
    u16* sprites;

    get_lsp_data(((u16*)get_menu_lsp_tbl(0x18A))[slot->pad_no], &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x18B), &pos);
    PutPageArrow((u16*)get_menu_lsp_tbl(0x18D), slot->scroll.col, slot->scroll.col_count, slot->scroll.move_flags, &pos,
                 0);
    row = slot->entries_b;
    names = (u16*)get_menu_lsp_tbl(0x18E);
    i = 0;
    values = (u16*)get_menu_lsp_tbl(0x18F);
    for (; i < slot->scroll.row_limit; values++, i++, row++) {
        spr_data_copy(&spr, get_lsp_data(names[row->page_kind], NULL));
        spr.color = row->field_0x0C;
        draw_font(spr, (s8*)get_menu_msg(0x1F, row->page_kind), 4, &pos);
        anchor = get_lsp_data(*values, NULL);
        at.x = pos.x + anchor->pos.x;
        at.y = pos.y + anchor->pos.y;
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xDC), &at);
        if (row->selected != 0) {
            option_cursor_draw(slot, row, &at);
        }
        color = GetMenuFontColor(row->field_0x02 == 1, row->selected == 1, 1, 0);
        switch (row->page_kind) {
        case 3:
        case 4:
        case 8:
        case 10:
            sprites = ((u16**)get_menu_lsp_tbl(0x190))[row->page_kind];
            option_value_sprite_draw(sprites[ck_arena_cfg(slot->field_0x011, page04_split_option_ids[row->page_kind])],
                                     color, &at);
            break;
        default:
            sprites = ((u16**)get_menu_lsp_tbl(0x190))[row->page_kind];
            option_value_sprite_draw(sprites[ck_arena_cfg(slot->field_0x011, page04_split_option_ids[row->page_kind])],
                                     color, &at);
            break;
        }
    }
}

/* 0x804429DC (0x14): opens menu page 17. */
void movie_page17_open(MenuSlot* slot) {
    slot->field_0x014 = 17;
    slot->field_0x001 = 0;
    page17_scroll_init(slot);
}

/* 0x804429F0 (0x7C): sets page 17's scroll (three rows, six with the network option on) and fills its rows. */
static void page17_scroll_init(MenuSlot* slot) {
    MenuScroll* scroll;

    scroll = &slot->scroll;
    if (get_option_cfg(28) == 0) {
        menu_scroll_init(scroll, 0, 3, 3);
    } else {
        menu_scroll_init(scroll, 0, 6, 3);
    }
    page17_rows_build(slot);
}

/* 0x80442A6C (0x80): the value of transfer row `row` (3 transfer on/off, 4 level, 5 flag). */
static u8 page17_transfer_get(u8 row) {
    switch (row) {
    case 3:
        if (system_w.transfer_mode_0xa50 == 1) {
            return 0;
        }
        return 1;
    case 4:
        return system_w.transfer_level_0xa52;
    case 5:
        if (system_w.transfer_flag_0xa51 == 1) {
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 0x80442AEC (0xBC): stores `value` (clamped) as transfer row `row` and applies it. */
static void page17_transfer_set(u8 row, u8 value) {
    u8 max;

    max = page17_value_max[row - 3];
    if (value >= max) {
        value = max;
    }
    switch (row) {
    case 3:
        if (value == 0) {
            system_w.transfer_mode_0xa50 = 1;
        } else {
            system_w.transfer_mode_0xa50 = 0;
        }
        applyTransferSettings();
        break;
    case 4:
        system_w.transfer_level_0xa52 = value;
        applyTransferLevel();
        break;
    case 5:
        if (value == 0) {
            system_w.transfer_flag_0xa51 = 1;
        } else {
            system_w.transfer_flag_0xa51 = 0;
        }
        syncNetTransferFlag();
        break;
    }
}

/* 0x80442BA8 (0x1B8): fills page 17's rows of the page. */
static void page17_rows_build(MenuSlot* slot) {
    MenuEntry* row;
    s16 index;
    s32 i;
    u8 value;
    u32 color;

    row = slot->entries_b;
    index = slot->scroll.rows_per_page * slot->scroll.col;
    i = 0;
    for (; i < slot->scroll.row_limit; index++, row++, i++) {
        row->page_kind = index;
        row->selected = i == slot->scroll.row;
        if (i == 2 && slot->field_0x00F == 1) {
            row->field_0x02 = 0;
        } else {
            row->field_0x02 = 1;
        }
        if (index == 4 || index == 5) {
            if (page17_transfer_get(3) == 1) {
                row->field_0x02 = 0;
            } else {
                row->field_0x02 = 1;
            }
        }
        if (row->field_0x02 == 0) {
            row->field_0x0C = 0x8C8C8CFF;
        } else {
            color = 0x6E8C7EFF;
            if (row->selected == 1) {
                color = 0x7DE6BEFF;
            }
            row->field_0x0C = color;
        }
        if (index < 3) {
            value = get_option_cfg(page17_option_ids[index]);
            if (value == 0) {
                row->field_0x06 = 1;
            } else if (value >= option_cfg_max[page17_option_ids[index]]) {
                row->field_0x06 = 2;
            } else {
                row->field_0x06 = 0;
            }
        } else {
            value = page17_transfer_get(index);
            if (value == 0) {
                row->field_0x06 = 1;
            } else if (value >= page17_value_max[index - 3]) {
                row->field_0x06 = 2;
            } else {
                row->field_0x06 = 0;
            }
        }
    }
}

/* 0x80442D60 (0x1D4): steps page 17: the option rows and the transfer rows. */
s32 movie_page17_step(MenuSlot* slot) {
    s32 ret;
    u16 input;

    ret = 0;
    input = slot->buttons_trig | (slot->field_0x008 & 0xF);
    switch (slot->field_0x001) {
    case 0:
        if (input & 0x20) {
            ret = 2;
            sysSE_req(1);
        } else if (input & 0x10) {
            if (slot->entries_b[slot->scroll.row].field_0x02 != 0) {
                if (slot->scroll.index <= 2) {
                    slot->option_value_0x234 = get_option_cfg(page17_option_ids[slot->scroll.index]);
                } else {
                    slot->option_value_0x234 = page17_transfer_get(slot->scroll.index);
                }
                slot->field_0x001 = 1;
                sysSE_req(0);
                menu_slot0_lock_set(1);
            } else {
                sysSE_req(2);
            }
        } else {
            page17_cursor_step(slot, input);
        }
        break;
    case 1:
        slot->option_move_0x236 = 0;
        if (input & 0x20) {
            if (slot->scroll.index <= 2) {
                set_option_cfg(page17_option_ids[slot->scroll.index], slot->option_value_0x234);
            } else {
                page17_transfer_set(slot->scroll.index, slot->option_value_0x234);
            }
            page17_rows_build(slot);
            slot->field_0x001 = 0;
            sysSE_req(1);
            menu_slot0_lock_set(0);
        } else if (input & 0x10) {
            slot->field_0x001 = 0;
            sysSE_req(0);
            menu_slot0_lock_set(0);
        } else if (input & 0xC) {
            if (slot->scroll.index <= 2) {
                slot->option_move_0x236 = option_value_step(input, page17_option_ids[slot->scroll.index]);
            } else {
                slot->option_move_0x236 = page17_transfer_step(input, slot->scroll.index);
            }
            page17_rows_build(slot);
        }
        break;
    }
    return ret;
}

/* 0x80442F34 (0x58): moves page 17's cursor and refills the rows when it moved. */
static void page17_cursor_step(MenuSlot* slot, u16 input) {
    MenuScroll* scroll;

    scroll = &slot->scroll;
    menu_scroll_step(scroll, input, 5);
    if (scroll->move_flags != 0) {
        page17_rows_build(slot);
    }
}

/* 0x80442F8C (0x194): draws the terms progress bar at `pos`: its frame, its black well and the filled part. */
static void terms_progress_draw(_mh_ivec2_* pos) {
    Vec at;
    Vec2 size;
    GXColor color;
    f32 rate;

    at.x = 8.0f + pos->x;
    at.y = 5.0f + pos->y;
    at.z = 0.0f;
    size.x = 206.0f;
    size.y = 11.0f;
    color.r = 0x5F;
    color.g = 0x55;
    color.b = 0x4B;
    color.a = 0xFF;
    disp_beta(&at, &size, color);
    at.x += 3.0f;
    at.y += 3.0f;
    size.x = 200.0f;
    size.y = 5.0f;
    disp_beta(&at, &size, color_black);
    rate = getNetTermsProgressLevel() * 0.0625f;
    if (rate > 1.0f) {
        rate = 1.0f;
    } else if (rate < 0.0f) {
        rate = 0.0f;
    }
    size.x = 200.0f * rate;
    color.r = 0x57;
    color.g = 0xC0;
    color.b = 0x91;
    color.a = 0xFF;
    disp_beta(&at, &size, color);
}

/* 0x80443120 (0x434): draws page 17: the rows, their values, the cursor, the progress bar and the help text. */
void movie_page17_draw(MenuSlot* slot) {
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    _SPR_DATA_ spr;
    u8 pages;
    u8 arrows;
    MenuEntry* row;
    u16* names;
    s32 i;
    u16* values;
    s8* text;
    s32 color;
    u16* sprites;
    u8** msgs;
    u16 index;
    u8 value;

    get_lsp_data(0x74C, &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xD7), &pos);
    pages = slot->scroll.col_count;
    if (pages >= 2) {
        arrows = slot->field_0x001 == 0 ? 0 : 0x80;
        PutPageArrow((u16*)get_menu_lsp_tbl(0xD9), slot->scroll.col, pages, slot->scroll.move_flags, &pos, arrows);
    }
    row = slot->entries_b;
    names = (u16*)get_menu_lsp_tbl(0xDA);
    i = 0;
    values = (u16*)get_menu_lsp_tbl(0xDB);
    for (; i < slot->scroll.row_limit; names++, values++, i++, row++) {
        if (row->page_kind == -1 || *names == 0xFFFF) {
            continue;
        }
        spr_data_copy(&spr, get_lsp_data(*names, NULL));
        spr.color = row->field_0x0C;
        if (row->page_kind <= 2) {
            text = (s8*)get_menu_msg(0x20, row->page_kind);
        } else {
            text = (s8*)MH3GetErrorString2(row->page_kind + 0x27);
        }
        draw_font(spr, text, 4, &pos);
        get_lsp_data(*values, &at);
        at.x += pos.x;
        at.y += pos.y;
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xDC), &at);
        color = GetMenuFontColor(row->field_0x02 == 1, row->selected == 1, 1, 0);
        if (row->selected != 0) {
            option_cursor_draw(slot, row, &at);
        }
        if (row->page_kind == 4) {
            switch (row->field_0x06) {
            case 0:
                draw_number_idx(0x78B, page17_transfer_get(row->page_kind), color, &at);
                break;
            case 1:
                option_value_sprite_draw(0x789, color, &at);
                break;
            case 2:
                option_value_sprite_draw(0x78A, color, &at);
                break;
            }
        } else {
            sprites = ((u16**)get_menu_lsp_tbl(0xE3))[row->page_kind];
            if (sprites != NULL) {
                if (row->page_kind <= 2) {
                    option_value_sprite_draw(sprites[ck_option_cfg(page17_option_ids[row->page_kind])], color, &at);
                } else {
                    option_value_sprite_draw(sprites[page17_transfer_get(row->page_kind)], color, &at);
                }
            }
        }
    }
    if (slot->scroll.col == 1) {
        get_lsp_data(0x762, &at);
        at.x += pos.x;
        at.y += pos.y;
        terms_progress_draw(&at);
    }
    get_lsp_data(0x794, &pos);
    draw_window_frame_preset(pos.x, pos.y, 0);
    font_set_size(16, 16);
    switch (slot->field_0x001) {
    case 0:
        index = slot->scroll.index;
        if (index <= 2) {
            text = (s8*)get_menu_msg(0x22, index);
        } else {
            text = (s8*)MH3GetErrorString2(page17_title_msg[index - 3]);
        }
        break;
    case 1:
        index = slot->scroll.index;
        msgs = ((u8***)get_menu_msg_tbl(0x24))[index];
        if (index <= 2) {
            text = (s8*)msgs[ck_option_cfg(page17_option_ids[index])];
        } else if (index == 3 || index == 5) {
            value = page17_transfer_get(index);
            text = (s8*)MH3GetErrorString2(page17_value_msg[slot->scroll.index - 3] + value);
        } else {
            text = (s8*)MH3GetErrorString2(page17_value_msg[index - 3]);
        }
        break;
    }
    font_print_ex(pos.x + 24, pos.y + 16, 0, text);
}

#pragma optimization_level 3
/* ---------------------------------------------------------------------------------------------------------------- */
/* The character make. */

/* size: 0x274 (the allocation `charmake_init` makes) - the character make's work: its steps, the edited item, the
 * hunter's name and look, the colour palette cursor, the camera and the model's turn. */
typedef struct CharmakeWork {
    /* +0x000 */ u8 step;
    /* +0x001 */ u8 sub;
    /* +0x002 */ u8 sub2;
    /* +0x003 */ u8 pad_0x003;
    /* +0x004 */ s16 timer;                 /* the fade-out frames, then the ending's frame count */
    /* +0x006 */ u8 pad_0x006[6];
    /* +0x00C */ u8 item;                   /* the edited item: 1..11, 12 the finish row */
    /* +0x00D */ u8 pad_0x00D[2];
    /* +0x00F */ char name[9];
    /* +0x018 */ u8 sex;                    /* the hunter model (0 or 1) */
    /* +0x019 */ s8 equip;                  /* the demo equipment set (0..3) */
    /* +0x01A */ u8 inner_rgb[3];           /* the inner-wear colour */
    /* +0x01D */ u8 pad_0x01D;
    /* +0x01E */ s8 face;
    /* +0x01F */ u8 pad_0x01F[2];
    /* +0x021 */ s8 makeup_color;
    /* +0x022 */ s8 hair;
    /* +0x023 */ u8 hair_rgb[3];            /* the hair colour */
    /* +0x026 */ s8 voice;
    /* +0x027 */ u8 makeup;
    /* +0x028 */ s8 voice_sample;           /* the next sample the voice row plays */
    /* +0x029 */ u8 pad_0x029[3];
    /* +0x02C */ u32 voice_handle;          /* the sample playing (0: none) */
    /* +0x030 */ s8 palette_cursor;         /* the colour palette's cell (12: custom) */
    /* +0x031 */ u8 palette_custom;         /* 1 while the custom colour box is open */
    /* +0x032 */ s16 custom_x;              /* the custom colour box's cursor */
    /* +0x034 */ s16 custom_y;
    /* +0x036 */ s8 left_arrow;             /* the sex arrows' blink frames (-1: off) */
    /* +0x037 */ s8 right_arrow;
    /* +0x038 */ u8 camera;                 /* the camera position (0, 1 the close-up, 2 the ending) */
    /* +0x039 */ u8 pad_0x039[0x1C4 - 0x039];
    /* +0x1C4 */ nw4r::math::VEC3 eye_goal;
    /* +0x1D0 */ nw4r::math::VEC3 eye;
    /* +0x1DC */ nw4r::math::VEC3 at_goal;
    /* +0x1E8 */ nw4r::math::VEC3 at;
    /* +0x1F4 */ u8 pad_0x1F4[4];
    /* +0x1F8 */ _CP_VECTOR turn;           /* the model's rotation */
    /* +0x204 */ u8 pad_0x204[0x218 - 0x204];
    /* +0x218 */ u32 hair_color;            /* the hair colour as an RGBA word */
    /* +0x21C */ u32 inner_color;           /* the inner-wear colour as an RGBA word */
    /* +0x220 */ s16 layout[10];            /* the palette screen's boxes */
    /* +0x234 */ u8 pad_0x234[0x270 - 0x234];
    /* +0x270 */ s16 hold;                  /* frames the item rows ignore the pad */
    /* +0x272 */ u8 confirm;                /* 1 while a confirmation is shown */
    /* +0x273 */ u8 pad_0x273;
} CharmakeWork;

/* size: 0xC - one equipment piece of a demo set, copied as three words. */
typedef struct DemoEquipPiece {
    /* +0x0 */ u32 word[3];
} DemoEquipPiece;

/* size: 0x60 - a demo set's skill list. */
typedef struct DemoEquipSkills {
    /* +0x00 */ u32 word[24];
} DemoEquipSkills;

/* size: 0x20 - a demo set's decoration list. */
typedef struct DemoEquipDecos {
    /* +0x00 */ u32 word[8];
} DemoEquipDecos;

/* size: 0x100 - one demo equipment set: the header bytes, the two colour words, nine pieces and the two lists (the
 * record `arena_userdata_apply` applies). */
typedef struct DemoEquipSet {
    /* +0x00 */ u8 head[4];
    /* +0x04 */ u32 color_a;
    /* +0x08 */ u8 kind[6];
    /* +0x0E */ u8 pad_0x0E[2];
    /* +0x10 */ u32 color_b;
    /* +0x14 */ DemoEquipPiece piece0;
    /* +0x20 */ DemoEquipPiece piece1;
    /* +0x2C */ DemoEquipPiece piece2;
    /* +0x38 */ DemoEquipPiece piece3;
    /* +0x44 */ DemoEquipPiece piece4;
    /* +0x50 */ DemoEquipPiece piece5;
    /* +0x5C */ DemoEquipPiece piece6;
    /* +0x68 */ DemoEquipPiece piece7;
    /* +0x74 */ DemoEquipPiece piece8;
    /* +0x80 */ DemoEquipSkills skills;
    /* +0xE0 */ DemoEquipDecos decos;
} DemoEquipSet;

/* The two colour-change callbacks the character make's palette uses (one per colour kind). */
typedef struct CharmakeColorFuncs {
    /* +0x0 */ void (*set)(u8 kind, u8* rgb);
    /* +0x4 */ u32 (*name_ok)(char* name);
} CharmakeColorFuncs; /* size: 0x8 */

/* A light direction built through `setVec3`, so the file-scope table is constructed by `__sinit`.  size: 0xC */
struct CharmakeLightDir : public nw4r::math::VEC3 {
    /* Builds (x, y, z). */
    CharmakeLightDir(f32 x, f32 y, f32 z) {
        setVec3(this, x, y, z);
    }
};

extern "C" {
/* The character make's work (allocated by `charmake_init`). */
CharmakeWork* charmake_work;
/* The palette's callbacks. */
CharmakeColorFuncs charmake_func;
}

/* The male and female demo equipment sets `charmake_init` copies into `demoEquip`. */
DemoEquipSet demo_equip_male = {
    { 0x00, 0x00, 0x00, 0x00 }, 0x000000FF, { 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 }, { 0, 0 }, 0x000000FF,
    { { 0x0700001D, 0x00000000, 0x00000000 } },
    { { 0x00000000, 0x00000000, 0x00000000 } },
    { { 0x00000000, 0x00000000, 0x00000000 } },
    { { 0x05000000, 0x00000000, 0x00000000 } },
    { { 0x01000000, 0x00000000, 0x00000000 } },
    { { 0x03000000, 0x00000000, 0x00000000 } },
    { { 0x02000000, 0x00000000, 0x00000000 } },
    { { 0x04000000, 0x00000000, 0x00000000 } },
    { { 0x06000000, 0x00000000, 0x00000000 } },
    { { 0 } },
    { { 0 } },
};
DemoEquipSet demo_equip_female = {
    { 0x01, 0x00, 0x00, 0x00 }, 0x000000FF, { 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 }, { 0, 0 }, 0x000000FF,
    { { 0x0700001D, 0x00000000, 0x00000000 } },
    { { 0x00000000, 0x00000000, 0x00000000 } },
    { { 0x00000000, 0x00000000, 0x00000000 } },
    { { 0x05000000, 0x00000000, 0x00000000 } },
    { { 0x01000000, 0x00000000, 0x00000000 } },
    { { 0x03000000, 0x00000000, 0x00000000 } },
    { { 0x02000000, 0x00000000, 0x00000000 } },
    { { 0x04000000, 0x00000000, 0x00000000 } },
    { { 0x06000000, 0x00000000, 0x00000000 } },
    { { 0 } },
    { { 0 } },
};
/* The face and hair model ids each sex offers. */
u16 charmake_face_ids_male[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
u16 charmake_face_ids_female[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
u16 charmake_hair_ids_male[11] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
u16 charmake_hair_ids_female[11] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
u16* charmake_face_ids[2] = { charmake_face_ids_male, charmake_face_ids_female };
u8 charmake_face_count[2] = { 9, 9 };
u16* charmake_hair_ids[2] = { charmake_hair_ids_male, charmake_hair_ids_female };
u8 charmake_hair_count[2] = { 11, 11 };

static void charmake_turn_step(CharmakeWork* work);
static void demo_equip_copy(DemoEquipSet* dst, DemoEquipSet* src);

/* The two demo equipment sets the character make dresses each model in (one per sex, plus a spare). */
DemoEquipSet demoEquip[3];
/* The name the software keyboard edits. */
char charmake_name_buffer[0x18];

/* The four light colours of the character make's scene. */
GXColor charmake_light_colors[4] = {
    { 0xFF, 0xFF, 0xFF, 0xFF }, { 0x40, 0x50, 0x50, 0xFF }, { 0x1A, 0x1A, 0x1A, 0xFF }, { 0x80, 0x80, 0x80, 0xFF },
};
/* The camera's eye and target for each camera position. */
Vec charmake_camera_table[6] = {
    { 0.0f, 85.0f, 300.0f },   { -80.0f, 75.0f, 0.0f }, { -20.0f, 160.0f, 80.0f },
    { -20.0f, 150.0f, 0.0f },  { 0.0f, 70.0f, 350.0f }, { 0.0f, 70.0f, 0.0f },
};

/* The four light directions. */
CharmakeLightDir charmake_light_dirs[4] = {
    CharmakeLightDir(0.0f, 0.0f, 50.0f),
    CharmakeLightDir(-50.0f, 70.0f, 40.0f),
    CharmakeLightDir(70.0f, -30.0f, -50.0f),
    CharmakeLightDir(0.0f, 100.0f, 0.0f),
};

/* 0x80443554 (0x4C): picks palette cell `index` of colour kind `kind` (1 the inner wear, else the hair). */
static void charmake_color_pick(u8 kind, u8 index, u32* rgba, s16* x, s16* y) {
    u16 id;

    if (kind == 1) {
        get_change_inner_color(index, rgba, &id, x, y);
    } else {
        get_change_hair_color(index, rgba, &id, x, y);
    }
}

/* 0x804435A0 (0x24C): copies character-make parameter `kind` to `out`. */
/* untyped: caller-owned payload - each kind fills its own parameter block */
void get_charmake_param(u8 kind, void* param) {
    CharmakeWork* work;
    u8* out;

    out = (u8*)param;
    work = charmake_work;
    if (work == NULL) {
        return;
    }
    switch (kind) {
    case 0:
        memcpy(out, work->name, 10);
        break;
    case 12:
        if (work->item == 1) {
            out[0] = work->sub;
        } else {
            out[0] = 0;
        }
        break;
    case 1:
        out[0] = work->sex;
        out[1] = work->left_arrow;
        out[2] = work->right_arrow;
        break;
    case 2:
        out[0] = face_type_get(work->sex);
        out[1] = face_eye_get(work->sex);
        break;
    case 3:
        out[0] = work->equip;
        break;
    case 4:
        out[0] = work->inner_rgb[0];
        out[1] = work->inner_rgb[1];
        out[2] = work->inner_rgb[2];
        out[3] = work->palette_cursor;
        out[4] = work->palette_custom;
        out[5] = work->custom_x;
        out[6] = work->custom_y;
        break;
    case 5:
        out[0] = work->face;
        break;
    case 6:
        out[0] = work->makeup;
        break;
    case 7:
        out[0] = work->makeup_color;
        break;
    case 8:
        out[0] = work->hair;
        break;
    case 9:
        out[0] = work->hair_rgb[0];
        out[1] = work->hair_rgb[1];
        out[2] = work->hair_rgb[2];
        out[3] = work->palette_cursor;
        out[4] = work->palette_custom;
        out[5] = work->custom_x;
        out[6] = work->custom_y;
        break;
    case 10:
        out[0] = work->voice;
        break;
    case 11:
        out[0] = work->item;
        if (work->step == 4) {
            if (work->item == 12 && work->sub2 == 2) {
                out[1] = 3;
            } else {
                out[1] = 1;
            }
        } else if (work->step == 3 && work->sub == 1) {
            out[1] = 2;
        } else if (work->step == 3 && work->sub == 2) {
            out[1] = 4;
        } else {
            out[1] = 0;
        }
        if (work->confirm == 0) {
            out[2] = 0;
        } else {
            out[2] = 1;
        }
        break;
    }
}

/* 0x804437EC (0x10): stores `value` as byte `index` of the character make's work. */
void set_charmake_r_no(u8 index, u8 value) {
    ((u8*)charmake_work)[index] = value;
}

/* 0x804437FC (0xA8): sets the scene's ambient and four directional lights. */
static void charmake_light_set(void) {
    s32 i;
    GXColor* color;
    CharmakeLightDir* dir;

    set_amblight(0, (GXColor){ 0x1A, 0x1A, 0x1A, 0xFF });
    color = charmake_light_colors;
    make_dir_light2(0, charmake_light_dirs, *color, 0);
    for (i = 0, dir = charmake_light_dirs; i < 4; color++, dir++, i++) {
        make_dir_light2(i + 5, dir, *color, 0);
    }
}

/* 0x804438A4 (0xB4): sets the ending pose: each shown model plays the decided motion pair. */
static void charmake_finish_pose(CharmakeWork* work) {
    u16 motion[2];
    s32 i;
    u16 count;
    _PLW* plw;

    Pl_decide_mot_get(&motion[0], &motion[1]);
    work->turn.y = 0;
    plw = (_PLW*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    for (i = 0; i < count; plw++, i++) {
        if (plw->slot_active != 0 && plw->field_0x001 != 0) {
            plw->field_0x058 = 0;
            Pl_chr_set_attr(plw, motion[i & 1], 10, 0, 1);
        }
    }
}

/* 0x80443958 (0x1A4): turns, colours and poses the two models; only the edited sex's model is shown. */
static void charmake_model_update(CharmakeWork* work) {
    s32 i;
    u16 count;
    _PLW* plw;

    if (work->palette_custom == 0) {
        charmake_turn_step(work);
    }
    plw = (_PLW*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    for (i = 0; i < count; plw++, i++) {
        plw->motion_pos_0x3C = 0.0f;
        plw->motion_pos_0x40 = 0.0f;
        plw->motion_pos_0x44 = 0.0f;
        eft_rot_vec_copy(&plw->rot_0x54, &work->turn);
        plw->field_0x01E = 1;
        plw->field_0x258[0] = (work->hair_color & 0xFF000000) >> 24;
        plw->field_0x258[1] = (work->hair_color & 0xFF0000) >> 16;
        plw->field_0x258[2] = (work->hair_color & 0xFF00) >> 8;
        plw->field_0x258[3] = 0xFF;
        plw->field_0x25C[0] = (work->inner_color & 0xFF000000) >> 24;
        plw->field_0x25C[1] = (work->inner_color & 0xFF0000) >> 16;
        plw->field_0x25C[2] = (work->inner_color & 0xFF00) >> 8;
        plw->field_0x25C[3] = 0xFF;
        if (Get_motion_no(plw) < 700) {
            if (i & 1) {
                Pl_chr_setX(plw, 700, 0, 0);
            } else {
                Pl_chr_setX(plw, 720, 0, 0);
            }
        }
        if (work->sex == i) {
            if (pl_net_slot_ready_ck(plw) != 0) {
                plw->field_0x001 = 1;
            } else {
                plw->field_0x001 = 0;
            }
        } else {
            plw->field_0x001 = 0;
        }
    }
}

/* 0x80443AFC (0x58): aims the camera's goals at camera position `index`. */
static void charmake_camera_goal_set(CharmakeWork* work, u8 index) {
    work->eye_goal.x = charmake_camera_table[index * 2].x;
    work->eye_goal.y = charmake_camera_table[index * 2].y;
    work->eye_goal.z = charmake_camera_table[index * 2].z;
    work->at_goal.x = charmake_camera_table[index * 2 + 1].x;
    work->at_goal.y = charmake_camera_table[index * 2 + 1].y;
    work->at_goal.z = charmake_camera_table[index * 2 + 1].z;
}

/* 0x80443B54 (0xCC): places the scene camera at camera position 0. */
static void charmake_camera_init(CharmakeWork* work) {
    s32 handle;
    nw4r::g3d::Camera camera;
    nw4r::g3d::Camera::PostureInfo posture;

    handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(0);
    word_copy_return_dst(&camera, &handle);
    camera_posture_info_ctor(&posture);
    work->camera = 0;
    charmake_camera_goal_set(work, 0);
    copyVec3(&work->eye, &work->eye_goal);
    copyVec3(&work->at, &work->at_goal);
    posture.mType = 2;
    copyVec3((nw4r::math::VEC3*)&posture.mTargetX, &work->at);
    posture.mUnk28 = 0.0f;
    camera.SetPosition(work->eye);
    camera.SetPosture(posture);
    camera.SetPerspective(50.0f, Screen_w.aspect, 1.0f, 10000.0f);
}

/* 0x80443C20 (0x10): switches between the full and the close-up camera. */
static void charmake_camera_toggle(CharmakeWork* work) {
    work->camera ^= 1;
    charmake_camera_goal_set(work, work->camera);
}

/* 0x80443C30 (0x110): moves the camera a tenth of the way to its goals. */
static void charmake_camera_step(CharmakeWork* work) {
    s32 handle;
    nw4r::g3d::Camera camera;
    nw4r::g3d::Camera::PostureInfo posture;

    handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(0);
    word_copy_return_dst(&camera, &handle);
    camera_posture_info_ctor(&posture);
    camera_posture_get(&camera, &posture);
    work->eye.x += (work->eye_goal.x - work->eye.x) / 10.0f;
    work->eye.y += (work->eye_goal.y - work->eye.y) / 10.0f;
    work->eye.z += (work->eye_goal.z - work->eye.z) / 10.0f;
    work->at.x += (work->at_goal.x - work->at.x) / 10.0f;
    work->at.y += (work->at_goal.y - work->at.y) / 10.0f;
    work->at.z += (work->at_goal.z - work->at.z) / 10.0f;
    copyVec3((nw4r::math::VEC3*)&posture.mTargetX, &work->at);
    camera.SetPosition(work->eye);
    camera.SetPosture(posture);
}

/* 0x80443D40 (0x110): turns the model with the remote's or the classic controller's shoulder buttons and switches
 * the camera with the minus button. */
static void charmake_turn_step(CharmakeWork* work) {
    PlayerPad* pad;
    u16 pressed;
    s32 turn;

    pad = Psw;
    pressed = pad->button_0x2C0.pressed_0x04;
    turn = 0;
    switch (get_ControlType(0)) {
    case 1:
        if (pad->control_0x104 & 0x400) {
            turn = 1;
        } else if (pad->control_0x104 & 0x800) {
            turn = 2;
        }
        break;
    case 2:
        if (pad->control_0x0DE & 4) {
            turn = 1;
        } else if (pad->control_0x0DE & 8) {
            turn = 2;
        }
        break;
    }
    if (turn & 1) {
        work->turn.y = (u16)(work->turn.y + 0x400);
    }
    if (turn & 2) {
        work->turn.y = (u16)(work->turn.y - 0x400);
    }
    if (pressed & 0x80) {
        charmake_camera_toggle(work);
    }
}

/* 0x80443E50 (0x124): sets up the character make: the work, the demo sets, the two hunter models and the
 * defaults. */
void charmake_init(void) {
    CharmakeWork* work;

    GameMode_set(0);
    charmake_work = (CharmakeWork*)work_mem_alloc(sizeof(CharmakeWork));
    memset(charmake_work, 0, sizeof(CharmakeWork));
    demo_equip_copy(&demoEquip[0], &demo_equip_male);
    demo_equip_copy(&demoEquip[1], &demo_equip_female);
    create_move_work(6);
    set_move_work_max(2, 2);
    set_max_player(2);
    set_com_motion_type(3);
    quest_snd_wk_init();
    face_work_init();
    init_player_work();
    player_init_data_load();
    work = charmake_work;
    work->sex = 0;
    work->layout[4] = 0;
    work->layout[6] = 0;
    work->layout[5] = 0xBF;
    work->layout[7] = 0xBF;
    work->hair_color = 0x371A00FF;
    work->layout[0] = 20;
    work->layout[1] = 160;
    work->layout[8] = 32;
    work->layout[9] = 240;
    work->hair_rgb[0] = 0x38;
    work->hair_rgb[1] = 0x1A;
    work->hair_rgb[2] = 0;
    work->inner_color = 0xFFFFFFFF;
    work->layout[2] = 0xBF;
    work->layout[3] = 0;
    work->inner_rgb[0] = 0xFF;
    work->inner_rgb[1] = 0xFF;
    work->inner_rgb[2] = 0xFF;
    snd_bank_loader_reset();
}

/* 0x80443F74 (0x1A0): copies demo equipment set `src` to `dst`. */
static void demo_equip_copy(DemoEquipSet* dst, DemoEquipSet* src) {
    dst->head[0] = src->head[0];
    dst->head[1] = src->head[1];
    dst->head[2] = src->head[2];
    dst->head[3] = src->head[3];
    dst->color_a = src->color_a;
    dst->kind[0] = src->kind[0];
    dst->kind[1] = src->kind[1];
    dst->kind[2] = src->kind[2];
    dst->kind[3] = src->kind[3];
    dst->kind[4] = src->kind[4];
    dst->kind[5] = src->kind[5];
    dst->color_b = src->color_b;
    dst->piece0 = src->piece0;
    dst->piece1 = src->piece1;
    dst->piece2 = src->piece2;
    dst->piece3 = src->piece3;
    dst->piece4 = src->piece4;
    dst->piece5 = src->piece5;
    dst->piece6 = src->piece6;
    dst->piece7 = src->piece7;
    dst->piece8 = src->piece8;
    dst->skills = src->skills;
    dst->decos = src->decos;
}

/* 0x80444114 (0x48): selects face slot `sex` and shows that model's face, hiding the other's. */
static void charmake_face_select(u8 sex) {
    face_select_set(sex, sex);
    face_draw_on(sex);
    face_draw_off(sex ^ 1);
}

/* 0x8044415C (0x64): loads face `face` of model `sex`. */
static void charmake_face_load(u8 sex, u8 face) {
    face_tex_load(sex, charmake_face_ids[sex][face], 0);
    face_parts_tex_load(sex, 0, 0);
    face_tex_bind(sex);
}

/* 0x804441C0 (0x18): non-zero once the edited model's object is ready. */
static s32 charmake_model_ready_ck(CharmakeWork* work) {
    return pl_obj_ready_ck(&pl_global->table[work->sex]);
}

/* 0x804441D8 (0xBC): moves the item cursor up (1..12, wrapping); 1 when it moved. */
static u32 charmake_item_step(CharmakeWork* work) {
    PlayerPad* pad;
    u16 input;

    pad = Psw;
    input = pad->button_0x2C0.held_0x14 | pad->button_0x2C0.pressed_0x04;
    if (input & 1) {
        titleSE_req(3);
        if (--work->item < 1) {
            work->item = 12;
        }
        return 1;
    }
    if (input & 2) {
        titleSE_req(3);
        if (++work->item >= 13) {
            work->item = 1;
        }
        return 1;
    }
    return 0;
}

/* 0x80444294 (0xAC): applies the face choice to the edited model. */
static void charmake_face_apply(CharmakeWork* work) {
    _PLW* plw;
    u16* ids;
    u8 count;

    plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[work->sex];
    ids = charmake_face_ids[work->sex];
    count = charmake_face_count[work->sex];
    if (work->face >= count) {
        work->face = count - 1;
    }
    plw->field_0x25C[5] = work->face;
    charmake_face_load(work->sex, work->face);
    charmake_face_select(work->sex);
    pl_parts_set(work->sex, 6, ids[work->face]);
}

/* 0x80444340 (0xE4): applies the hair choice to the edited model (the previous one stays when it cannot be set). */
static void charmake_hair_apply(CharmakeWork* work) {
    _PLW* plw;
    s8 prev;
    u16* ids;
    u8 count;
    s8 ok;

    plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[work->sex];
    prev = work->hair;
    ids = charmake_hair_ids[work->sex];
    count = charmake_hair_count[work->sex];
    if (work->hair >= count) {
        work->hair = count - 1;
    }
    ok = pl_parts_set_ck(work->sex, 5, ids[work->hair]);
    if (ok == -1) {
        work->hair = prev;
        return;
    }
    if (ok != 0) {
        plw->field_0x25C[4] = work->hair;
        pl_parts_set(work->sex, 5, ids[work->hair]);
    }
}

/* 0x80444424 (0xF0): dresses the edited model in demo equipment set `equip`. */
static void charmake_equip_apply(CharmakeWork* work) {
    _PLW* plw;
    DemoEquipSet* set;

    plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[work->sex];
    set = &demoEquip[work->sex];
    plw->user_profile_0x1C = work->equip;
    pl_parts_set(work->sex, 0, pl_equip_model_id_get(plw, (_EQUIP*)&set->piece4));
    pl_parts_set(work->sex, 1, pl_equip_model_id_get(plw, (_EQUIP*)&set->piece6));
    pl_parts_set(work->sex, 2, pl_equip_model_id_get(plw, (_EQUIP*)&set->piece5));
    pl_parts_set(work->sex, 3, pl_equip_model_id_get(plw, (_EQUIP*)&set->piece7));
    pl_parts_set(work->sex, 4, pl_equip_model_id_get(plw, (_EQUIP*)&set->piece3));
}

/* 0x80444514 (0x178): starts a new user data from the character made: name, look, voice, colours and the seed. */
static void charmake_userdata_store(CharmakeWork* work) {
    Q_UserData* user;
    _PLW* plw;
    s32 i;
    u8 slot;

    user = get_userdata();
    userdata_clear();
    userdata_new_game_init(-1);
    plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[work->sex];
    for (i = strlen(work->name); i <= 0; i--) {
        if (work->name[i] == ' ') {
            work->name[i] = 0;
        }
    }
    strcpy(user->hunter_name_0x0003, work->name);
    user->hunter_sex_0x0000 = work->sex;
    user->hunter_face_0x0001 = plw->field_0x25C[5];
    user->hunter_voice_0x009F = work->voice;
    user->hunter_equip_0x009E = work->equip;
    user->inner_color_0x39BC = (work->inner_rgb[0] << 24) | (work->inner_rgb[1] << 16) | (work->inner_rgb[2] << 8) | 0xFF;
    user->hunter_hair_0x0002 = plw->field_0x25C[4];
    user->hair_color_0x0014 = (work->hair_rgb[0] << 24) | (work->hair_rgb[1] << 16) | (work->hair_rgb[2] << 8) | 0xFF;
    user->face_type_0x39A0 = face_type_get(work->sex);
    user->face_eye_0x39A1 = face_eye_get(work->sex);
    user->makeup_0x3DE6 = work->makeup;
    user->makeup_color_0x3DE7 = work->makeup_color;
    slot = system_w.vs_load_slot_0x7e9 + system_w.vs_load_player_0x7e8 * 3;
    user->seed_0x53A8 = (slot << 24) | ran_suu(1);
}

/* 0x8044468C (0x340): steps the value of the item rows with right/left (equipment, makeup, face, makeup colour,
 * hair, voice); 1 when the pad was taken. */
static u32 charmake_value_step(CharmakeWork* work) {
    PlayerPad* pad;
    u16 pressed;

    pad = Psw;
    pressed = pad->button_0x2C0.pressed_0x04;
    if (work->hold > 0) {
        work->hold--;
        return 0;
    }
    if (pressed & 2) {
        switch (work->item) {
        case 4:
            if (work->equip < 3) {
                work->equip++;
                charmake_equip_apply(work);
                sysSE_req(3);
            }
            return 1;
        case 7:
            if (work->makeup < 9) {
                work->makeup++;
                face_makeup_set(work->sex, work->makeup, work->makeup_color);
                sysSE_req(3);
            }
            return 1;
        case 6:
            if (work->face < charmake_face_count[work->sex] - 1) {
                work->face++;
                charmake_face_apply(work);
                sysSE_req(3);
            }
            return 1;
        case 8:
            if (work->makeup_color < 12) {
                work->makeup_color++;
                face_makeup_set(work->sex, work->makeup, work->makeup_color);
                sysSE_req(3);
            }
            return 1;
        case 9:
            if (work->hair < charmake_hair_count[work->sex] - 1) {
                work->hair++;
                charmake_hair_apply(work);
                sysSE_req(3);
            }
            return 1;
        case 11:
            if (work->voice < 19) {
                work->voice++;
                sysSE_req(3);
            }
            return 1;
        }
    }
    if (pressed & 1) {
        switch (work->item) {
        case 4:
            if (work->equip > 0) {
                work->equip--;
                charmake_equip_apply(work);
                sysSE_req(3);
            }
            return 1;
        case 7:
            if (work->makeup != 0) {
                work->makeup--;
                face_makeup_set(work->sex, work->makeup, work->makeup_color);
                sysSE_req(3);
            }
            return 1;
        case 6:
            if (work->face > 0) {
                work->face--;
                charmake_face_apply(work);
                sysSE_req(3);
            }
            return 1;
        case 8:
            if (work->makeup_color > 0) {
                work->makeup_color--;
                face_makeup_set(work->sex, work->makeup, work->makeup_color);
                sysSE_req(3);
            }
            return 1;
        case 9:
            if (work->hair > 0) {
                work->hair--;
                charmake_hair_apply(work);
                sysSE_req(3);
            }
            return 1;
        case 11:
            if (work->voice > 0) {
                work->voice--;
                sysSE_req(3);
            }
            return 1;
        }
    }
    return 0;
}

/* Steps the colour palette of `kind` (1 the inner wear at `rgb`/`color`, 0 the hair): the cells, the custom box. */
static inline void charmake_palette_step(CharmakeWork* work, u16 pressed, u16 buttons, u8 kind, u8* rgb, u32* color) {
    if (work->palette_custom == 0) {
        if (pressed & 0x20) {
            sysSE_req(1);
            work->step--;
        } else if (pressed & 8) {
            sysSE_req(3);
            if (++work->palette_cursor > 12) {
                work->palette_cursor = 0;
            }
        } else if (pressed & 4) {
            sysSE_req(3);
            if (--work->palette_cursor < 0) {
                work->palette_cursor = 12;
            }
        } else if (pressed & 1) {
            sysSE_req(3);
            if ((u8)work->palette_cursor <= 2) {
                work->palette_cursor = 12;
            } else if (work->palette_cursor != 12) {
                work->palette_cursor -= 3;
            } else {
                work->palette_cursor = 11;
            }
        } else if (pressed & 2) {
            sysSE_req(3);
            if ((u8)(work->palette_cursor - 9) <= 2) {
                work->palette_cursor = 12;
            } else if (work->palette_cursor != 12) {
                work->palette_cursor += 3;
            } else {
                work->palette_cursor = 0;
            }
        } else if (pressed & 0x10) {
            titleSE_req(4);
            if (work->palette_cursor < 12) {
                charmake_color_pick(kind, work->palette_cursor, color, &work->custom_x, &work->custom_y);
                rgb[0] = (*color & 0xFF000000) >> 24;
                rgb[1] = (*color & 0xFF0000) >> 16;
                rgb[2] = (*color & 0xFF00) >> 8;
            } else {
                work->palette_custom = 1;
            }
        }
    } else if (pressed & 0x20) {
        sysSE_req(1);
        work->palette_custom = 0;
    } else if (lb_cursor_box_step(&work->custom_x, &work->custom_y, 1, 199, 1, 87, buttons) == 1) {
        charmake_func.set(kind ^ 1, rgb);
        *color = (rgb[0] << 24) | (rgb[1] << 16) | (rgb[2] << 8) | 0xFF;
    }
}

/* 0x804449CC (0xF78): the character make's frame: set-up, the item menu, the name keyboard, the item editors, the
 * confirmation and the ending; returns -1 without a work, -3 when cancelled, -2 when finished, else the screen. */
s32 charmake_move(void) {
    CharmakeWork* work;
    PlayerPad* pad;
    u16 pressed;
    u16 held;
    u16 buttons;
    s32 ret;
    s32 key;
    _PLW* plw;

    work = charmake_work;
    pad = Psw;
    pressed = pad->button_0x2C0.pressed_0x04;
    held = pad->button_0x2C0.hold_0x00;
    buttons = pad->button_0x2C0.press_0x08;
    if (work == NULL) {
        return -1;
    }
    ret = 0;
    switch (work->step) {
    case 0:
        ret = 1;
        work->step++;
        charmake_camera_init(work);
        title_se_load();
        break;
    case 1:
        ret = 1;
        if (file_loading_ck(NULL, NULL) != 1) {
            pl_motion_set();
            ((StageMapView*)stage_w)->mapno = 0xFF;
            ((StageMapView*)stage_w)->areano = 0;
            filter_reset();
            light_init();
            charmake_light_set();
            work->name[0] = 0;
            work->step++;
        }
        break;
    case 2:
        ret = 1;
        if (player_move_start_ck() == 1) {
            player_move_start(-1);
            work->step++;
            work->sub = 0;
            GlareFilter_on();
            work->item = 1;
            charmake_face_select(work->sex);
            get_move_work_adrs(2);
            charmake_equip_apply(work);
            charmake_face_apply(work);
            charmake_hair_apply(work);
        }
        break;
    case 3:
        switch (work->sub) {
        case 0:
            if (charmake_item_step(work) == 1) {
                break;
            }
            if (pressed & 0x20) {
                if (work->item != 12) {
                    work->item = 12;
                    sysSE_req(1);
                } else {
                    work->sub++;
                    work->confirm = 1;
                    sysSE_req(1);
                }
            } else if ((pressed & 0x10) && work->item >= 1) {
                if (work->item == 11) {
                    work->step++;
                    plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[work->sex];
                    plw->se_name_idx = work->voice;
                    pl_voice_bank_load(plw);
                    work->voice_sample = 0;
                } else if (work->item == 12) {
                    work->sub = 2;
                    work->sub2 = 0;
                    work->confirm = 0;
                } else {
                    if (work->item == 1) {
                        kbd_open(3);
                        strcpy(charmake_name_buffer, work->name);
                        set_kbd_param(charmake_name_buffer, 16);
                    }
                    work->step++;
                    work->sub = 0;
                }
                sysSE_req(0);
                work->palette_cursor = 0;
                work->palette_custom = 0;
                work->custom_x = 100;
                work->custom_y = 45;
                work->left_arrow = -1;
                work->right_arrow = -1;
            } else {
                charmake_model_update(work);
            }
            break;
        case 1:
            switch (work->sub2) {
            case 0:
                if (work->confirm == 0) {
                    if (pressed & 0x10) {
                        sysSE_req(0);
                        fade_set(0, 1);
                        work->timer = 32;
                        work->sub2++;
                    } else if (pressed & 8) {
                        work->confirm = 1;
                        sysSE_req(3);
                    } else if (pressed & 0x20) {
                        work->confirm = 1;
                        sysSE_req(1);
                    }
                } else if (pressed & 0x10) {
                    work->sub = 0;
                    work->sub2 = 0;
                    sysSE_req(1);
                } else if (pressed & 4) {
                    work->confirm = 0;
                    sysSE_req(3);
                }
                break;
            case 1:
                if (--work->timer <= 0) {
                    work_mem_free(charmake_work);
                    return -3;
                }
                break;
            }
            break;
        case 2:
            if (work->confirm == 0) {
                if (pressed & 0x10) {
                    if (strlen(work->name) != 0) {
                        sysSE_req(0);
                        work->step++;
                        charmake_userdata_store(work);
                        work->sub = 0;
                        work->sub2 = 0;
                        ret = 1;
                    } else {
                        sysSE_req(2);
                    }
                } else if (pressed & 8) {
                    work->confirm = 1;
                    sysSE_req(3);
                } else if (pressed & 0x20) {
                    work->confirm = 1;
                    sysSE_req(1);
                }
            } else if (pressed & 0x10) {
                work->sub = 0;
                work->sub2 = 0;
                sysSE_req(1);
            } else if (pressed & 4) {
                work->confirm = 0;
                sysSE_req(3);
            }
            break;
        }
        break;
    case 4:
        switch (work->item) {
        case 0:
            work->step--;
            break;
        case 1:
            switch (work->sub) {
            case 0:
                key = kbd_move();
                if (key != 0) {
                    if (key == 1) {
                        if (charmake_func.name_ok(charmake_name_buffer) == 1) {
                            strcpy(work->name, charmake_name_buffer);
                            work->step--;
                        } else {
                            work->sub = 1;
                        }
                    } else {
                        work->step--;
                    }
                }
                break;
            case 1:
                if (pressed & 0x10) {
                    sysSE_req(0);
                    work->sub = 0;
                }
                break;
            }
            break;
        case 2:
            if (work->left_arrow >= 0) {
                if (++work->left_arrow > 30) {
                    work->left_arrow = -1;
                }
            }
            if (work->right_arrow >= 0) {
                if (++work->right_arrow > 30) {
                    work->right_arrow = -1;
                }
            }
            if (charmake_model_ready_ck(work) != 0) {
                if ((pressed & 8) || (pressed & 4)) {
                    sysSE_req(3);
                    work->sex ^= 1;
                    charmake_face_load(work->sex, work->sex);
                    charmake_face_select(work->sex);
                    get_move_work_adrs(2);
                    charmake_equip_apply(work);
                    charmake_face_apply(work);
                    charmake_hair_apply(work);
                    face_makeup_set(work->sex, work->makeup, work->makeup_color);
                    if (pressed & 4) {
                        work->left_arrow = 0;
                    } else {
                        work->right_arrow = 0;
                    }
                } else if (pressed & 0x20) {
                    sysSE_req(1);
                    work->step--;
                }
            }
            break;
        case 4:
            if (charmake_model_ready_ck(work) != 0 && charmake_value_step(work) != 1 && (pressed & 0x20)) {
                sysSE_req(1);
                work->step--;
            }
            break;
        case 5:
            charmake_palette_step(work, pressed, buttons, 1, work->inner_rgb, &work->inner_color);
            break;
        case 6:
            if (charmake_model_ready_ck(work) != 0 && charmake_value_step(work) != 1 && (pressed & 0x20)) {
                sysSE_req(1);
                work->step--;
            }
            break;
        case 3:
            if (pressed & 0x20) {
                sysSE_req(1);
                work->step--;
            } else {
                if (held & 8) {
                    face_type_next(0);
                    face_type_next(1);
                }
                if (held & 4) {
                    face_type_prev(0);
                    face_type_prev(1);
                }
            }
            break;
        case 8:
            if (charmake_value_step(work) != 1 && (pressed & 0x20)) {
                sysSE_req(1);
                work->step--;
            }
            break;
        case 7:
            if (charmake_value_step(work) != 1 && (pressed & 0x20)) {
                sysSE_req(1);
                work->step--;
            }
            break;
        case 9:
            if (charmake_model_ready_ck(work) != 0 && charmake_value_step(work) != 1 && (pressed & 0x20)) {
                sysSE_req(1);
                work->step--;
            }
            break;
        case 10:
            charmake_palette_step(work, pressed, buttons, 0, work->hair_rgb, &work->hair_color);
            break;
        case 11:
            if (file_loading_ck(NULL, NULL) != 1) {
                plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[work->sex];
                if (charmake_value_step(work) == 1) {
                    if (plw->se_name_idx != work->voice) {
                        if (work->voice_handle != 0 && snd_stream_reloc_busy_ck(work->voice_handle) != 0) {
                            snd_stream_reloc_stop(work->voice_handle);
                            work->voice_handle = 0;
                        }
                        plw->se_name_idx = work->voice;
                        pl_voice_bank_load(plw);
                        work->voice_sample = 0;
                    }
                } else if (pressed & 0x20) {
                    work->step--;
                    sysSE_req(1);
                } else if (pressed & 0x10) {
                    work->voice_handle = se_bank_req(work->sex + 0x14, work->voice_sample);
                    work->voice_sample++;
                    if (work->voice_sample >= 12) {
                        work->voice_sample = 0;
                    }
                }
            }
            break;
        case 12:
            switch (work->sub2) {
            case 0:
                ret = 2;
                break;
            case 1:
                system_w.field_0x890 = 0;
                charmake_userdata_store(work);
                ret = 3;
                titleSE_req(5);
                PlayStream(1, 1);
                work->camera = 0;
                charmake_camera_goal_set(work, 2);
                charmake_finish_pose(work);
                work->sub2++;
                work->timer = 0;
                break;
            case 2:
                if (++work->timer > 90) {
                    work->step = 100;
                    ret = -2;
                } else {
                    ret = 3;
                }
                break;
            }
            break;
        }
        if (work->item != 12) {
            charmake_model_update(work);
        }
        break;
    case 10:
        work->step = 2;
        break;
    case 11:
        work->step++;
        break;
    case 12:
        charmake_face_select(1);
        charmake_face_load(1, 1);
        work->step++;
        break;
    case 13:
        charmake_face_select(work->sex);
        charmake_face_load(work->sex, work->sex);
        work->step = 2;
        break;
    }
    nwMoveStart();
    prim_init_all();
    player_control_move();
    yure_move();
    charmake_camera_step(work);
    light_move();
    ((nw4r::g3d::ScnRoot*)pRoot)->SetCurrentCamera(0);
    nwMoveEnd();
    return ret;
}

/* 0x80445944 (0x1C): applies the demo equipment set of `plw`'s slot to it. */
void set_demo_equip_data(_PLW* plw) {
    arena_userdata_apply((u8*)&demoEquip[plw->chunk_ofs], (u8*)plw);
}
