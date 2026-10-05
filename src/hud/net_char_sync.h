/* The declarations `src/hud/net_char_sync.cpp` owns (docs/plan.md 6.5, rules 1-5).
 *
 * The unit is the game's character-state network sync: each family packs a work record into one local wire
 * message (the structs below, one per message) and sends it with `broadcastSessionCommand(message, size)`, the
 * `NetworkSessionManagerPat` send guarded by `isServerSelectState()`; its receivers apply a message back.  The
 * player family reaches its work through `get_move_work_adrs(2)` (0xB20 stride, `PlMoveWork`), the enemy family
 * through `_ENEMY_WORK`, the enemy-control family through `EmcWork` and the effect-slot family through `EftSlot`.
 * Every message starts with `NetMsgHeader`; the enemy ones continue with `NetEmIdent`, the effect-slot ones with
 * `NetEftIdent`.  Constructors and `NetMsgHeader::fill` are class members (their map rows carry the manglings).
 */
#ifndef MHTRI_HUD_NET_CHAR_SYNC_H
#define MHTRI_HUD_NET_CHAR_SYNC_H

#include "types.h"
#include "hud/NetMsgHeader.h"
#include "pl.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/ENEMY_MINI_WORK.h"
#include "enemy/EnemyData.h"
#include "enemy/enemy_control.h"
#include "ef/EftSlot.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the messages this unit sends and receives (one struct per wire message) ---- */


/* The 0x4C-byte player-state message `Pl_net_send_state`/`Pl_net_send_state_alt` build and `Pl_net_recv_state`/`Pl_net_recv_state_alt`
 * apply: position, the running act, the health pair and the four decoration-skill bytes. size: 0x4C */
typedef struct NetPlStateMsg {
    NetPlStateMsg();   /* constructs the position sub-object(s) */
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ u16 field_0x04;   /* the caller's `param` */
    /* +0x06 */ u8 pad_0x06[0x2];
    /* +0x08 */ VEC3 pos;         /* `_PLW` +0x03C */
    /* +0x14 */ u32 field_0x14;   /* `_PLW` +0x0AC */
    /* +0x18 */ u32 field_0x18;   /* `_PLW` +0x0A8 */
    /* +0x1C */ u8 pad_0x1C[0x4];
    /* +0x20 */ u8 kind_0x15;     /* `_PLW` +0x015 */
    /* +0x21 */ u8 area_0x16;     /* `_PLW` +0x016 */
    /* +0x22 */ u8 sub_area;     /* `_PLW` +0x3B6 */
    /* +0x23 */ u8 act_kind;      /* `_PLW` +0x00A, the key the receiver switches on */
    /* +0x24 */ u16 act_no;       /* `_PLW` +0x00C */
    /* +0x26 */ u16 field_0xB6;   /* `_PLW` +0x0B6 */
    /* +0x28 */ s16 health;       /* `_PLW` +0x370 */
    /* +0x2A */ s16 health_max;   /* `_PLW` +0x372 */
    /* +0x2C */ s16 field_0x37A;  /* `_PLW` +0x37A */
    /* +0x2E */ u8 stagger;       /* `_PLW` +0x386 for act kind 7, else `_PLW` +0x312 */
    /* +0x2F */ u8 field_0x5C4;   /* `_PLW` +0x5C4 */
    /* +0x30 */ u8 field_0x56B;   /* `_PLW` +0x56B */
    /* +0x31 */ u8 field_0x64F;   /* `_PLW` +0x64F */
    /* +0x32 */ u16 field_0x306;  /* `_PLW` +0x306 */
    /* +0x34 */ f32 gauge;        /* `_PLW` +0x3B8 (the skill point total) widened to float */
    /* +0x38 */ u8 field_0x001;   /* `_PLW` +0x001 */
    /* +0x39 */ u8 act_mode;      /* `_PLW` +0x018 */
    /* +0x3A */ u8 held_item_kind; /* `_PLW` +0x26C */
    /* +0x3B */ u8 field_0x567;   /* `_PLW` +0x567 */
    /* +0x3C */ s16 field_0x37E;  /* `_PLW` +0x37E */
    /* +0x3E */ s16 field_0x452;  /* `_PLW` +0x452 sign-extended */
    /* +0x40 */ u32 field_0x3D8;  /* `_PLW` +0x3D8 */
    /* +0x44 */ u32 field_0x3DC;  /* `_PLW` +0x3DC */
    /* +0x48 */ u8 deco_skill[4]; /* `_PLW` +0x612 `deco_skill_id[4]`, low byte of each */
} NetPlStateMsg;

