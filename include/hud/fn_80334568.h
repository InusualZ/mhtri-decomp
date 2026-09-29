/* The declarations `src/hud/fn_80334568.cpp` owns (docs/plan.md 6.5, rules 1-5).
 *
 * The unit is the game's character-state network sync: its builders pack fields of a player work record
 * (`_PLW`, reached through `get_move_work_adrs(2)`, 0xB20 stride) or of an enemy work record
 * (`_ENEMY_WORK`) into a small local message and hand it to `fn_8042C9C8(message, size)` - the
 * `NetworkSessionManagerPat` send, guarded by `fn_8042CB9C()` (`net_ctrl_wk->0x11 == 7`), vtable slot
 * 0x128 - and its receivers unpack one back into the work record.  The module is `hud` from the flags
 * and the placement (see the unit header); the content is network, so that call is recorded there as
 * the unit's first promotion candidate.
 *
 * Rule-2 debt, recorded rather than guessed.  The callees whose owner is a registered unit are included
 * from the owner's header; only the ones no header carries at all (`fn_8027D5A4`, `fn_8028BD54`,
 * `fn_8042C9C8`, `fn_8042CB9C`, `fn_8042CC20`, `em_get_unique_work`, `lbl_805E1ED0`) are re-declared
 * below, and the fold into `Pl/pl_act.h`, `Pl/fn_80288CEC.h`, `Network/network_pat_control.h` and
 * `include/unsplit/unknown.h` is a `shared-file` request in this unit's outbox, not this unit's edit.
 */
#ifndef MHTRI_HUD_NET_CHAR_SYNC_H
#define MHTRI_HUD_NET_CHAR_SYNC_H

#include "types.h"
#include "pl.h"
#include "enemy.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _ENEMY_MINI_WORK;

/* ---- the messages this unit sends and receives (one struct per wire message) ---- */

/* The 4-byte header every sender of this unit writes first.  `+0x1` is the sender's own slot index,
 * `+0x2` the destination (the following slot for a player message, the fixed 5 an enemy message uses)
 * and `+0x3` the kind the receiver switches on. size: 0x4 */
typedef struct NetMsgHeader {
    /* +0x0 */ u8 reserved;   /* always 0 */
    /* +0x1 */ u8 from_slot;
    /* +0x2 */ u8 to_slot;
    /* +0x3 */ u8 kind;
} NetMsgHeader;

/* The 0x4C-byte player-state message `fn_80334A9C`/`fn_803359A0` build and `fn_80334C94`/`fn_80335B50`
 * apply: position, the running act, the health pair and the four decoration-skill bytes. size: 0x4C */
typedef struct NetPlStateMsg {
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ u16 field_0x04;   /* the caller's `param` */
    /* +0x06 */ u8 pad_0x06[0x2];
    /* +0x08 */ VEC3 pos;         /* `_PLW` +0x03C */
    /* +0x14 */ u32 field_0x14;   /* `_PLW` +0x0AC */
    /* +0x18 */ u32 field_0x18;   /* `_PLW` +0x0A8 */
    /* +0x1C */ u8 pad_0x1C[0x4];
    /* +0x20 */ u8 kind_0x15;     /* `_PLW` +0x015 */
    /* +0x21 */ u8 area_0x16;     /* `_PLW` +0x016 */
    /* +0x22 */ u8 field_0x3b6;
    /* +0x23 */ u8 act_kind;      /* `_PLW` +0x00A, the key the receiver switches on */
    /* +0x24 */ u16 act_no;       /* `_PLW` +0x00C */
    /* +0x26 */ u16 field_0xb6;   /* `_PLW` +0x0B6 */
    /* +0x28 */ s16 health;       /* `_PLW` +0x370 */
    /* +0x2A */ s16 health_max;   /* `_PLW` +0x372 */
    /* +0x2C */ s16 field_0x37a;  /* `_PLW` +0x37A */
    /* +0x2E */ u8 stagger;       /* `_PLW` +0x386 for act kind 7, else `_PLW` +0x312 */
    /* +0x2F */ u8 field_0x5c4;   /* `_PLW` +0x5C4 */
    /* +0x30 */ u8 field_0x56b;   /* `_PLW` +0x56B */
    /* +0x31 */ u8 field_0x64f;   /* `_PLW` +0x64F */
    /* +0x32 */ u16 field_0x306;  /* `_PLW` +0x306 */
    /* +0x34 */ f32 gauge;        /* `_PLW` +0x3B8 widened to float */
    /* +0x38 */ u8 field_0x001;   /* `_PLW` +0x001 */
    /* +0x39 */ u8 act_mode;      /* `_PLW` +0x018 */
    /* +0x3A */ u8 field_0x26c;   /* `_PLW` +0x26C */
    /* +0x3B */ u8 field_0x567;   /* `_PLW` +0x567 */
    /* +0x3C */ s16 field_0x37e;  /* `_PLW` +0x37E */
    /* +0x3E */ s16 field_0x452;  /* `_PLW` +0x452 sign-extended */
    /* +0x40 */ u32 field_0x3d8;  /* `_PLW` +0x3D8 */
    /* +0x44 */ u32 field_0x3dc;  /* `_PLW` +0x3DC */
    /* +0x48 */ u8 deco_skill[4]; /* `_PLW` +0x612 `deco_skill_id[4]`, low byte of each */
} NetPlStateMsg;

