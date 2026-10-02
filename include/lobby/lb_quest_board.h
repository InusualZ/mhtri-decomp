/*
 * This unit's own views of the lobby records `src/lobby/lb_quest_board.cpp` reads, plus the
 * declarations of the symbols of OTHER units it calls (docs/plan.md 6.5, rules 1/2/9).
 *
 * `lobby_w` (`LbQuestBoardLobby`): the `.bss` lobby work block is 0x17C bytes at 0x806AAB44 and
 * every lobby unit needs a different set of its bytes.  The band's own convention, already carried
 * by `include/lobby/lb_npc.h`, `include/lobby/fn_8020C588.h`, `include/lobby/fn_8021E1EC.h`,
 * `include/lobby/fn_802FA9A0.h` and `include/unsplit/lobby.h`, is one view per unit - the views
 * genuinely disagree (different byte is the "state" to each), so no single header can hold them and
 * this one keeps only the offsets the quest-board screen reads.
 */
#ifndef MHTRI_LOBBY_LB_QUEST_BOARD_H
#define MHTRI_LOBBY_LB_QUEST_BOARD_H

#include "types.h"

namespace nw4r {
namespace ef {
class Effect;
}  // namespace ef
}  // namespace nw4r

/* The pooled-effect list a screen block keeps at `+0x038`: a count followed by the array of effect
 * handles the effect library retires.  Only the two members the band touches are named.
 * size: 0x8 (lower bound) */
typedef struct LbEftList {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ nw4r::ef::Effect* effects_0x004[1];
} LbEftList;

/* The record `LbQuestBoardWork::data_0x264` points at: the screen's payload buffer (the block's
 * whole body is memset before it is filled, so only the offsets the band touches are named).
 * size: 0xB03 (lower bound) */
typedef struct LbQuestBoardData {
    /* +0x000 */ u8 pad_0x000[0x14];
    /* +0x014 */ u8 field_0x014;
    /* +0x015 */ u8 pad_0x015[0xAEC];
    /* +0xB01 */ u8 flags_0xB01;
    /* +0xB02 */ u8 param_0xB02;
} LbQuestBoardData;

/* The quest-board screen work block reachable through `lobby_w.menu_0xAC` - the object the band
 * memsets whole (0x2000 bytes) and the update/draw steps take as their `this`.
 * size: 0x2000 */
typedef struct LbQuestBoardWork {
    /* +0x000 */ u8 unused_0x000[2];
    /* +0x002 */ u8 kind_0x002;      /* 8 / 9 select the effect updater (`fn_803963D0`) */
    /* +0x003 */ u8 unused_0x003[2];
    /* +0x005 */ u8 state_0x005;     /* the screen step the two dispatchers switch on */
    /* +0x006 */ u8 unused_0x006[6];
    /* +0x00C */ u32 field_0x00C;
    /* +0x010 */ u8 unused_0x010[0x28];
    /* +0x038 */ LbEftList* eft_0x038;
    /* +0x03C */ u8 unused_0x03C[0x20D];
    /* +0x249 */ s8 index_0x249;     /* table index the label lookup takes */
    /* +0x24A */ u8 unused_0x24A[4];
    /* +0x24E */ u16 cursor_0x24E;
    /* +0x250 */ u16 count_0x250;
    /* +0x252 */ u8 unused_0x252[6];
    /* +0x258 */ u32 field_0x258;
    /* +0x25C */ u8 unused_0x25C[8];
    /* +0x264 */ LbQuestBoardData* data_0x264;  /* the payload the screen was opened with */
    /* +0x268 */ u8 unused_0x268[0x124];
    /* +0x38C */ u16 value_0x38C;
} LbQuestBoardWork;

/* The `.bss` lobby work block (`lobby_w`, 0x806AAB44, 0x17C B in the map) as this unit reads it:
 * the screen selector, the screen work block and the two bytes the open/close helpers toggle. */
typedef struct LbQuestBoardLobby {
    /* +0x000 */ u8 state_0x000;
    /* +0x001 */ u8 unused_0x001[0x007];
    /* +0x008 */ u8 active_0x008;
    /* +0x009 */ u8 unused_0x009[0x049];
    /* +0x052 */ u8 flags_0x052;
    /* +0x053 */ u8 unused_0x053[0x2F];
    /* +0x082 */ u16 value_0x082;
    /* +0x084 */ u8 unused_0x084[0x028];
    /* +0x0AC */ LbQuestBoardWork* menu_0xAC;
} LbQuestBoardLobby; /* size: 0x17C */

extern "C" {
extern LbQuestBoardLobby lobby_w;   /* .bss 0x806AAB44 */

/* Other units' still-unrenamed `fn_XXXXXXXX` symbols.  rule 7 deferred: these are references to
 * OTHER units' unrenamed symbols only (checked with `symedit.py range` over the two bands that
 * bracket this one, 0x8038E8E8 and 0x803D3CE8, both registered as `enemy`/`Network` placeholders
 * whose `fn_` names the map still carries). */
void fn_8004DF10(void* work, u32* out_a, u32* out_b);
void sysSE_stop(u32 id);
void eft_res_slot_release(void* work);
void fn_800F8A44(void* list, s32 count);
void fn_80214EF0(u32 panel, u32 value);
void fn_80215170(u32 panel, u32 value);
void fn_803772A8(u8 a, u8 b);
void* fn_80395DF4(LbQuestBoardWork* work, u32 kind);
u32 fn_803B7154(struct QuestRecord* value, u16 param, u8 arg, LbQuestBoardData* data);
u16* fn_804338E0(s32 index);
}

/* Mangled callees, declared through their owner (rule 9); each spelling was confirmed with
 * `tools/units/mangle.py` against the map's own symbol. */
void set_zmode(bool depth_test, u8 arg1, bool arg2);      /* set_zmode__FbUcb */
void set_blendmode(u8 src, u8 dst, u8 op);                /* set_blendmode__FUcUcUc */
void sysSE_req(long id);                                  /* sysSE_req__Fl */

/* 0x80395C84 - resets the quest board: the prototype is the leaf header's (`lb_quest_board_reset` has C linkage). */
#include "lobby/lb_quest_board_reset.h"

#endif /* MHTRI_LOBBY_LB_QUEST_BOARD_H */