/* The 0x4C-byte alternate player-state message `Pl_net_send_state_alt` builds and `Pl_net_recv_state_alt` applies: the state
 * message's head, with the shell angle and the armed-weapon bytes where the state message keeps its sub-area
 * and arming fields. size: 0x4C */
typedef struct NetPlStateAltMsg {
    NetPlStateAltMsg();   /* constructs the position sub-object(s) */
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
    /* +0x23 */ u8 pad_0x23;
    /* +0x24 */ u16 act_no;       /* `_PLW` +0x00C */
    /* +0x26 */ u16 field_0xB6;   /* `_PLW` +0x0B6 */
    /* +0x28 */ s16 health;       /* `_PLW` +0x370 */
    /* +0x2A */ s16 health_max;   /* `_PLW` +0x372 */
    /* +0x2C */ s16 field_0x37A;  /* `_PLW` +0x37A */
    /* +0x2E */ u8 shell_angle;   /* `_PLW` +0x583 */
    /* +0x2F */ u8 field_0x64F;   /* `_PLW` +0x64F */
    /* +0x30 */ u16 field_0x306;  /* `_PLW` +0x306 */
    /* +0x32 */ u8 pad_0x32[0x2];
    /* +0x34 */ f32 gauge;        /* `_PLW` +0x3B8 widened to float */
    /* +0x38 */ u8 field_0x001;   /* `_PLW` +0x001 */
    /* +0x39 */ u8 act_mode;      /* `_PLW` +0x018 */
    /* +0x3A */ u8 held_item_kind; /* `_PLW` +0x26C */
    /* +0x3B */ u8 pad_0x3B;
    /* +0x3C */ s16 field_0x37E;  /* `_PLW` +0x37E */
    /* +0x3E */ u8 pad_0x3E[0x2];
    /* +0x40 */ u32 field_0x3D8;  /* `_PLW` +0x3D8 */
    /* +0x44 */ u32 field_0x3DC;  /* `_PLW` +0x3DC */
    /* +0x48 */ u8 deco_skill[4]; /* `_PLW` +0x612 `deco_skill_id[4]`, low byte of each */
} NetPlStateAltMsg;

/* The 0x28-byte player attack message `Pl_net_send_hit` builds and sends. size: 0x28 */
typedef struct NetPlAtkMsg {
    NetPlAtkMsg();   /* constructs the position sub-object(s) */
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ VEC3 pos;         /* the caller's position record */
    /* +0x10 */ _CP_VECTOR rot_0x10; /* (0, the caller's `param`, 0): the rotation the receiver copies to `_PLW` +0x054 */
    /* +0x1C */ u8 kind_0x15;     /* `_PLW` +0x015 actor kind */
    /* +0x1D */ u8 area_0x16;     /* the caller's area byte, stored to `_PLW` +0x016 by the receiver */
    /* +0x1E */ u8 field_0x5C4;   /* `_PLW` +0x5C4 */
    /* +0x1F */ u8 field_0x01F;   /* `pl_act_stage_get` (the act's follow-up latch)` */
    /* +0x20 */ u16 field_0x416;  /* `_PLW` +0x416 */
    /* +0x22 */ u16 field_0x41C;  /* `_PLW` +0x41C */
    /* +0x24 */ u8 field_0x009;   /* `_PLW` +0x009 */
    /* +0x25 */ u8 field_0x001;   /* `_PLW` +0x001 */
    /* +0x26 */ u8 pad_0x26[0x2];
} NetPlAtkMsg;