/* The 0x28-byte player attack message `fn_80335200` builds and sends. size: 0x28 */
typedef struct NetPlAtkMsg {
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ VEC3 pos;         /* the caller's position record */
    /* +0x10 */ u32 field_0x10;   /* zeroed */
    /* +0x14 */ u32 field_0x14;   /* the caller's `param`, widened from its u16 source */
    /* +0x18 */ u32 field_0x18;   /* zeroed */
    /* +0x1C */ u8 kind_0x15;     /* `_PLW` +0x015 actor kind */
    /* +0x1D */ u8 attack_kind;   /* the caller's `attack_kind` */
    /* +0x1E */ u8 field_0x5c4;   /* `_PLW` +0x5C4 */
    /* +0x1F */ s8 field_0x01F;   /* `fn_80274808` (the act's follow-up latch)` */
    /* +0x20 */ u16 field_0x416;  /* `_PLW` +0x416 */
    /* +0x22 */ u16 field_0x41c;  /* `_PLW` +0x41C */
    /* +0x24 */ u8 field_0x009;   /* `_PLW` +0x009 */
    /* +0x25 */ u8 field_0x001;   /* `_PLW` +0x001 */
    /* +0x26 */ u8 pad_0x26[0x2];
} NetPlAtkMsg;

/* The 0x30-byte player message `fn_8033502C` builds from a caller-supplied position record. size: 0x30 */
typedef struct NetPlPosMsg {
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ VEC3 pos;
    /* +0x10 */ u32 field_0x10;   /* `_PLW` +0x0AC */
    /* +0x14 */ u32 field_0x14;   /* `_PLW` +0x0A8 */
    /* +0x18 */ u32 field_0x18;   /* zeroed */
    /* +0x1C */ u8 kind_0x15;     /* `_PLW` +0x015 */
    /* +0x1D */ u8 area_0x16;     /* `_PLW` +0x016 */
    /* +0x1E */ s16 health;       /* `_PLW` +0x370 */
    /* +0x20 */ s16 health_max;   /* `_PLW` +0x372 */
    /* +0x22 */ s16 field_0x37a;  /* `_PLW` +0x37A */
    /* +0x24 */ s16 field_0x37e;  /* `_PLW` +0x37E */
    /* +0x26 */ u8 pad_0x26[0x2];
    /* +0x28 */ u32 field_0x3d8;  /* `_PLW` +0x3D8 */
    /* +0x2C */ u32 field_0x3dc;  /* `_PLW` +0x3DC */
} NetPlPosMsg;

