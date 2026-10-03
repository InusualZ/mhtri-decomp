/*
 * Declarations of `src/lobby/lb_server_sel_trans.cpp` (`.text` 0x803C3A5C..0x803CCDF8): the demo/event work accessors and the
 * server-selection transition band.  Moved here from `include/menu/get_pop_dat_ptr.h` (`event_demo_ck`) and from the units that
 * declared them locally when the phase 4 recut moved the addresses out of `menu/get_pop_dat_ptr`.
 *
 * Linkage follows the map name: `event_demo_ck__Fv` is a C++ free function (declared at C++ scope, rule 9); the
 * plain-named symbols are `extern "C"`.
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

/* Declarations moved here from `include/unsplit/NetworkStream.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct NetworkStreamWriter;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803CB9B4 - builds an empty packet (the `NetworkBuffer` base, then its own table). */
void networkPacket_construct(NetworkStreamWriter* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SERVER_SEL_TRANS_H */
