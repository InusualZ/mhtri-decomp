/*
 * menu/movie.cpp - movie playback (`Movie_open`, `Movie_draw_sub`, `Movie_close`, `Check_movie_finish`, `Movie_cut`)
 *   over a copy of the SDK's THPSimple player and THPDraw helpers, `NetworkFriendInfo`'s constructor and the
 *   disc-error screen (`movie_drive_wait`).
 * RANGE. .text 0x8043D524-0x8043F2FC (49 functions); extab 0x8001DBEC-0x8001DCC4, extabindex 0x8003E718-0x8003E85C,
 *   .data 0x806047D0-0x806048B0, .bss 0x806E26E0-0x806E28B0, .sdata 0x80793A80-0x80793AF0, .sdata2
 *   0x8079C8D0-0x8079C900.  The left edge is the `movie.cpp` `__FILE__` anchor (one copy in the DOL); the right edge
 *   is `menu/menu_friendlist.cpp` (the `.data` V->S seam at 0x806048B0, `.sdata`'s second "%s" at 0x80793AF0,
 *   `.sdata2`'s re-pooled 0.0f/1.0f at 0x8079C900, and the `extabindex` record of 0x8043F314 at 0x8003E85C).
 * FLAGS. `cflags_menu` (configure.py); file-scope `#pragma pool_data off` (MWCC otherwise addresses the strings and
 *   tables through one pooled base).
 * NAMES. Module `menu` and the file from the `__FILE__` string "movie.cpp".  The THPSimple/THPDraw functions carry the
 *   SDK sample's names (the panic strings "THPSimpleOpen fail"/"THPSimplePreLoad fail" and the map's
 *   `THPSimpleGetTotalFrame` name the copy); its record layout is the sample's minus the audio fields.
 *   GUESS (from each body and its callers): `movie_drive_wait`, `movie_change_request`, `movie_frame_draw`,
 *   `movie_playing_ck`, `movie_work`, `movie_table`, `dvd_error_*`, `dvd_error`, `system_shutdown_prepare`,
 *   `thp_gx_*`, the types `MovieWork`, `DvdError`, and every foreign name this file spells from the name registry.
 * RESIDUALS.
 *  - `NetworkFriendInfo`'s inline destructor is emitted here as a weak function (retail's table points at the copy
 *    `Network/NetworkCommunityPat.cpp` emits), so `.text` is 0x44 bytes longer than retail's.
 *  - `THPSimpleOpen`: the header's component offset takes r29 where retail splits it (r30, then r29 after `+= 0x14`).
 *  - `VideoDecode`: `THP/THPDec.h` returns `u8`, so the result gets a `clrlwi` retail lacks (request menu-movie#2).
 *  - `THPSimpleCalcNeedMemory`, `THPSimpleSetBuffer`: `mullw` takes the size operands in the other order; the SDK's
 *    `xSize * ySize` swaps the loads as well and scores lower than `ySize * xSize`.
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
