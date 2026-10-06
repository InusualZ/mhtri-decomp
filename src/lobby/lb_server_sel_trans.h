/*
 * Declarations of `lobby/lb_server_sel_trans.cpp`: the demo/event work accessors and the server-selection transition
 * band.  Linkage follows the map: `event_demo_ck__Fv` is C++ (rule 9), the plain-named symbols are `extern "C"`.
 */
#ifndef MHTRI_LOBBY_LB_SERVER_SEL_TRANS_H
#define MHTRI_LOBBY_LB_SERVER_SEL_TRANS_H

#include "types.h"

struct DemoWork;

/* 0x803C4814 - non-zero while the demo work is playing the demo the game asks about. */
u32 event_demo_ck(void);

#ifdef __cplusplus
extern "C" {
#endif

/* The demo work's per-frame pass (0x803C4BA0..; unwritten). */
void demo_work_process(struct DemoWork* work);

/* 0x803C7EAC / 0x803C7F88 - two entry points of the transition band the enemy note-pane code calls (unwritten; names are the
 * map's placeholders). */
void fn_803C7EAC(void);
void fn_803C7F88(void);

/* 0x803C8454 - opens NPC `npc`'s talk message `msg` (a formatted string when `arg` is 0) in window kind `kind`; 0
 * while a demo runs or the talk cannot open.  GUESS name. */
s32 npc_talk_start(s8 npc, u8 msg, u16 arg, u8 kind);
/* 0x803C8660 / 0x803C8690 / 0x803C86B4 - whether the talk window is open, closes it, and sets its flag byte.  GUESS
 * names. */
u8 npc_talk_active_ck(void);
void npc_talk_end(void);
void npc_talk_flag_set(u8 flag);
/* 0x803C8670 / 0x803C8680 / 0x803C86A4 - the talk window's speaker, choice flag and message index (GUESS names). */
u8 talk_msg_speaker_get(void);
u8 talk_msg_choice_ck(void);
u8 talk_msg_index_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SERVER_SEL_TRANS_H */
