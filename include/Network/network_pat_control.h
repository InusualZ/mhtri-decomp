/*
 * include/Network/network_pat_control.h - shared views for the network pat-control band
 * (`Network/network_pat_control.cpp`, `.text` 0x80429B94..0x8043065C).
 *
 * The `net_ctrl_wk` singleton (map: `.sbss:0x80794CF8`, a 4-byte pointer) is the record every
 * function in the band dereferences; its owner TU is unclaimed, so the extern lives here beside the
 * one unit that reads it today (docs/plan.md 6.5 rule 2's unsplit-address gap).  Only the offsets
 * this unit reads are named; every other byte stays `pad_0xNNN`.
 *
 * `NetCtrlWk` size: 0xC3F3 approximate - the highest offset any function in the band touches is
 * +0xC3F2; the record is larger than anything this unit proves.
 *
 * The four-slot "*Pat" holder and the accessor family that walks it live in their owner's header,
 * `Network/NetworkPat.h`, which this file includes (rule 1/2) - the family is declared once, by the
 * unit that defines it.
 */
#ifndef MHTRI_NETWORK_NETWORK_PAT_CONTROL_H
#define MHTRI_NETWORK_NETWORK_PAT_CONTROL_H

#include "types.h"
#include "Network/NetworkPat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One slot of the 16-entry server/message table at `NetCtrlWk::entries_0x7CD8` (stride 0x5C). */
typedef struct NetCtrlEntry {
    /* +0x00 */ u8 in_use_0x00;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u8 kind_0x02;
    /* +0x03 */ u8 flags_0x03;
    /* +0x04 */ char name_0x04[0x48];
    /* +0x4C */ u32 value_0x4C;
    /* +0x50 */ char text_0x50[0xC];
} NetCtrlEntry; /* size: 0x5C */

/* One slot of the 100-record `slots_0x3ED4` table (map `lbl_80794CF8` band).  The first byte is the
 * slot's state (0 = free), +0x04 is the owner pointer the state machine hands out, +0x3C/+0x40 the two
 * float members and +0x54 the slot's mode word. */
typedef struct NetSlot {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ void* owner_0x04;   /* untyped: opaque handle - the slot hands its owner back to the caller */
    /* +0x08 */ u8 pad_0x08[0x34];
    /* +0x3C */ f32 valueA_0x3C;
    /* +0x40 */ f32 valueB_0x40;
    /* +0x44 */ u8 pad_0x44[0x10];
    /* +0x54 */ u32 mode_0x54;
} NetSlot; /* size: 0x58 */

/* One record of the 128-entry `pool_0x6320` array (stride 0x20).  `in_use_0x08` marks a handed-out
 * record; the first record also carries a live count at +0x04. */
typedef struct NetPoolEntry {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 count_0x04;
    /* +0x08 */ u32 in_use_0x08;
    /* +0x0C */ u8 pad_0x0C[0x14];
} NetPoolEntry; /* size: 0x20 */

/* The network-control work record reached through `net_ctrl_wk`.  `entries_0x7CD8` is 16 records
 * of 0x5C; the tail offsets (+0x82C4, +0xBF2C, +0xC3F2, ...) are separate flags/counters. */
