/*
 * Network/network_pat_control.cpp - the 0x80429B94-0x8043065C network pat-control band
 * (114 functions, 27336 B).
 *
 * NAMING (the file name, and the unit's own subject).  The range carries no `__FILE__` string - its
 * `.data` pool holds the alloc-failure strings, `mh3uswii`, `mh.capcom.co.jp`, `SPACE_STR` and no bare
 * source-file name - and the runtime dump answers `zz_<addr>_` for every address in it; the real names
 * the map does spell out (`can_enter_server`, `get_server_type_name`, `get_server_type_desc`,
 * `get_server_big_data_offset_element`, `get_network_sub_error_msg`, `getOnlineSupportCode`,
 * `CalculateEvents`) describe one subject: the server select the band runs through the `net_ctrl_wk`
 * work record.  What it drives is the **Pat** layer - its own vocabulary is `getPatsObject`,
 * `getNetworkSessionManagerPat`, `getNetworkCommunityPat`, `getNetworkLayerPat`,
 * `updatePatInterface180`, `loadPatInterfaceBuffers`, `setPatField854`/`setPatField860` - so the file
 * takes that word: evidence class 3 (behaviour plus the sibling units' scheme), with the module
 * `Network/`, which is where the four other units of this subsystem sit (`Network/NetworkPat.cpp`,
 * `Network/network_state.cpp`, `Network/initNetworkSessionStable.cpp`, `Network/fn_803D3CE8.cpp`).
 * The previous stem was the map's start address (`fn_80429B94.cpp`), which the landing gate refuses as
 * a unit name.
 *
 * Home and lib, from evidence (brief section 2, classes 3/4): the code is game code (it drives
 * `getPatsObject`/`getNetworkSessionManagerPat`, reads the lobby singleton and the `net_ctrl_wk` work
 * record), not SDK code, and the object carries extab/extabindex (C++ exceptions on) - so it takes the
 * game-root `main` lib's flags (`cflags_main`: `Wii/1.3`, `-O3`, `-inline noauto`, `-Cpp_exceptions
 * on`), like the un-moduled game-root units it was registered beside.
 *
 * Sections this unit owns: .text 0x80429B94..0x8043065C, extab 0x8001D368..0x8001D558 (54 records,
 * 496 B), extabindex 0x8003DE54..0x8003E0DC (54 records).  The `.data` jump tables owned by functions
 * in the range are claimed separately (playbook 53); the hand-written labels between them are the
 * neighbouring proposals' and are filed as `range` config_requests, not claimed.
 *
 * TYPES.  The session manager this band dispatches through is the real class
 * `NetworkSessionManagerPat` (`include/Network/fn_803D3CE8.h`, its owner's header): the slot +0x00 of
 * the holder the accessors walk, the object `__ct__24NetworkSessionManagerPatFv` (0x803D68D0) builds,
 * whose constructor stores the class's vtable 0x805FB0F0 at +0x00.  The two calls this band makes on
 * it are the class's own virtuals - `broadcastPlayerSlots` (+0x128) and `flush` (+0x148) - not
 * address-keyed slots of a hand-written table, so the virtual dispatch is MWCC's own
 * (`lwz r12,0(r3)` / `lwz r12,<slot>(r12)`).
 *
 * NAMING GUESSES.  `updateNetworkPatControl` (0x80429B94, the range's namesake: the 0x2C68 B state
 * machine with three jump tables) is **GUESSED** from its only caller, `fn_8042C7FC` ("focus the
 * network-control work when it is idle") and from its own prologue, which gates on `net_ctrl_wk`'s
 * state and flags before dispatching - i.e. the band's per-frame update.  It is not reconstructed yet.
 *
 * Score at this commit and the residuals are below; re-measure with
 * `python tools/units/recompile.py Network/network_pat_control --measure <symbol>`.
 *
 * Residuals: `updateNetworkPatControl` (the 0x2C68 B state machine, three jump tables), fn_8042D44C,
 * fn_8042FB84 and fn_8042FFF0 are not reconstructed yet; the band's other functions that this file
 * does not define keep their map names - pre-existing rule-7 debt (about 40 definitions) that a
 * reconstruction pass retires with the bodies, the same way the rows written here were named.
 */

#include "Network/network_pat_control.h"
#include "Network/fn_803D3CE8.h"   /* NetworkSessionManagerPat - the class the band dispatches through */
#include "Runtime.PPCEABI.H/memset.h"            /* memset, owner Runtime.PPCEABI.H/memset.c (rule 2) */
#include "unsplit/Runtime.PPCEABI.H.h"           /* strcpy - unowned MSL helper (rule 2's unsplit gap) */
#include "g3d/g3d_anmchr.h"                      /* flfntStrLen, owner g3d/g3d_anmchr.cpp (rule 2) */