/* The 0x30-byte player message `Pl_net_send_pos` builds from a caller-supplied position record. size: 0x30 */
typedef struct NetPlPosMsg {
    NetPlPosMsg();   /* constructs the position sub-object(s) */
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ VEC3 pos;
    /* +0x10 */ u32 field_0x10;   /* `_PLW` +0x0AC */
    /* +0x14 */ u32 field_0x14;   /* `_PLW` +0x0A8 */
    /* +0x18 */ u32 field_0x18;   /* zeroed */
    /* +0x1C */ u8 kind_0x15;     /* `_PLW` +0x015 */
    /* +0x1D */ u8 area_0x16;     /* `_PLW` +0x016 */
    /* +0x1E */ s16 health;       /* `_PLW` +0x370 */
    /* +0x20 */ s16 health_max;   /* `_PLW` +0x372 */
    /* +0x22 */ s16 field_0x37A;  /* `_PLW` +0x37A */
    /* +0x24 */ s16 field_0x37E;  /* `_PLW` +0x37E */
    /* +0x26 */ u8 pad_0x26[0x2];
    /* +0x28 */ u32 field_0x3D8;  /* `_PLW` +0x3D8 */
    /* +0x2C */ u32 field_0x3DC;  /* `_PLW` +0x3DC */
} NetPlPosMsg;

/* The 0x38-byte player message `Pl_net_send_extra` builds and sends. size: 0x38 */
typedef struct NetPlExtraMsg {
    NetPlExtraMsg();   /* constructs the position sub-object(s) */
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
    /* +0x26 */ u16 field_0xB6;   /* `_PLW` +0x0B6 */
    /* +0x28 */ s16 health;       /* `_PLW` +0x370 */
    /* +0x2A */ s16 health_max;   /* `_PLW` +0x372 */
    /* +0x2C */ u8 field_0x001;   /* `_PLW` +0x001 */
    /* +0x2D */ u8 act_mode;      /* `_PLW` +0x018 */
    /* +0x2E */ s16 field_0x37E;  /* `_PLW` +0x37E */
    /* +0x30 */ u16 field_0x650;  /* `_PLW` +0x650 */
    /* +0x32 */ s16 field_0x652;  /* `_PLW` +0x652 */
    /* +0x34 */ u8 field_0x655;   /* `_PLW` +0x655 */
    /* +0x35 */ u8 pad_0x35[0x3];
} NetPlExtraMsg;

/* The 0x0C-byte player message `Pl_net_send_item` builds: two layouts selected by its `kind`, one for the
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

/* The 4-byte identity record every enemy message carries after its header: the enemy's id, the phase
 * counter the receiver matches and the sender's message step.  size: 0x4 */
typedef struct NetEmIdent {
    /* +0x0 */ u16 enemy_id;    /* `_ENEMY_WORK` +0x01A, the `em_get_unique_work` key */
    /* +0x2 */ u8 phase;        /* `_ENEMY_WORK` +0x016 */
    /* +0x3 */ u8 step;         /* the sender's `net_seq_0x01E` before the increment */
} NetEmIdent;

/* The part-state block the state and target messages end with: the damage level of each of the eight
 * parts, the two broken-part records and their values.  size: 0x20 */
typedef struct NetEmParts {
    /* +0x00 */ u8 damage_level[8];   /* `em_parts_damage_level_get` of each part */
    /* +0x08 */ s8 broken_part[2];    /* the part state of the first two parts with break data */
    /* +0x0A */ s16 value[8];         /* `EmPartRec::value_0x02` of each part */
    /* +0x1A */ s16 broken_value[2];  /* `EmPartRec::value_0x04` of the first two parts with break data */
    /* +0x1E */ u8 pad_0x1E[0x2];
} NetEmParts;

/* The 0x6C-byte enemy-state message (kind 1) the enemy sender builds and its receiver applies: position,
 * target, the rotation words, the action, the special part and the status flags.  size: 0x6C */