/* The 0x38-byte player message `fn_80335468` builds and sends. size: 0x38 */
typedef struct NetPlExtraMsg {
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ u16 field_0x04;   /* the caller's `param` */
    /* +0x06 */ u8 pad_0x06[0x2];
    /* +0x08 */ VEC3 pos;         /* `_PLW` +0x03C */
    /* +0x14 */ u32 field_0x14;   /* `_PLW` +0x0AC */
    /* +0x18 */ u32 field_0x18;   /* `_PLW` +0x0A8 */
    /* +0x1C */ u8 pad_0x1C[0x4];
    /* +0x20 */ u8 kind_0x15;     /* `_PLW` +0x015 */
    /* +0x21 */ u8 area_0x16;     /* `_PLW` +0x016 */
    /* +0x22 */ u8 act_kind;      /* `_PLW` +0x00A */
    /* +0x23 */ u8 pad_0x23[0x1];
    /* +0x24 */ u16 act_no;       /* `_PLW` +0x00C */
    /* +0x26 */ u16 field_0xb6;   /* `_PLW` +0x0B6 */
    /* +0x28 */ s16 health;       /* `_PLW` +0x370 */
    /* +0x2A */ s16 health_max;   /* `_PLW` +0x372 */
    /* +0x2C */ u8 field_0x001;   /* `_PLW` +0x001 */
    /* +0x2D */ u8 act_mode;      /* `_PLW` +0x018 */
    /* +0x2E */ s16 field_0x37e;  /* `_PLW` +0x37E */
    /* +0x30 */ u16 field_0x650;  /* `_PLW` +0x650 */
    /* +0x32 */ s16 field_0x652;  /* `_PLW` +0x652 */
    /* +0x34 */ u8 field_0x655;   /* `_PLW` +0x655 */
    /* +0x35 */ u8 pad_0x35[0x3];
} NetPlExtraMsg;

/* The 0x0C-byte player message `fn_803356F0` builds: two layouts selected by its `kind`, one for the
 * primary act (kind 7) and one for the rest. size: 0xC */
typedef struct NetPlShortMsg {
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ u8 field_0x04;   /* the caller's `param` on the non-7 layout */
    /* +0x05 */ u8 pad_0x05[0x1];
    /* +0x06 */ u8 field_0x06;   /* `_PLW` +0x656 (kind 7: `_PLW` +0x008) */
    /* +0x07 */ u8 field_0x07;   /* `_PLW` +0x008 (kind 7: the caller's `param`) */
    /* +0x08 */ u16 field_0x650; /* `_PLW` +0x650 */
    /* +0x0A */ u16 field_0x652; /* `_PLW` +0x652 */
} NetPlShortMsg;

/* The enemy-state message every receiver of this unit takes.  The layout beyond the header and the
 * enemy id is not named here (the receivers that walk it are not reconstructed yet), so the tail is a
 * stated approximation: 0x54 is the largest offset a receiver of this unit reads. size: 0x5C
 * (approximate) */
typedef struct NetEmStateMsg {
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ u16 enemy_id;    /* the `em_get_unique_work` key */
    /* +0x06 */ u8 pad_0x06[0x56];
} NetEmStateMsg;

/* The 8-byte per-enemy status record `fn_80337648` copies out of an enemy work record. size: 0x8 */
typedef struct NetEmStatus {
    /* +0x0 */ u16 field_0x0;
    /* +0x2 */ u8 field_0x2;
    /* +0x3 */ u8 field_0x3;
    /* +0x4 */ u8 field_0x4;
    /* +0x5 */ u8 field_0x5;
    /* +0x6 */ u8 field_0x6;
    /* +0x7 */ u8 field_0x7;
} NetEmStatus;

/* The player move-work record `get_move_work_adrs(2)` indexes with a 0xB20 stride.  `_PLW` is its
 * head - every field this unit touches lies inside `_PLW` - and the tail is not named anywhere yet.
 * size: 0xB20 */
typedef struct PlMoveWork {
    /* +0x000 */ _PLW pl;
    /* +0xAF4 */ u8 pad_0xAF4[0xB20 - sizeof(_PLW)];
} PlMoveWork;

/* ---- this unit's own bodies, in address order (a bare `fn_` map name is the map's placeholder) ---- */

