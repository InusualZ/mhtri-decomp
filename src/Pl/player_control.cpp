/* Pl/player_control.cpp - the player control set
 *
 * `.text` 0x802673A4..0x802693C4, 12 functions written (the rest of the range is not decompiled yet).
 * Phase 4 (docs/splits/phase4): recut registered unit, built from `Pl/fn_80262940.cpp`.
 * Each function keeps the `#pragma` state it had in its retired source.
 */

#include "types.h"
#include "pl.h"
#include "nw4r/math.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "ef/eft052.h" /* hud_item_msg_push (the owner's header, rule 2) */
#include "lobby/fn_8021E1EC.h"
#include "ef/fn_800CDB2C.h"
#include "sound/fn_800D7F54.h"

extern "C" {
s32 fn_802673B8(void);
void fn_802673D4(u8 index);
u32 fn_80267A78(u8 index);
u8 fn_80267C84(struct _PLW* self);
u32 fn_80268308(struct _PLW* self);
s32 fn_80269394(struct _PLOBJ* obj);

void fn_80268E38(struct _PLW* self, u8* slots, u8 index);
void fn_80268E48(struct _PLW* self, u32 unused, s32 mode, u8** stream);
}

/* The map spells this one `set_max_player__Fl`, a C++ mangling, so the definition sits at C++ scope and
 * the front-end reproduces the map's name (docs/plan.md 6.5 rule 9). */
void set_max_player(s32 value);

/* 0x802673A4 - stores the maximum player count. */
void set_max_player(s32 value) {
    if (lbl_80794B28 == NULL) {
        return;
    }
    lbl_80794B28->count = value;
}

/* 0x802673B8 - reads the maximum player count back. */
s32 fn_802673B8(void) {
    if (lbl_80794B28 == NULL) {
        return 0;
    }
    return lbl_80794B28->count;
}

/* 0x802673D4 - marks one player slot as ready. */
void fn_802673D4(u8 index) {
    lbl_80794B2C[index] = 1;
}

/* 0x802681D4 - arms the player data load. */
void player_init_data_load(void) {
    if (lbl_80794B28 == NULL) {
        return;
    }
    lbl_80794B28->state = 1;
}

/* 0x802681EC - every live player's 11 state slots have left the "not yet started" value. */
u32 player_move_start_ck(void) {
    _PLGLOBAL* global = lbl_80794B28;
    _PLWORK* work = global->objects;
    _PLOBJ* obj = global->table;
    s32 pending = 0;
    s32 i;

    for (i = 0; i < global->count; i++) {
        if (work[i].active != 0) {
            s32 slot;

            for (slot = 0; slot < 11; slot++) {
                if (obj[i].state[slot] != 1) {
                    pending++;
                }
            }
        }
    }
    return pending == 0;
}

/* 0x80267A78 - one player's per-player work word. */
u32 fn_80267A78(u8 index) {
    if (lbl_80794B28 == NULL) {
        return 0;
    }
    return lbl_80794B28->players_0x44C[index];
}

/* 0x80268E38 - marks one record's slot as "loaded". */
void fn_80268E38(_PLW* self, u8* slots, u8 index) {
    slots[index] = 3;
}

/* 0x80268E48 - advances one streamed record's state from "queued" to "loading". */
void fn_80268E48(_PLW* self, u32 unused, s32 mode, u8** stream) {
    u8* base;
    u32 offset;

    if (mode != 4) {
        return;
    }
    base = stream[0];
    offset = (u32)stream[2];
    if (base[offset] == 4) {
        base[offset] = 5;
    }
}

/* 0x80267C84 - the player-type index the current game mode uses. */
u8 fn_80267C84(_PLW* self) {
    u8 type = 0;
    u8 mode = fn_800CF208();

    if (mode == 2) {
        if (PlayMode_ck() == 5) {
            type = 1;
        } else {
            type = 2;
        }
    } else if (mode == 0 || mode == 3) {
        type = self->field_0x47B;
    }
    return type;
}

/* 0x80268308 - the player record is loaded and every one of its 11 slots has left "empty". */
u32 fn_80268308(_PLW* self) {
    s32 global;
    s32 any;
    s32 i;
    u8 index;

    if (lbl_80794B28 == NULL) {
        return 0;
    }
    index = self->chunk_ofs;
    if (lbl_80794B28->slot_state[index] != 2) {
        return 0;
    }
    any = 0;
    for (i = 0; i < 11; i++) {
        if (lbl_80794B28->table[index].state[i] != 1) {
            any = 1;
        }
    }
    return any != 0;
}

/* 0x80268260 - starts the move for one player (`-1` = every live player). */
void player_move_start(s32 index) {
    _PLGLOBAL* global = lbl_80794B28;
    s32 i;

    if (global == NULL) {
        return;
    }
    if (index == -1) {
        for (i = 0; i < global->count; i++) {
            if (global->objects[i].active != 0) {
                fn_80223E54((s32)global->objects[i].model_0x13C);
            }
        }
    } else if (global->objects[index].active != 0) {
        fn_80223E54((s32)global->objects[index].model_0x13C);
    }
    global->state = 4;
}

/* 0x80269394 - no slot of this record has advanced past "empty". */
s32 fn_80269394(_PLOBJ* obj) {
    s32 count = 0;
    s32 i;

    for (i = 0; i < 11; i++) {
        if (obj->state[i] > 1) {
            count++;
        }
    }
    return count == 0;
}