#ifdef __cplusplus
extern "C" {
#endif

/* The work-record singleton itself (`.sbss:0x80794CF8`, 4 B).  Its owner TU is unregistered and every
 * function in this band dereferences it, so this unit claims the range (rule 12) and defines it here;
 * `Network/fn_80423E74.cpp` reaches the same row through the header's `extern`. */
NetCtrlWk* net_ctrl_wk;

/* --- forward declarations of this unit's own entry points --- */
void updateNetworkPatControl(void);
void fn_8042C7FC(void);
BOOL fn_8042C814(void);
u8 fn_8042C844(void);
s8 fn_8042C850(void);
BOOL fn_8042C8B0(void);
u8 fn_8042C8DC(void);
void fn_8042C9C8(u32 a, u32 b);
void fn_8042CA44(void);
BOOL fn_8042CAA0(u32 server_index);
NetCtrlEntry* fn_8042CC68(void);
u8 fn_8042CBB4(void);
s32 fn_8042CCE4(u8* bytes);
BOOL fn_8042CB6C(u32 index);

/*
 * Focus the network-control work when it is idle.
 */
void fn_8042C7FC(void)
{
    if (net_ctrl_wk->state_0x011 != 0) {
        updateNetworkPatControl();
    }
}

/*
 * Whether the control is in one of its two "server select" sub-states.
 */
BOOL fn_8042C814(void)
{
    u8 state = net_ctrl_wk->sub_state_0x017;

    if (state == 0x1F || (u8)(state + 0xE0) <= 1) {
        return TRUE;
    }
    return FALSE;
}

/*
 * The currently selected server id.
 */
u8 fn_8042C844(void)
{
    return net_ctrl_wk->selected_server_0x06A;
}

/*
 * How many of the four server slots are occupied.
 */
s8 fn_8042C850(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    s32 count = 0;

    if (work == NULL) {
        return 0;
    }
    if (work->server_slot_state_0x040[0] == 2) {
        count = 1;
    }
    if (work->server_slot_state_0x040[1] == 2) {
        count++;
    }
    if (work->server_slot_state_0x040[2] == 2) {
        count++;
    }
    if (work->server_slot_state_0x040[3] == 2) {
        count++;
    }
    return (s8)count;
}

/*
 * The network session manager's readiness probe.
 */
BOOL fn_8042C8B0(void)
{
    return fn_803DF1A8(getNetworkSessionManagerPat(getPatsObject(), 0));
}
/*
 * The lowest occupied server index, or 0 when none is.
 */
u8 fn_8042C8DC(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    u8 best = 0xFF;
    s8 value;

    if (work == NULL) {
        return 0;
    }
    if (!fn_8042C8B0()) {
        return 0;
    }
    if (work->state_0x011 == 7) {
        value = work->server_index_0x074[0];
        if (value != -1 && best > value) {
            best = value;
        }
        value = work->server_index_0x074[1];
        if (value != -1 && best > value) {
            best = value;
        }
        value = work->server_index_0x074[2];
        if (value != -1 && best > value) {
            best = value;
        }
        value = work->server_index_0x074[3];
        if (value != -1 && best > value) {
            best = value;
        }
    }
    if (best == 0xFF) {
        return 0;
    }
    return best;
}

/*
 * Tell the session manager that a server row was entered.
 */
void fn_8042C9C8(u32 a, u32 b)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (fn_8042C8B0() != 0 && work->state_0x011 == 7) {
        NetworkSessionManagerPat* manager = getNetworkSessionManagerPat(getPatsObject(), 0);

        manager->broadcastPlayerSlots(a, b);
    }
}

/*
 * Tell the session manager that the server list was left.
 */
void fn_8042CA44(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (fn_8042C8B0() != 0 && work->state_0x011 == 7) {
        NetworkSessionManagerPat* manager = getNetworkSessionManagerPat(getPatsObject(), 0);

        manager->flush();
    }
}

/*
 * Whether the low byte of the server rows is the given row.
 */
BOOL fn_8042CAA0(u32 server_index)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return FALSE;
    }
    if (!fn_8042C8B0()) {
        return FALSE;
    }
    if (work->state_0x011 == 7) {
        if ((u8)server_index == work->server_index_0x074[0]) {
            return TRUE;
        }
        if ((u8)server_index == work->server_index_0x074[1]) {
            return TRUE;
        }
        if ((u8)server_index == work->server_index_0x074[2]) {
            return TRUE;
        }
        if ((u8)server_index == work->server_index_0x074[3]) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * Whether the server slot at the given index is occupied.
 */
BOOL fn_8042CB6C(u32 index)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return FALSE;
    }
    return work->server_slot_state_0x040[(u8)index] == 2;
}

/*
 * Whether the control is in its server-select state.
 */
BOOL fn_8042CB9C(void)
{
    return net_ctrl_wk->state_0x011 == 7;
}

/*
 * How many of the four server rows carry a valid index.
 */