typedef struct NetEmStateMsg {
    NetEmStateMsg();   /* constructs the position sub-object(s) */
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ NetEmIdent ident;
    /* +0x08 */ VEC3 pos;            /* `_ENEMY_WORK` +0x188 */
    /* +0x14 */ f32 height;          /* `_ENEMY_WORK` +0x1AC */
    /* +0x18 */ VEC3 target;         /* `_ENEMY_WORK` +0x36C */
    /* +0x24 */ u16 rot_x;           /* `_ENEMY_WORK` +0x1BC, truncated */
    /* +0x26 */ u16 rot_y;           /* `_ENEMY_WORK` +0x1C0 */
    /* +0x28 */ u16 rot_z;           /* `_ENEMY_WORK` +0x1C4 */
    /* +0x2A */ u8 field_0x1EE;      /* `_ENEMY_WORK` +0x1EE */
    /* +0x2B */ u8 field_0x1EF;      /* `_ENEMY_WORK` +0x1EF */
    /* +0x2C */ u16 field_0x1F0;     /* `_ENEMY_WORK` +0x1F0; the receiver stores it to +0x1EC */
    /* +0x2E */ u8 field_0x1E4;      /* `_ENEMY_WORK` +0x1E4 */
    /* +0x2F */ u8 field_0x43D;      /* `_ENEMY_WORK` +0x43D */
    /* +0x30 */ u8 area_no;          /* `_ENEMY_WORK` +0x1E1 */
    /* +0x31 */ u8 field_0x1E7;      /* `_ENEMY_WORK` +0x1E7 */
    /* +0x32 */ u8 field_0x380;      /* `_ENEMY_WORK` +0x380, the special part armed selector */
    /* +0x33 */ u8 state_0x381;      /* `_ENEMY_WORK` +0x381 */
    /* +0x34 */ u16 special_part;    /* the id of the special part the +0x382 index selects, 0xFFFF = none */
    /* +0x36 */ u8 field_0x9F8;      /* `_ENEMY_WORK` +0x9F8 */
    /* +0x37 */ u8 field_0x383;      /* `_ENEMY_WORK` +0x383 */
    /* +0x38 */ f32 field_0x384;     /* `_ENEMY_WORK` +0x384 */
    /* +0x3C */ u16 field_0x388;     /* `_ENEMY_WORK` +0x388 */
    /* +0x3E */ u8 field_0x9F7;      /* `_ENEMY_WORK` +0x9F7 */
    /* +0x3F */ u8 field_0x94F;      /* `_ENEMY_WORK` +0x94F */
    /* +0x40 */ u16 field_0x8A2;     /* `_ENEMY_WORK` +0x8A2 */
    /* +0x42 */ u16 status_bits;     /* bit 0 +0x43E, 1 alt mode, 2-3 the +0x89F mode, 4 +0x916 > 0, 5 +0x94C, 6 status 2,
                                      * 7 +0x1F9, 8 +0x1FA, 9 +0x1FD armed, 10 +0x99B, 11 +0x439 == 0 */
    /* +0x44 */ u8 field_0x91C;      /* `_ENEMY_WORK` +0x91C */
    /* +0x45 */ u8 field_0x919;      /* `_ENEMY_WORK` +0x919 */
    /* +0x46 */ u8 field_0x961;      /* `_ENEMY_WORK` +0x961 */
    /* +0x47 */ u8 pad_0x47;
    /* +0x48 */ u32 field_0x7A0;     /* `_ENEMY_WORK` +0x7A0 */
    /* +0x4C */ NetEmParts parts;
} NetEmStateMsg;

/* The 0x60-byte enemy-target message (kind 2): position, rotation words, the bound player and the
 * status counters.  size: 0x60 */
typedef struct NetEmTargetMsg {
    NetEmTargetMsg();   /* constructs the position sub-object(s) */
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ NetEmIdent ident;
    /* +0x08 */ VEC3 pos;            /* `_ENEMY_WORK` +0x188 */
    /* +0x14 */ f32 height;          /* `_ENEMY_WORK` +0x1AC */
    /* +0x18 */ u16 rot_x;           /* `_ENEMY_WORK` +0x1BC, truncated */
    /* +0x1A */ u16 rot_y;           /* `_ENEMY_WORK` +0x1C0 */
    /* +0x1C */ u16 rot_z;           /* `_ENEMY_WORK` +0x1C4 */
    /* +0x1E */ s8 owner_slot;       /* `_ENEMY_WORK` +0x1F6 */
    /* +0x1F */ u8 area_no;          /* `_ENEMY_WORK` +0x1E1 */
    /* +0x20 */ u8 field_0x94F;      /* `_ENEMY_WORK` +0x94F */
    /* +0x21 */ u8 field_0x91C;      /* `_ENEMY_WORK` +0x91C */
    /* +0x22 */ u8 field_0x919;      /* `_ENEMY_WORK` +0x919 */
    /* +0x23 */ u8 field_0x439;      /* `_ENEMY_WORK` +0x439 */
    /* +0x24 */ s16 field_0x934;     /* `_ENEMY_WORK` +0x934 */
    /* +0x26 */ s16 field_0x92E;     /* `_ENEMY_WORK` +0x92E */
    /* +0x28 */ s16 field_0x924;     /* `_ENEMY_WORK` +0x924 */
    /* +0x2A */ s16 field_0x93A;     /* `_ENEMY_WORK` +0x93A */
    /* +0x2C */ s16 field_0x93E;     /* `_ENEMY_WORK` +0x93E */
    /* +0x2E */ s16 field_0x944;     /* `_ENEMY_WORK` +0x944 */
    /* +0x30 */ s16 field_0x926;     /* `_ENEMY_WORK` +0x926 */
    /* +0x32 */ s16 field_0x916;     /* `_ENEMY_WORK` +0x916 */
    /* +0x34 */ u16 field_0x1F2;     /* `_ENEMY_WORK` +0x1F2 */
    /* +0x36 */ s16 field_0x8A6;     /* `_ENEMY_WORK` +0x8A6 */
    /* +0x38 */ u8 field_0x961;      /* `_ENEMY_WORK` +0x961 */
    /* +0x39 */ u8 confirmed;        /* 1 when the sender asked for it or the ready count is one */
    /* +0x3A */ u8 pad_0x3A[0x2];
    /* +0x3C */ u32 field_0x7A0;     /* `_ENEMY_WORK` +0x7A0 */
    /* +0x40 */ NetEmParts parts;
} NetEmTargetMsg;

