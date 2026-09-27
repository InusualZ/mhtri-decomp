/*
 * fn_80429B94.cpp - the 0x80429B94-0x8043065C network/server-control band (114 functions, 27336 B).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/symbols/dumpmap.py: every address in the range answers `zz_<addr>_` or a global name, never a
 * `__FILE__` emitter; the whole unit's `.data` pool carries the strings "so_alloc_fail",
 * "dwc_alloc_fail", "mh3uswii" and "mh.capcom.co.jp" and no bare source-file name).  The real names
 * the map does spell out (`can_enter_server`, `get_server_type_name`, `get_server_type_desc`,
 * `get_server_big_data_offset_element`, `get_network_sub_error_msg`, `getOnlineSupportCode`,
 * `CalculateEvents`) are network/server-control entry points.
 *
 * Home and lib, from evidence (brief section 2, classes 3/4): no `__FILE__` string and no runtime-dump
 * source name cover the range, so the file keeps the map's `fn_80429B94` stem.  The code is game code
 * (it drives `getPatsObject`/`getNetworkSessionManagerPat` and reads the lobby singleton `lobby_w`),
 * not SDK code, and the object carries extab/extabindex (C++ exceptions on) - so it takes the `main`
 * game-root lib's flags (`cflags_main`: `Wii/1.3`, `-O3`, `-inline noauto`, `-Cpp_exceptions on`),
 * like the other un-moduled game-root units beside `main.cpp`.  The module is genuinely un-evidenced;
 * the nearest registered units (Camellia below, Runtime.PPCEABI.H above) name different modules, so
 * the file sits at the repository root and this header records the gap.
 *
 * Sections this unit owns: .text 0x80429B94..0x8043065C, extab 0x8001D368..0x8001D558 (54 records,
 * 496 B), extabindex 0x8003DE54..0x8003E0DC (54 records).  The `.data` jump tables owned by functions
 * in the range are claimed separately (playbook 53); the hand-written labels between them are the
 * neighbouring proposals' and are filed as `range` config_requests, not claimed.
 *
 * Score at this commit and the residuals are below; re-measure with
 * `python tools/units/recompile.py fn_80429B94 --measure <symbol>`.
 *
 * Residuals: fn_80429B94 (the 0x2C68 B state machine, three jump tables), fn_8042D44C, fn_8042FB84
 * and fn_8042FFF0 are not reconstructed yet; every remaining function keeps its map name (rule 7
 * deferral above).
 */

#include "fn_80429B94.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- forward declarations of this unit's own entry points --- */
void fn_80429B94(void);
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

char* strcpy(char* dst, const char* src);
void* memset(void* dst, int value, u32 size);

/*
 * Focus the network-control work when it is idle.
 */
void fn_8042C7FC(void)
{
    if (net_ctrl_wk->state_0x011 != 0) {
        fn_80429B94();
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
        void* manager = getNetworkSessionManagerPat(getPatsObject(), 0);
        (*(NetSessionManagerVtbl**)manager)->fn_0x128(manager, a, b);
    }
}

/*
 * Tell the session manager that the server list was left.
 */
void fn_8042CA44(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (fn_8042C8B0() != 0 && work->state_0x011 == 7) {
        void* manager = getNetworkSessionManagerPat(getPatsObject(), 0);
        (*(NetSessionManagerVtbl**)manager)->fn_0x148(manager);
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
