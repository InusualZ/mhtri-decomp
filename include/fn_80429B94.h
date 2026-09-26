/*
 * fn_80429B94.h - shared views for the 0x80429B94 band (network/server-control work).
 *
 * The `net_ctrl_wk` singleton (map: `.sbss:0x80794CF8`, a 4-byte pointer) is the record every
 * function in the band dereferences; its owner TU is unclaimed, so the extern lives here beside the
 * one unit that reads it today (docs/plan.md 6.5 rule 2's unsplit-address gap).  Only the offsets
 * this unit reads are named; every other byte stays `pad_0xNNN`.
 *
 * `NetCtrlWk` size: 0xC3F3 approximate - the highest offset any function in the band touches is
 * +0xC3F2; the record is larger than anything this unit proves.
 */
#ifndef MHTRI_FN_80429B94_H
#define MHTRI_FN_80429B94_H

#include "types.h"

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
    /* +0x078 */ u8 pad_0x078[0x7C60];
    /* +0x7CD8 */ NetCtrlEntry entries_0x7CD8[16];
    /* +0x8298 */ u8 pad_0x8298[0x2C];
    /* +0x82C4 */ u8 flag_0x82C4;
    /* +0x82C5 */ u8 flag_0x82C5;
    /* +0x82C6 */ u8 pad_0x82C6;
    /* +0x82C7 */ u8 flag_0x82C7;
    /* +0x82C8 */ u8 pad_0x82C8[0x3C64];
    /* +0xBF2C */ u8 flag_0xBF2C;
    /* +0xBF2D */ u8 pad_0xBF2D[0x433];
    /* +0xC360 */ u16 counter_0xC360;
    /* +0xC362 */ u16 counter_0xC362;
    /* +0xC364 */ u32 counter_0xC364;
    /* +0xC368 */ u8 pad_0xC368[0x8A];
    /* +0xC3F2 */ u8 flag_0xC3F2;
    /* +0xC3F3 */ u8 tail_0xC3F3[];
} NetCtrlWk; /* size: 0xC3F3 (approximate, see above) */

/* A view of an SDK/vtable object that only the slot at +0x128 / +0x148 is ever called through
 * (docs/plan.md 6.5 rule 10: a table outside our ranges is referenced as a struct of typed function
 * pointers, never as a class). */
typedef struct NetSessionManagerVtbl {
    /* +0x000 */ void* pad_0x000[0x128 / 4];
    /* +0x128 */ void (*fn_0x128)(void* self, u32 a, u32 b);
    /* +0x12C */ void* pad_0x12C[0x1C / 4];
    /* +0x148 */ void (*fn_0x148)(void* self);
} NetSessionManagerVtbl;

/* The work-record singleton, defined by another (unclaimed) TU. */
extern NetCtrlWk* net_ctrl_wk;

/* The network facade the band drives; both are unsplit addresses. */
void* getPatsObject(void);
void* getNetworkSessionManagerPat(void* pats, int index);
BOOL fn_803DF1A8(void* session_manager);

#ifdef __cplusplus
}
#endif

/* Mangled callees are declared with their real signatures, never with the map's mangling
 * (docs/plan.md 6.5 rule 9): the C++ front-end emits the map's name itself. */
void sysSE_req(long id);
int flfntStrLen(char* str);

#endif /* MHTRI_FN_80429B94_H */