/* The 0x0A-byte enemy action-flag message (kind 3): the enemy's pending action flags.  size: 0xA */
typedef struct NetEmFlagsMsg {
    /* +0x0 */ NetMsgHeader hdr;
    /* +0x4 */ NetEmIdent ident;
    /* +0x8 */ u8 flags;         /* `_ENEMY_WORK` +0xAEE: one action request per bit */
    /* +0x9 */ u8 pad_0x09;
} NetEmFlagsMsg;

/* The 0x10-byte enemy action message (kind 4).  size: 0x10 */
typedef struct NetEmActMsg {
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ NetEmIdent ident;
    /* +0x08 */ u8 sub_kind;     /* 0 arms the action, 1 clears it */
    /* +0x09 */ u8 act_id;       /* `_ENEMY_WORK` +0x7C8 */
    /* +0x0A */ u8 pad_0x0A[0x2];
    /* +0x0C */ f32 scale;       /* `_ENEMY_WORK` +0x1D0 */
} NetEmActMsg;

/* The 0x0C-byte enemy count message (kind 5).  size: 0xC */
typedef struct NetEmCountMsg {
    /* +0x0 */ NetMsgHeader hdr;
    /* +0x4 */ NetEmIdent ident;
    /* +0x8 */ u8 sub_kind;      /* 1 a level counter, 2 a part bit set */
    /* +0x9 */ u8 pad_0x09;
    /* +0xA */ u16 value;        /* the level counter (+0x919) or the part bit set */
} NetEmCountMsg;

/* The 0x0A-byte enemy bind message (kind 6).  size: 0xA */
typedef struct NetEmBindMsg {
    /* +0x0 */ NetMsgHeader hdr;
    /* +0x4 */ NetEmIdent ident;
    /* +0x8 */ u8 slot;          /* the player slot the enemy is bound to */
    /* +0x9 */ u8 pad_0x09;
} NetEmBindMsg;

/* The 0x0A-byte enemy release message (kind 7).  size: 0xA */
typedef struct NetEmReleaseMsg {
    /* +0x0 */ NetMsgHeader hdr;
    /* +0x4 */ NetEmIdent ident;
    /* +0x8 */ u8 flag;          /* 0 asks the receiver to re-send its state */
    /* +0x9 */ u8 slot;          /* `_ENEMY_WORK` +0x1F6 */
} NetEmReleaseMsg;

/* ---- the enemy-control message family (group 9) and the effect-slot family (group 10) ---- */

/* The status message (kind 1 and 5 of group 9): the last enemy-control event, replayed by the receiver on
 * the enemy's work and mini work.  size: 0xC */
typedef struct NetEmcStatusMsg {
    /* +0x0 */ NetMsgHeader hdr;
    /* +0x4 */ EmcStatus status;
} NetEmcStatusMsg;

/* The marker message (kind 2 of group 9): one 0x24-byte marker record of the enemy-control work, packed
 * field for field.  size: 0x20 */
