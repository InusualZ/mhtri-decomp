/* This unit's own types and declarations (`src/lobby/fn_8020C588.cpp`).
 *
 * The range is the lobby's player-character control band; it drives the lobby work block `lobby_w` and
 * the lobby string pool.  A header rather than the unit's own source because the lobby C++ ABI it
 * declares is spelled with the map's own name for the string helper (`LbStr__FUcUs` has to come out of
 * a `LbStr(unsigned char, unsigned short)` definition) and because the symbols this range **owns**
 * belong with their owner, not in `include/unsplit/lobby.h` (docs/plan.md 6.5 rule 2).
 *
 * `lobby_w` and `lbl_80794880` are re-declared here with this range's own view because
 * `include/unsplit/lobby.h`'s view is not the one this range reads: that header spells `lbl_80794880`
 * as a byte array where the map records a 4-byte `.sbss` pointer (this range loads it, so it is a
 * pointer), and it declares `fn_8021213C(s32, s16)` where this range defines it with one argument.
 * Recorded as a config_request; `include/lobby/fn_801F3294.h` and `include/lobby/fn_8021E1EC.h` carry
 * the same note for the same header.
 *
 * docs/plan.md 6.5 rules 3/4/5: every type states its size, every field its offset and a name.
 */
#ifndef MHTRI_LOBBY_FN_8020C588_H
#define MHTRI_LOBBY_FN_8020C588_H

#include "types.h"

/* The `.bss` lobby work block (`lobby_w`, 0x806AAB44, 0x17C bytes in the map).  The two in-use player
 * slots sit on a 10-byte stride from +0x84; the rest of the record belongs to the band's other units.
 * size: 0x17C */
typedef struct LbLobbyWork {
    /* +0x000 */ u8 state_0x000;   /* the lobby scene/mode `fn_80211E68` dispatches on (0..0x25) */
    /* +0x001 */ u8 unused_0x001[3];
    /* +0x004 */ u8 toggle_0x004;  /* the byte `fn_80211E68` flips, then plays SE 5 / SE 1 for */
    /* +0x005 */ u8 unused_0x005[3];
    /* +0x008 */ u8 redraw_0x008;  /* set when `state_0x000` is outside the menu states 0x23/0x24 */
    /* +0x009 */ u8 unused_0x009[0x1F];
    /* +0x028 */ u8 countdown_0x028; /* the byte counter `fn_8021261C` decays while it is nonzero */
    /* +0x029 */ u8 unused_0x029[7];
    /* +0x030 */ s32 counter_0x030;  /* incremented once per `fn_8021261C` */
    /* +0x034 */ s16 slide_0x034;    /* the cursor slide `fn_8021261C` ramps inside +-0x3C */
    /* +0x036 */ u8 unused_0x036[0x4E];
    /* +0x084 */ u16 cmd_mask_0x084[2][5]; /* per-player UI command/availability words, rebuilt from
                                           * the pad record by `fn_80212370`.  The four bit testers
                                           * (fn_8021213C/12198/121F4/12250) read slots 0-3 of player 0
                                           * and the four getters (fn_802122E8, fn_802122AC,
                                           * `glplatTextureGetHeight`, fn_80212334) return slots 0-3
                                           * whole. */
    /* +0x098 */ u8 unused_0x098[0xDA];
    /* +0x172 */ u16 busy_0x172;    /* nonzero while a lobby transition owns the screen */
} LbLobbyWork; /* size: 0x17C */

extern "C" {
extern LbLobbyWork lobby_w;

/* The lobby item-database pointer (`lbl_80794880`, `.sbss` 0x80794880, 4 bytes).  `fn_802125C8`
 * toggles the byte 0x3E00 of the block it points at. */
extern u8* lbl_80794880;
}

/* The `lb_*_str` tables: four `.sbss` pointers to arrays of string pointers, indexed by species id -
 * `fn_80211DCC` felyne-chaCha skill, `fn_80211DE0` felyne mask, `fn_80211DF4` felyne name,
 * `fn_80211E08` poogie name. */
extern s32* lb_chacha_skill_str;
extern s32* lb_chacha_mask_str;
extern s32* lb_cat_name;
extern s32* lb_pig_name;

/* The lobby string table: `lb_str_tbl` is a `.sbss` pointer to five arrays of string pointers, and
 * `lbl_805B9450` holds their counts.  `LbStr` returns group `kind`'s entry `idx`, or group 1's first
 * entry when either is out of range. */
extern u32** lb_str_tbl;
extern s32 lbl_805B9450[6];

/* ------------------------------------------------------------------ *
 * Declarations this range owns
 * ------------------------------------------------------------------ */

/* `LbStr__FUcUs` - the lobby string fetch (`include/unsplit/lobby.h` declares it `void* LbStr(u8,u16)`;
 * the return type is not part of the mangling, so the two spellings are the same symbol). */
void* LbStr(u8 kind, u16 idx);

/* The lobby screen-fade state (`src/fn_80056F24.cpp`); its symbol is the mangling
 * `get_fade_stat__Fl`, so it is C++ linkage like `LbStr`. */
s32 get_fade_stat(s32 slot);

extern "C" {
/* The four command-mask testers and the four getters, all reading player 0 of
 * `LbLobbyWork::cmd_mask_0x084`. */
s32 fn_8021213C(u16 mask);
s32 fn_80212198(u16 mask);
s32 fn_802121F4(u16 mask);
s32 fn_80212250(u16 mask);
u16 fn_802122AC(void);
u16 fn_802122E8(void);
u16 fn_80212334(void);

/* `glplatTextureGetHeight` is the map's own name for the unguarded getter of `cmd_mask_0x084[0][0]`. */
u16 glplatTextureGetHeight(void);

void fn_80212540(void* self);
void fn_80212584(void* self);
s16 fn_802126E8(s16* table, s16 count, u32 value);
s16 fn_80212724(u8* table, s16 count, u32 value);
}

/* ------------------------------------------------------------------ *
 * Foreign callees
 *
 * Declared here (this unit's header) because no header of theirs does it yet - `fn_802A9068` and
 * `fn_803768F8` have no owner at all, `fn_801E9888`/`fn_801E9C58`'s owner (`src/lobby/fn_801E7530.cpp`)
 * has no header, and `fn_8021F238`/`fn_80220114`'s owner (`src/lobby/fn_8021E1EC.cpp`) does not declare
 * them.  Recorded as config_requests; the argument types are the ones the callees' own bodies show.
 * ------------------------------------------------------------------ */

extern "C" {
/* The lobby's own guard: nonzero while a dialogue/prompt owns input. */
s32 fn_8021F238(void);

/* Unsplit-band callees with no registered owner. */
s32 fn_800D0708(void);
void fn_801E9888(void);
void fn_801E9C58(void);
void fn_80220114(void);
void fn_802A9068(void* self, u16 value, s32 a, s32 b);
void fn_802FF2C0(void);
u32 fn_803768F8(void);
}

#endif /* MHTRI_LOBBY_FN_8020C588_H */