void fn_803346B4(_PLW* plw);
void fn_80334A34(NetMsgHeader* hdr, u8 from, u8 to, u8 kind);
void fn_80334A9C(_PLW* plw, u8 from, u8 to, u8 kind, u16 param);
void fn_80334C94(u8 slot, const NetPlStateMsg* msg);
u32 fn_80334A4C(void);
void* fn_80334C60(NetPlStateMsg* msg);
void fn_8033502C(_PLW* plw, u8 from, u8 to, u8 kind);
void* fn_80335114(NetPlPosMsg* msg);
void fn_80335148(u8 slot, const NetPlPosMsg* msg);
void fn_80335200(u8 attack_kind, const VEC3* pos, u16 param, u8 kind);
void* fn_80335328(NetPlAtkMsg* msg);
void fn_8033535C(u8 attack_kind, const VEC3* pos, u16 param);
void fn_8033536C(u8 attack_kind, const VEC3* pos, u16 param);
void fn_8033537C(u8 slot, const NetPlExtraMsg* msg, u32 extra);
void fn_80335468(_PLW* plw, u8 from, u8 to, u8 kind, u16 param);
void* fn_80335574(NetPlExtraMsg* msg);
void fn_803355A8(u8 slot, const NetPlExtraMsg* msg);
void fn_803356F0(_PLW* plw, u8 from, u8 to, u8 kind, u16 param);
void fn_803357A8(u8 slot, const NetPlExtraMsg* msg);
void fn_803358D8(u8 slot, const NetPlExtraMsg* msg);
void fn_803359A0(_PLW* plw, u8 from, u8 to, u8 kind, u16 param);
void* fn_80335B1C(NetPlStateMsg* msg);
void fn_80335B50(u8 slot, const NetPlStateMsg* msg);
void fn_80335CE8(_PLW* plw, u8 kind, u16 param);
void fn_80335DD8(u8 slot, const NetMsgHeader* msg);
void fn_8033609C(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void fn_80336488(_ENEMY_WORK* work, const NetEmStateMsg* msg);
void fn_803368E8(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void fn_80336AB0(_ENEMY_WORK* work, const NetEmStateMsg* msg);
void fn_80336D14(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void fn_80336DA8(_ENEMY_WORK* work, const NetEmStateMsg* msg);
void fn_80336EA0(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void fn_80336F34(_ENEMY_WORK* work, const NetEmStateMsg* msg);
void fn_80336F84(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void fn_80337050(_ENEMY_WORK* work, const NetEmStateMsg* msg);
void fn_80337104(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void fn_80337198(_ENEMY_WORK* work, const NetEmStateMsg* msg);
void fn_80337224(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void fn_803372B8(_ENEMY_WORK* work, const NetEmStateMsg* msg);
void fn_8033737C(_ENEMY_WORK* work, u8 kind, u16 param);
void fn_803374B0(_ENEMY_WORK* unused_work, const NetEmStateMsg* msg);
void fn_80337648(NetEmStatus* dst, const NetEmStatus* src);

/* ---- callees with no owner header (rule-2 debt; the outbox carries the fold request) ---- */
/* `copyVec3` (0x80041E40) is `src/mh3_pad.cpp`'s and now comes from `include/mh3_pad.h`, which this
 * unit includes - the `(10197)` clash with `include/ef.h`'s `VEC3*` pair is closed (both headers
 * spell the helpers identically, 2026-09-27). */
/* The four `Pl/fn_80273B14.cpp` callees this unit drives (`Pl_act_set_motion`,
 * `Pl_act_set_motion_slot`, `fn_80274808`, `fn_802756F0`) come from that unit's own header
 * (`Pl/fn_80273B14.h`), which the unit includes - a declaration belongs with the TU that owns the
 * symbol (rule 2).  The `Pl_act_state_ck` collision this block used to describe was with
 * `hud/cockpit_quest.h`, which this unit does not include. */
void fn_8027D5A4(struct _PLW* self, s32 param);                                 /* Pl/pl_act.cpp */
u32 fn_8028BD54(void);                                                          /* Pl/fn_80288CEC.cpp */
void fn_8042C9C8(const void* msg, u32 size);                                    /* Network/network_pat_control.cpp */
u32 fn_8042CB9C(void);                                                          /* Network/network_pat_control.cpp */
u32 fn_8042CC20(void);                                                          /* Network/network_pat_control.cpp */
extern u8 lbl_805E1ED0[];                                                       /* .data 0x805E1ED0 */

#ifdef __cplusplus
}

/* ---- C++-linkage callees (rule 9: the map spells these names mangled, so the declaration is the real
 * one at C++ scope - writing the mangled spelling, or an `extern "C"` one, emits the wrong reloc). ---- */

/* `ef/fn_800CDB2C.cpp` owns it; its own header does not carry the declaration. */
void* get_move_work_adrs(u8 kind);

/* The enemy module's id lookup; the mangling is
 * `em_get_unique_work__FUsPP11_ENEMY_WORKPP16_ENEMY_MINI_WORK`. */
u32 em_get_unique_work(u16 id, _ENEMY_WORK** out, struct _ENEMY_MINI_WORK** mini);

#endif

#endif /* MHTRI_HUD_NET_CHAR_SYNC_H */