typedef struct NetEmcMarkerMsg {
    NetEmcMarkerMsg();   /* constructs the position sub-object(s) */
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ u16 field_0x00;        /* `Marker2Rec` +0x00 */
    /* +0x06 */ u16 enemy_id;          /* `Marker2Rec` +0x02, the `em_get_unique_work` key */
    /* +0x08 */ u8 field_0x04;         /* `Marker2Rec` +0x04 */
    /* +0x09 */ u8 field_0x05;         /* `Marker2Rec` +0x05 */
    /* +0x0A */ u8 field_0x06;         /* `Marker2Rec` +0x06 */
    /* +0x0B */ u8 field_0x07;         /* `Marker2Rec` +0x07 */
    /* +0x0C */ u8 field_0x08;         /* `Marker2Rec` +0x08 */
    /* +0x0D */ u8 field_0x0A;         /* `Marker2Rec` +0x0A */
    /* +0x0E */ u16 field_0x18;        /* `Marker2Rec` +0x18, truncated */
    /* +0x10 */ u16 field_0x1C;        /* `Marker2Rec` +0x1C, truncated */
    /* +0x12 */ u16 field_0x20;        /* `Marker2Rec` +0x20, truncated */
    /* +0x14 */ VEC3 pos;              /* `Marker2Rec` +0x0C */
} NetEmcMarkerMsg;

/* The 4-byte identity record every effect-slot message carries after its header: the slot's two keys, its
 * live byte and the sender's step.  size: 0x4 */
typedef struct NetEftIdent {
    /* +0x0 */ u8 key_0x00;     /* `EftSlot::key_0x00` */
    /* +0x1 */ u8 key_0x01;     /* `EftSlot::key_0x01` */
    /* +0x2 */ u8 live_0x14;    /* `EftSlot::field_0x14` */
    /* +0x3 */ u8 step;         /* the sender's `EftSlot::field_0x04` before the increment, or 0 */
} NetEftIdent;

/* The slot-state message (kind 1): the slot's state, mode and work binding.  size: 0x12 */
typedef struct NetEftStateMsg {
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ NetEftIdent ident;
    /* +0x08 */ u8 live_0x14;      /* `EftSlot::field_0x14` */
    /* +0x09 */ u8 state_0x08;     /* `EftSlot::field_0x08` */
    /* +0x0A */ u8 field_0x0B;     /* `EftSlot::field_0x0B` */
    /* +0x0B */ u8 mode_0x0f;      /* `EftSlot::field_0x0F` */
    /* +0x0C */ u16 work_id;       /* the id of the work record the slot tracks, 0xFFFF = none */
    /* +0x0E */ u8 bound;          /* 1 when the slot's work index is the sender's own */
    /* +0x0F */ u8 field_0x3A;     /* `EftSlot::field_0x3A` */
    /* +0x10 */ u8 field_0x36;     /* `EftSlot::field_0x36` */
    /* +0x11 */ u8 field_0x05;     /* `EftSlot::field_0x05` */
} NetEftStateMsg;

/* The slot-step message (kind 2).  size: 0xE */
typedef struct NetEftStepMsg {
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ NetEftIdent ident;
    /* +0x08 */ u8 live_0x14;      /* `EftSlot::field_0x14` */
    /* +0x09 */ u8 state_0x08;     /* `EftSlot::field_0x08` */
    /* +0x0A */ u8 field_0x0B;     /* `EftSlot::field_0x0B` */
    /* +0x0B */ u8 mode_0x0f;      /* `EftSlot::field_0x0F` */
    /* +0x0C */ u16 work_id;       /* the id of the work record the slot tracks, 0xFFFF = none */
} NetEftStepMsg;

/* The slot-live message (kind 3).  size: 0xA */
typedef struct NetEftLiveMsg {
    /* +0x0 */ NetMsgHeader hdr;
    /* +0x4 */ NetEftIdent ident;
    /* +0x8 */ u8 pad_0x08;
    /* +0x9 */ u8 kind_0x14;       /* `EftSlot::field_0x14`: the key the receiver re-points the slot to */
} NetEftLiveMsg;

/* The slot-position message (kind 4).  size: 0x14 */
typedef struct NetEftPosMsg {
    NetEftPosMsg();   /* constructs the position sub-object(s) */
    /* +0x00 */ NetMsgHeader hdr;
    /* +0x04 */ NetEftIdent ident;
    /* +0x08 */ VEC3 pos;          /* `EftSlot::pos_0x24` */
} NetEftPosMsg;

