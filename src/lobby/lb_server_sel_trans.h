/*
 * Declarations of `lobby/lb_server_sel_trans.cpp`: the demo/event work and its accessors, and the server-selection
 * transition band.  Linkage follows the map: `event_demo_ck__Fv` is C++ (rule 9), the plain-named symbols are
 * `extern "C"`.
 */
#ifndef MHTRI_LOBBY_LB_SERVER_SEL_TRANS_H
#define MHTRI_LOBBY_LB_SERVER_SEL_TRANS_H

#include "types.h"

/* One demo the stage can play: the `.sdata` records `demo_work_start` and `demo_work_process` hand to the slots.
 * size: 0x8 */
typedef struct DemoDef {
    /* +0x0 */ u8 area;          /* the stage area the demo plays in (`get_now_areano`) */
    /* +0x1 */ u8 pad_0x1;
    /* +0x2 */ u16 progress_flag; /* the user-data progress flag the demo raises once seen (0: always plays) */
    /* +0x4 */ u8 demo_no;       /* the demo id (`stage_dcm_buffer_tbl` slot, `emc_event_set` event) */
    /* +0x5 */ u8 pad_0x5[0x3];
} DemoDef; /* size: 0x8 */

/* One of the demo work's three pending demos.  size: 0x8 */
typedef struct DemoSlot {
    /* +0x0 */ u8 active;          /* set while the slot holds a demo */
    /* +0x1 */ u8 step;            /* `demo_slot_play`'s step */
    /* +0x2 */ u8 pad_0x2[0x2];
    /* +0x4 */ const DemoDef* def; /* the demo the slot plays */
} DemoSlot; /* size: 0x8 */

/* The demo/event work `demo_work` (.bss 0x806D2B20).  size: 0x24 */
typedef struct DemoWork {
    /* +0x00 */ u8 state_0x00;   /* 0 idle, 1 waiting, 2 BGM loading, 3 BGM start, 4 the demo the game asks about */
    /* +0x01 */ u8 slot;         /* the slot `demo_sound_update` plays */
    /* +0x02 */ u8 demo_no_0x02; /* `get_demo_no`, 0xFF = none */
    /* +0x03 */ u8 variant;      /* the variant `demo_work_process` picks for the queued demo */
    /* +0x04 */ u32 field_0x04;  /* the demo frame (`get_demo_data`) */
    /* +0x08 */ u8 running;      /* `demo_set_running` */
    /* +0x09 */ s8 bgm_status;   /* `demo_bgm_load`'s result for the playing demo */
    /* +0x0A */ u8 queued;       /* set once `demo_slot_queue` filled a slot this pass */
    /* +0x0B */ u8 pad_0x0B;
    /* +0x0C */ DemoSlot slots[3];
} DemoWork; /* size: 0x24 */

/* 0x803C4814 - non-zero while the demo work is playing the demo the game asks about. */
u32 event_demo_ck(void);
/* 0x803C4850 - the number of the demo the demo work is playing (its byte at +0x02). */
u8 get_demo_no(void);

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803C4840 - the demo frame. */
u32 get_demo_data(void);

/* 0x803C3D74 - restarts the demo work and adds demo `demo`'s record (14, 19 or 20) to the first slot. */
void demo_work_start_ex(s32 demo);
/* 0x803C4B74 - initialises the demo work for a fresh play-through. */
void demo_work_init(void);

/* 0x803C4210 - queues the demos of the current map's area whose progress flags are set. */
void demo_work_process(DemoWork* work);

/* 0x803C7EAC / 0x803C7F88 - clears the talk window, and runs its frame step (the enemy note-pane code calls both).
 * GUESS names. */
void npc_talk_init(void);
void npc_talk_update(void);

/* 0x803C8454 - opens NPC `npc`'s talk message `msg` (a formatted string when `arg` is 0) in window kind `kind`; 0
 * while a demo runs or the talk cannot open.  GUESS name. */
s32 npc_talk_start(s8 npc, u8 msg, u16 arg, u8 kind);
/* 0x803C8660 / 0x803C8690 / 0x803C86B4 - whether the talk window is open, closes it, and sets its flag byte.  GUESS
 * names. */
u8 npc_talk_active_ck(void);
void npc_talk_end(void);
void npc_talk_flag_set(u8 flag);
/* 0x803C8670 / 0x803C8680 / 0x803C86A4 - the talk window's speaker, choice flag and message index (GUESS names). */
s8 talk_msg_speaker_get(void);
u8 talk_msg_choice_ck(void);
u8 talk_msg_index_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SERVER_SEL_TRANS_H */