typedef struct NetCtrlWk {
    /* +0x000 */ u8 pad_0x000[0x10];
    /* +0x010 */ u8 mode_0x010;
    /* +0x011 */ u8 state_0x011;
    /* +0x012 */ u8 pad_0x012[0x5];
    /* +0x017 */ u8 sub_state_0x017;
    /* +0x018 */ u8 pad_0x018[0x28];
    /* +0x040 */ u8 server_slot_state_0x040[4];
    /* +0x044 */ u32 ready_count_0x044;
    /* +0x048 */ u8 pad_0x048[0x22];
    /* +0x06A */ u8 selected_server_0x06A;
    /* +0x06B */ u8 pad_0x06B[0x9];
    /* +0x074 */ s8 server_index_0x074[4];
    /* +0x078 */ u8 pad_0x078[0x3E58];
    /* +0x3ED0 */ NetSlot* slot_list_0x3ED0;
    /* +0x3ED4 */ NetSlot slots_0x3ED4[100];
    /* +0x6134 */ u8 pad_0x6134[0x1EC];
    /* +0x6320 */ NetPoolEntry pool_0x6320[128];
    /* +0x7320 */ u8 pad_0x7320[0x48];
    /* +0x7368 */ char name_0x7368[0xA];
    /* +0x7372 */ char name2_0x7372[0xA];
    /* +0x737C */ u8 pad_0x737C[0x10C];
    /* +0x7488 */ u8 msgTable_0x7488[0x4A0];
    /* +0x7928 */ u8 pad_0x7928[0x60];
    /* +0x7988 */ u8 used_0x7988[4];
    /* +0x798C */ u8 pad_0x798C[0xA];
    /* +0x7996 */ u32 slots_0x7996[0x40];
    /* +0x7A96 */ u8 pad_0x7A96[0x2];
    /* +0x7A98 */ u32* arrA_0x7A98[0x40];
    /* +0x7B98 */ u32 arrB_0x7B98[0x40];
    /* +0x7C98 */ u8 arrC_0x7C98[0x40];
    /* +0x7CD8 */ NetCtrlEntry entries_0x7CD8[16];
    /* +0x8298 */ u8 pad_0x8298[0x2C];
    /* +0x82C4 */ u8 flag_0x82C4;
    /* +0x82C5 */ u8 flag_0x82C5;
    /* +0x82C6 */ u8 pad_0x82C6;
    /* +0x82C7 */ u8 flag_0x82C7;
    /* +0x82C8 */ u8 pad_0x82C8[0x3C64];
    /* +0xBF2C */ u8 flag_0xBF2C;
    /* +0xBF2D */ u8 pad_0xBF2D[0x3D3];
    /* +0xC300 */ s32 msg_state_0xC300;
    /* +0xC304 */ u8 pad_0xC304[0x5C];
    /* +0xC360 */ u16 counter_0xC360;
    /* +0xC362 */ u16 counter_0xC362;
    /* +0xC364 */ u32 counter_0xC364;
    /* +0xC368 */ u8 pad_0xC368[0x8A];
    /* +0xC3F2 */ u8 flag_0xC3F2;
    /* +0xC3F3 */ u8 tail_0xC3F3[];
} NetCtrlWk; /* size: 0xC3F3 (approximate, see above) */

/* The work-record singleton, defined by another (unclaimed) TU. */
extern NetCtrlWk* net_ctrl_wk;

/* 0x8042C844 - the selected server id; 0x8042C850 - how many of the four server slots are occupied
 * (0 with no control record); 0x8042ED30 - the sub-state result byte at +0xBF2D (0 below state 0x32);
 * 0x8042EDE0 - clears the result flag at +0xC259; 0x8042FFDC - copies the 0x2000-byte quest record into
 * the control record at +0xC3EC.  Added with `quest/arenatask.cpp` (rule 2: this range owns the
 * addresses). */
u8 net_selected_server_get(void);
s8 net_occupied_slot_count(void);
u8 net_sub_result_get(void);
void net_result_flag_clear(void);
void net_quest_record_set(const void* record /* untyped: byte range - the 0x2000-byte record it copies */);

/* The network facade the band drives. */
/* 0x8042CC20 - no arguments; the sibling predicate of `fn_8042CB9C`, reached by
 * `ef/eft_slot.cpp`.  Added here because this unit owns the address (rule 2). */
BOOL fn_8042CC20(void);
/* The holder `getPatsObject` returns is `NetworkPat` (include/Network/NetworkPat.h).  The accessor
 * itself is at 0x803DA020, inside the unclaimed 0x803D70B8-0x803DDB64 Pat band, so no registered unit
 * owns it yet and its declaration sits here with the one unit that calls it. */
NetworkPat* getPatsObject(void);
/* `fn_803DF1A8` (0x803DF1A8) is the session manager's readiness probe: it is handed the object slot
 * +0x00 holds, so its parameter is that object's class. */
BOOL fn_803DF1A8(NetworkSessionManagerPat* session_manager);

#ifdef __cplusplus
}
#endif

/* Mangled callees are declared with their real signatures, never with the map's mangling
 * (docs/plan.md 6.5 rule 9): the C++ front-end emits the map's name itself.  `sysSE_req` (`__Fl`) has
 * no registered owner; `flfntStrLen` (`__FPc`, 0x8005B874) is owned by `g3d/g3d_anmchr.cpp`, so it is
 * declared in that unit's header (`g3d/g3d_anmchr.h`) and included where it is called (rule 2) -
 * `g3d/g3d_anmchr.h` and this header declare no other symbol in common, so a TU may include both. */
void sysSE_req(long id);

#ifdef __cplusplus
/* 0x8042EEA4 - the pending network sub-error message id (0 when there is none); the map's
 * `get_network_sub_error_msg__Fv`.  Added with `quest/arenatask.cpp` (rule 2: this range owns it). */
s32 get_network_sub_error_msg(void);
#endif

#endif /* MHTRI_NETWORK_NETWORK_PAT_CONTROL_H */