/* The slot-work message (kind 5).  size: 0xA */
typedef struct NetEftWorkMsg {
    /* +0x0 */ NetMsgHeader hdr;
    /* +0x4 */ NetEftIdent ident;
    /* +0x8 */ u8 work;            /* `EftSlot::work_0x03` */
    /* +0x9 */ u8 confirmed;       /* 1 when the caller asked for it or the ready count is one */
} NetEftWorkMsg;

/* The slot-bind message (kind 7).  size: 0x9 */
typedef struct NetEftBindMsg {
    /* +0x0 */ NetMsgHeader hdr;
    /* +0x4 */ NetEftIdent ident;
    /* +0x8 */ u8 work;            /* the caller's player slot */
} NetEftBindMsg;

/* The slot-release message (kind 8).  size: 0xA */
typedef struct NetEftReleaseMsg {
    /* +0x0 */ NetMsgHeader hdr;
    /* +0x4 */ NetEftIdent ident;
    /* +0x8 */ u8 flag;            /* 0 asks the receiver to re-send its state */
    /* +0x9 */ u8 work;            /* `EftSlot::work_0x03` */
} NetEftReleaseMsg;

/* The slot-mark message (kind 6).  size: 0xA */
typedef struct NetEftMarkMsg {
    /* +0x0 */ NetMsgHeader hdr;
    /* +0x4 */ NetEftIdent ident;
    /* +0x8 */ u8 field_0x3A;      /* `EftSlot::field_0x3A` */
    /* +0x9 */ u8 field_0x3B;      /* `EftSlot::field_0x3B` */
} NetEftMarkMsg;

/* The player move-work record `get_move_work_adrs(2)` indexes with a 0xB20 stride.  `_PLW` is its
 * head - every field this unit touches lies inside `_PLW` - and the tail is not named anywhere yet.
 * size: 0xB20 */
typedef struct PlMoveWork {
    /* +0x000 */ _PLW pl;
    /* +0xAF4 */ u8 pad_0xAF4[0xB20 - sizeof(_PLW)];
} PlMoveWork;

/* ---- this unit's own bodies, in address order (a bare `fn_` map name is the map's placeholder) ---- */

