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

#ifdef __cplusplus
}
#endif

/* The writer classes' constructors and destructors this unit defines (0x803CB8FC..0x803CBA2C) are declared
   on their classes in `Network/network_writer_types.h`. */

#endif /* MHTRI_LOBBY_LB_SERVER_SEL_TRANS_H */