u8 fn_8042CBB4(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    u8 count = 0;

    if (work->state_0x011 == 7) {
        if (work->server_index_0x074[0] != -1) {
            count = 1;
        }
        if (work->server_index_0x074[1] != -1) {
            count++;
        }
        if (work->server_index_0x074[2] != -1) {
            count++;
        }
        if (work->server_index_0x074[3] != -1) {
            count++;
        }
    }
    return count;
}

/*
 * Whether the ready counter is at one.
 */
BOOL fn_8042CC20(void)
{
    return net_ctrl_wk->ready_count_0x044 == 1;
}

/*
 * Whether the control is in its third mode.
 */
BOOL fn_8042CC38(void)
{
    return net_ctrl_wk->mode_0x010 == 3;
}

/*
 * Whether the control is in its third mode.
 */
BOOL fn_8042CC50(void)
{
    return net_ctrl_wk->mode_0x010 == 3;
}

/*
 * Reserve the first free entry of the server/message table.
 */
NetCtrlEntry* fn_8042CC68(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    for (u32 i = 0; i < 16; i++) {
        if (work->entries_0x7CD8[i].in_use_0x00 == 0) {
            work->entries_0x7CD8[i].in_use_0x00 = 1;
            return &work->entries_0x7CD8[i];
        }
    }
    return NULL;
}

/*
 * Whether the per-session flag is set.
 */
BOOL fn_8042CCB4(void)
{
    return net_ctrl_wk->flag_0xC3F2 == 1;
}

/*
 * Clear the per-session flag.
 */
void fn_8042CCD0(void)
{
    net_ctrl_wk->flag_0xC3F2 = 0;
}

/*
 * The length of the character (1-3 bytes) starting at the given shift-JIS byte, or 0 when invalid.
 */
s32 fn_8042CCE4(u8* bytes)
{
    s32 first = bytes[0];

    if (first <= 0x7F) {
        return 1;
    }
    if ((u32)(first - 0xC0) <= 0x1F) {
        s32 second = bytes[1];

        if (second == 0) {
            return 0;
        }
        if ((u32)(second - 0x80) > 0x3F) {
            return 0;
        }
        return 2;
    }
    if ((u32)(first - 0xE0) <= 0x0F) {
        s32 second = bytes[1];

        if (second == 0) {
            return 0;
        }
        if ((u32)(second - 0x80) > 0x3F) {
            return 0;
        }
        s32 fourth = bytes[3];

        if (fourth == 0) {
            return 0;
        }
        if ((u32)(fourth - 0x80) > 0x3F) {
            return 0;
        }
        return 3;
    }
    return 0;
}

/*
 * The printed width of a shift-JIS string: its character count less its space characters.
 */
s32 fn_8042CDA0(char* text)
{
    char buffer[0x4C];
    char* cursor = buffer;

    buffer[0] = 0;
    strcpy(buffer, text);
    s32 length = flfntStrLen(buffer);
    s32 spaces = 0;

    for (;;) {
        s32 step = fn_8042CCE4((u8*)cursor);

        if (step == 0) {
            break;
        }
        if (step == 1 && *cursor == 0x20) {
            spaces++;
        }
        cursor += step;
    }
    return length - spaces;
}

/*
 * Post a named server/message entry.
 */
BOOL fn_8042CE38(u8 kind, char* name, u32 value, u8 flags)
{
    if (net_ctrl_wk == NULL) {
        return FALSE;
    }
    if (name == NULL) {
        return FALSE;
    }
    if (!fn_8042CDA0(name)) {
        sysSE_req(2);
        return FALSE;
    }
    NetCtrlEntry* entry = fn_8042CC68();

    if (entry == NULL) {
        return FALSE;
    }
    strcpy(entry->name_0x04, name);
    entry->flags_0x03 = flags;
    entry->value_0x4C = value;
    entry->kind_0x02 = kind;
    memset(entry->text_0x50, 0, 0xA);
    return TRUE;
}

/*
 * Post a named server/message entry with a second line of text.
 */
BOOL fn_8042CF00(u8 kind, char* name, u32 value, u8 flags, char* text)
{
    if (net_ctrl_wk == NULL) {
        return FALSE;
    }
    if (name == NULL) {
        return FALSE;
    }
    if (!fn_8042CDA0(name)) {
        sysSE_req(2);
        return FALSE;
    }
    NetCtrlEntry* entry = fn_8042CC68();

    if (entry == NULL) {
        return FALSE;
    }
    strcpy(entry->name_0x04, name);
    entry->flags_0x03 = flags;
    entry->value_0x4C = value;
    entry->kind_0x02 = kind;
    memset(entry->text_0x50, 0, 0xA);
    if (text != NULL) {
        strcpy(entry->text_0x50, text);
    }
    return TRUE;
}

/*
 * Whether the server-request state is at two.
 */
BOOL fn_8042D094(void)
{
    return net_ctrl_wk->flag_0xBF2C == 2;
}

/*
 * Advise the server-request state.
 */
void fn_8042D0B0(void)
{
    net_ctrl_wk->flag_0xBF2C = 1;
}

#ifdef __cplusplus
}
#endif