void Pl_act_pair_enter(_PLW* plw, s32 pair);
void Pl_act_step_table_enter(_PLW* plw);
void Pl_act_frame_dispatch(_PLW* plw);
void Pl_net_send_state(_PLW* plw, u8 from, u8 to, u8 kind, u16 param);
void Pl_net_recv_state(u8 slot, const NetPlStateMsg* msg);
u32 Pl_net_can_send(void);
void Pl_net_send_pos(_PLW* plw, u8 from, u8 to, u8 kind);
void Pl_net_recv_pos(u8 slot, const NetPlPosMsg* msg);
void Pl_net_send_hit(u8 attack_kind, const VEC3* pos, u16 param, u8 kind);
void Pl_net_send_hit_kind3(u8 attack_kind, const VEC3* pos, u16 param);
void Pl_net_send_hit_kind5(u8 attack_kind, const VEC3* pos, u16 param);
void Pl_net_recv_hit(u8 slot, const NetPlAtkMsg* msg, u32 extra);
void Pl_net_send_extra(_PLW* plw, u8 from, u8 to, u8 kind, u16 param);
void Pl_net_recv_extra(u8 slot, const NetPlExtraMsg* msg);
void Pl_net_send_item(_PLW* plw, u8 from, u8 to, u8 kind, u16 param);
void Pl_net_recv_item_use(u8 slot, const NetPlShortMsg* msg);
void Pl_net_recv_item_result(u8 slot, const NetPlShortMsg* msg);
void Pl_net_send_state_alt(_PLW* plw, u8 from, u8 to, u8 kind, u16 param);
void Pl_net_recv_state_alt(u8 slot, const NetPlStateAltMsg* msg);
void Pl_net_send(_PLW* plw, u8 kind, u16 param);
void Pl_net_recv(u8 slot, const NetMsgHeader* msg);
void em_net_ident_set(_ENEMY_WORK* work, u8 step, NetEmIdent* ident);
void em_net_parts_pack(_ENEMY_WORK* work, NetEmParts* out);
void em_net_parts_unpack(_ENEMY_WORK* work, const NetEmParts* in);
void em_net_send_state(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void em_net_recv_state(_ENEMY_WORK* work, NetEmStateMsg* msg);
void em_net_send_target(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void em_net_recv_target(_ENEMY_WORK* work, const NetEmTargetMsg* msg);
void em_net_send_flags(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void em_net_recv_flags(_ENEMY_WORK* work, const NetEmFlagsMsg* msg);
void em_net_send_act(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void em_net_recv_act(_ENEMY_WORK* work, const NetEmActMsg* msg);
void em_net_send_count(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void em_net_recv_count(_ENEMY_WORK* work, const NetEmCountMsg* msg);
void em_net_send_bind(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void em_net_recv_bind(_ENEMY_WORK* work, NetEmBindMsg* msg);
void em_net_send_release(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param);
void em_net_recv_release(_ENEMY_WORK* work, NetEmReleaseMsg* msg);
void em_net_send(_ENEMY_WORK* work, u8 kind, u16 param);
void em_net_recv(u8 slot, const NetEmStateMsg* msg);
void emc_net_send_status(EmcWork* emc, u8 from, s32 to, u8 kind);
void emc_net_recv_status(NetEmcStatusMsg* msg);
void emc_net_send_marker(EmcWork* emc, u8 from, s32 to, u8 kind, u16 index);
void emc_net_recv_marker(NetEmcMarkerMsg* msg);
void emc_net_send(EmcWork* emc, u8 kind, u16 param);
void emc_net_recv(u8 slot, NetMsgHeader* msg);
void eft_net_ident_set(EftSlot* slot, u8 step, NetEftIdent* ident);
u16 eft_slot_work_id_get(EftSlot* slot);
u8 eft_slot_work_index_resolve(u8 mode, u16 work_id);
void eft_net_send_state(EftSlot* slot, u8 from, s32 to, u8 kind);
void eft_net_recv_state(EftSlot* slot, NetEftStateMsg* msg);
void eft_net_send_step(EftSlot* slot, u8 from, s32 to, u8 kind);
void eft_net_recv_step(EftSlot* slot, NetEftStepMsg* msg);
void eft_net_send_live(EftSlot* slot, u8 from, s32 to, u8 kind);
void eft_net_recv_live(EftSlot* slot, NetEftLiveMsg* msg);
void eft_net_send_pos(EftSlot* slot, u8 from, s32 to, u8 kind);
void eft_net_recv_pos(EftSlot* slot, NetEftPosMsg* msg);
void eft_net_send_work(EftSlot* slot, u8 from, s32 to, u8 kind, u8 confirm);
void eft_net_recv_work(EftSlot* slot, NetEftWorkMsg* msg, u8 own);
void eft_net_send_mark(EftSlot* slot, u8 from, s32 to, u8 kind);
void eft_net_recv_mark(EftSlot* slot, NetEftMarkMsg* msg);
void eft_net_send_bind(EftSlot* slot, u8 from, s32 to, u8 kind, u8 param);
void eft_net_recv_bind(EftSlot* slot, NetEftBindMsg* msg);
void eft_net_send_release(EftSlot* slot, u8 from, s32 to, u8 kind, u8 param);
void eft_net_recv_release(EftSlot* slot, NetEftReleaseMsg* msg);
void eft_net_send(EftSlot* slot, u32 mode, u32 value);


/* .data 0x805E1ED0 - the per-step table `Pl_act_step_table_enter` installs.  Rule-12 debt: the run is sole-owned by this
 * unit but span-blocked (`enemy/em_pl_frame` reads `lbl_805E1F6C` between it and the claim), so it cannot be claimed yet. */
extern u8 lbl_805E1ED0[];

#ifdef __cplusplus
}

/* ---- C++-linkage callees (rule 9: the map spells these names mangled, so the declaration is the real
 * one at C++ scope - writing the mangled spelling, or an `extern "C"` one, emits the wrong reloc). ---- */

/* The enemy module's id lookup; the mangling is
 * `em_get_unique_work__FUsPP11_ENEMY_WORKPP16_ENEMY_MINI_WORK`. */
u32 em_get_unique_work(u16 id, _ENEMY_WORK** out, struct _ENEMY_MINI_WORK** mini);

#endif

#endif /* MHTRI_HUD_NET_CHAR_SYNC_H */
