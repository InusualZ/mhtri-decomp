#ifndef PL_H
#define PL_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The player-module shared records.  `_PLW` is the player work record every Pl/ef/enemy/sound unit
 * touches; `MHchar` the actor's joint/model block; `_SLOTENT` and `_EQUIP` the equipment tables it
 * carries by value.  Each was copied into several units, with only the fields the owner read named;
 * the definitions below are the union of every copy (the merged layouts come from the field tables in
 * `build/tmp/canon__PLW.txt` etc.).
 *
 * Naming debt: many `_PLW` fields are still spelled `unkNNN` from `Pl/pl_act.cpp` (the copy that names
 * the most of the record) and `field_0xNN` from the others.  They are collected here with their offsets
 * so wave 2 can name them from context as each unit is converted (rule 5).
 */

/* One 12-byte equipment record (definition from `Pl/pl_skill.cpp`). size: 0xC */
typedef struct _EQUIP {
    u8 kind;         /* +0x0 */  /* equipment kind; picks which skill fields apply (1-5, 6, 7-15) */
    u8 deco_count;   /* +0x1 */  /* number of decoration skill ids in skill_id */
    u16 item_id;     /* +0x2 */  /* 0 when the slot is empty */
    u16 deco_level;  /* +0x4 */  /* two decoration skill levels, low byte first */
    u16 skill_id[3]; /* +0x6 */  /* the decoration skill ids */
} _EQUIP;

/* One 4-byte equipment-slot entry.  Both copies agree on the layout; `Pl/pl_act.cpp` calls the first
 * field `id` and `Pl/pl_skill.cpp` `item_id` -- the descriptive spelling is canonical. size: 0x4 */
typedef struct _SLOTENT {
    u16 item_id; /* +0x0 */
    s16 value;   /* +0x2 */
} _SLOTENT;

/* The actor's joint/model block.  Union of `ef/eft002.cpp`, `ef/eft007.cpp`, `ef/fn_80101DF4.cpp`,
 * `ef/fn_80114E34.cpp`, `ef/fn_8011722C.c`, `ef/fn_803066F0.c`, `enemy/fn_80138074.c` and
 * `sound/fn_800D7F54.cpp`.  They agree on every shared field; the full record is at least 0x140
 * (`ef/eft007.cpp` states 0x140 as `Pl/pl_act.cpp`'s extent, matching `fn_80114E34.cpp`'s +0x13C
 * pointer). size: 0x140 */
typedef struct MHchar {
    /* +0x000 */ u32 field_0x00;
    /* +0x004 */ VEC3 pos_0x04;
    /* +0x010 */ u8 pad_0x10[0x6];
    /* +0x016 */ u8 area_0x16;
    /* +0x017 */ u8 pad_0x17[0x5];
    /* +0x01C */ VEC3 scale_0x1C;
    /* +0x028 */ s32 field_0x28;
    /* +0x02C */ s32 field_0x2C;
    /* +0x030 */ s32 field_0x30;
    /* +0x034 */ u8 field_0x34;
    /* +0x035 */ u8 ready;
    /* +0x036 */ u8 pad_0x36[0xA];
    /* +0x040 */ s32 field_0x40;
    /* +0x044 */ u8 pad_0x44[0x10];
    /* +0x054 */ _CP_VECTOR rot_0x54;
    /* +0x060 */ u8 pad_0x60[0x4];
    /* +0x064 */ f32 field_0x64;
    /* +0x068 */ u8 pad_0x68[0xAC];
    /* +0x114 */ s32 field_0x114;
    /* +0x118 */ s32 field_0x118;
    /* +0x11C */ u8 pad_0x11C[0x20];
    /* +0x13C */ u8* field_0x13C;

#ifdef __cplusplus
    /* The engine model/joint entry points the effect units drive; their map names are `MHchar` members
     * (`setVisibility__6MHcharFUlb`, ...), so they are declared here once and called as members
     * (docs/plan.md 6.5 rule 9).  A member adds no storage, so the C view is unchanged. */
    void setVisibility(u32 index, bool visible);                                   /* ef/eft001.cpp, ef/eft007.cpp */
    void setTevKColor(u32 index, _GXTevKColorID id, _GXColor* color);              /* ef/eft001.cpp, ef/eft007.cpp */
    void setMatAlphaBlendMode(u32 index, _GXBlendMode mode, _GXBlendFactor src,
                              _GXBlendFactor dst, _GXLogicOp op);                  /* ef/eft001.cpp */
    void get_joint_wpos(u32 joint, nw4r::math::VEC3* out);                         /* ef/eft001.cpp, ef/eft007.cpp */
    void move(u16 flags);                                                          /* ef/eft001.cpp */
    void move2(nw4r::math::MTX34* mtx, u16 flags);                                 /* ef/eft001.cpp */
#endif
} MHchar;

/* The player work record.  Union of `Pl/pl_act.cpp` (the most complete view, 0x668), `Pl/pl_master.cpp`,
 * `Pl/pl_skill.cpp`, `ef/eft002.cpp`, `ef/eft004.cpp`, `ef/eft007.cpp`, `ef/fn_80114E34.cpp`,
 * `ef/fn_800FD520.c`, `enemy/fn_8012BDF4.cpp` and `sound/fn_800D7F54.cpp`.
 *
 * Disagreement found: the SIZE.  The Pl/ef views stop at 0x668; `sound/fn_800D7F54.cpp` reaches a
 * `_se_w*` at +0xAFC and `enemy/fn_8012BDF4.cpp` pads its copy to 0xB20, so the record continues past
 * 0x668.  The 0x668 boundary is the Pl module's view, not the record's end; the size here is the union
 * (0xB20) and the bytes between the Pl view and it are padding.  Also see `_PLW_PHYSICS` below: the
 * +0x13C pointer is spelled `u8*` in `Pl/pl_act.cpp`/`ef/fn_800FD520.c` and `_PLW_PHYSICS*` in
 * `ef/fn_80114E34.cpp` (same 4 bytes; the typed spelling is canonical).
 * size: 0xB20 */
typedef struct _PLW _PLW;
typedef struct _PLW_PHYSICS _PLW_PHYSICS; /* defined in ef/fn_80114E34.cpp; only pointed at here */
struct _PLW {
    /* +0x000 */ u8 slot_active;   /* the slot is in use (`fn_800EFAC0`/`fn_800EFDD8`) */
    /* +0x001 */ u8 pad_0x001[0x1];
    /* +0x002 */ u8 unk2;
    /* +0x003 */ u8 unk003[0x008 - 0x003];
    /* +0x008 */ u8 chunk_ofs;     /* plus 0x14 is the chunk index its files go to */
    /* +0x009 */ u8 unk009;
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 unk00B;
    /* +0x00C */ u16 unk00C;
    /* +0x00E */ u8 unk00E[0x14 - 0x0E];
    /* +0x014 */ u8 se_name_set;   /* picks the SE/BGM name table (`fn_800EFAC0`) */
    /* +0x015 */ u8 unk015;
    /* +0x016 */ u8 area_0x16;
    /* +0x017 */ u8 unk017[0x18 - 0x17];
    /* +0x018 */ u8 unk18;
    /* +0x019 */ u8 pad_0x19[0x1];
    /* +0x01A */ u16 field_0x01A;
    /* +0x01C */ u8 pad_0x1C[0x1];
    /* +0x01D */ u8 se_name_idx;   /* indexes the `fn_800EFAC0` name table */
    /* +0x01E */ u8 pad_0x01E[0x2];
    /* +0x020 */ u32 unk020;
    /* +0x024 */ u8 pad_0x24[0x8];
    /* +0x02C */ _SHELL_W* equip_0x2C;
    /* +0x030 */ s8 flag_0x30;
    /* +0x031 */ u8 pad_0x31[0xB];
    /* +0x03C */ f32 unk03C;
    /* +0x040 */ f32 unk40;
    /* +0x044 */ f32 unk44;
    /* +0x048 */ u8 unk048[0x54 - 0x48];
    /* +0x054 */ u32 param_0x54;
    /* +0x058 */ u32 unk58;
    /* +0x05C */ u32 unk5C;
    /* +0x060 */ f32 ground_y_0x060;  /* the player's base/target y the effect sits on */
    /* +0x064 */ f32 unk064;
    /* +0x068 */ u8 pad_0x68[0x4];
    /* +0x06C */ f32 unk6C;
    /* +0x070 */ f32 unk70;
    /* +0x074 */ u8 unk074;
    /* +0x075 */ u8 unk075[0x90 - 0x75];
    /* +0x090 */ f32 unk090[3];
    /* +0x09C */ f32 unk09C;
    /* +0x0A0 */ f32 unk0A0;
    /* +0x0A4 */ f32 unk0A4;
    /* +0x0A8 */ u8 unk0A8[0xB4 - 0xA8];
    /* +0x0B4 */ s16 unk0B4;
    /* +0x0B6 */ u8 pad_0xB6[0x4];
    /* +0x0BA */ u16 unkBA;
    /* +0x0BC */ u16 unkBC;
    /* +0x0BE */ u16 unkBE;
    /* +0x0C0 */ u8 pad_0xC0[0xC];
    /* +0x0CC */ u16 unkCC;
    /* +0x0CE */ u16 unkCE;
    /* +0x0D0 */ u8 pad_0xD0[0x40];
    /* +0x110 */ f32 unk110;
    /* +0x114 */ u8 pad_0x114[0x14];
    /* +0x128 */ u8 unk128;
    /* +0x129 */ u8 pad_0x129[0xB];
    /* +0x134 */ u8 unk134;
    /* +0x135 */ u8 pad_0x135[0x7];
    /* +0x13C */ _PLW_PHYSICS* physics_0x13C;
    /* +0x140 */ _EQUIP equipA[6];
    /* +0x188 */ u8 pad_0x188[0x1C];
    /* +0x1A4 */ u8 effect_key_0x1A4;
    /* +0x1A5 */ u8 pad_0x1A5[0x2B];
    /* +0x1D0 */ _EQUIP equipB;
    /* +0x1DC */ _EQUIP equipB2;
    /* +0x1E8 */ _EQUIP equipC;
    /* +0x1F4 */ _EQUIP equipD;
    /* +0x200 */ _EQUIP equipE[2];
    /* +0x218 */ s32 set_applied[7];
    /* +0x234 */ s32 set_pending[7];
    /* +0x250 */ u16 equip_valid;
    /* +0x252 */ u16 set_valid;
    /* +0x254 */ u16 deco_dirty;
    /* +0x256 */ u8 pad_0x256[0xE];
    /* +0x264 */ s16 unk264;
    /* +0x266 */ u8 unk266[0x269 - 0x266];
    /* +0x269 */ u8 unk269;
    /* +0x26A */ u8 unk26A;
    /* +0x26B */ u8 unk26B;
    /* +0x26C */ u8 unk26C;
    /* +0x26D */ u8 unk26D;
    /* +0x26E */ u16 unk26E;
    /* +0x270 */ s16 unk270;
    /* +0x272 */ s16 unk272;
    /* +0x274 */ u8 unk274;
    /* +0x275 */ u8 unk275;
    /* +0x276 */ u8 unk276;
    /* +0x277 */ u8 unk277[0x278 - 0x277];
    /* +0x278 */ _SLOTENT slot_id[24];
    /* +0x2D8 */ u8 unk2D8[0x2E0 - 0x2D8];
    /* +0x2E0 */ _SLOTENT spare_slot_id[8];
    /* +0x300 */ u8 unk300[0x304 - 0x300];
    /* +0x304 */ u16 unk304;
    /* +0x306 */ u16 unk306;
    /* +0x308 */ u8 unk308[0x30C - 0x308];
    /* +0x30C */ u8 unk30C;
    /* +0x30D */ u8 unk30D;
    /* +0x30E */ u8 unk30E;
    /* +0x30F */ u8 unk30F[0x313 - 0x30F];
    /* +0x313 */ s8 unk313;
    /* +0x314 */ u8 unk314;
    /* +0x315 */ u8 unk315[0x318 - 0x315];
    /* +0x318 */ u32 unk318;
    /* +0x31C */ s16 unk31C;
    /* +0x31E */ s16 unk31E;
    /* +0x320 */ s16 unk320;
    /* +0x322 */ u8 unk322[16];
    /* +0x332 */ u8 unk332[0x354 - 0x332];
    /* +0x354 */ f32 unk354;
    /* +0x358 */ f32 unk358;
    /* +0x35C */ u8 pad_0x35C[0x4];
    /* +0x360 */ u32 unk360;
    /* +0x364 */ u32 unk364;
    /* +0x368 */ u8 unk368;
    /* +0x369 */ u8 unk369;
    /* +0x36A */ u8 unk36A;
    /* +0x36B */ u8 unk36B;
    /* +0x36C */ s16 unk36C;
    /* +0x36E */ u8 unk36E[0x370 - 0x36E];
    /* +0x370 */ s16 unk370;
    /* +0x372 */ s16 unk372;
    /* +0x374 */ u8 pad_0x374[0x2];
    /* +0x376 */ s16 unk376;
    /* +0x378 */ s16 unk378;
    /* +0x37A */ s16 unk37A;
    /* +0x37C */ s16 unk37C;
    /* +0x37E */ u8 pad_0x37E[0x2];
    /* +0x380 */ s16 unk380;
    /* +0x382 */ u8 pad_0x382[0x2];
    /* +0x384 */ s16 field_0x384;
    /* +0x386 */ s16 unk386;
    /* +0x388 */ u8 unk388;
    /* +0x389 */ u8 unk389[0x38A - 0x389];
    /* +0x38A */ s16 unk38A;
    /* +0x38C */ s16 unk38C;
    /* +0x38E */ s16 unk38E;
    /* +0x390 */ s16 unk390;
    /* +0x392 */ s16 unk392;
    /* +0x394 */ s16 unk394;
    /* +0x396 */ s16 unk396;
    /* +0x398 */ u8 unk398[0x39E - 0x398];
    /* +0x39E */ u8 unk39E;
    /* +0x39F */ u8 unk39F[0x3A2 - 0x39F];
    /* +0x3A2 */ s8 unk3A2;
    /* +0x3A3 */ s8 unk3A3;
    /* +0x3A4 */ u8 unk3A4[0x3AC - 0x3A4];
    /* +0x3AC */ u32 unk3AC;
    /* +0x3B0 */ u8 unk3B0[0x3B4 - 0x3B0];
    /* +0x3B4 */ u8 unk3B4;
    /* +0x3B5 */ u8 unk3B5;
    /* +0x3B6 */ u8 pad_0x3B6[0x2];
    /* +0x3B8 */ u16 unk3B8;
    /* +0x3BA */ u16 unk3BA;
    /* +0x3BC */ f32 unk3BC;
    /* +0x3C0 */ f32 unk3C0;
    /* +0x3C4 */ f32 unk3C4;
    /* +0x3C8 */ f32 unk3C8;
    /* +0x3CC */ f32 unk3CC;
    /* +0x3D0 */ f32 unk3D0;
    /* +0x3D4 */ f32 unk3D4;
    /* +0x3D8 */ u32 unk3D8;
    /* +0x3DC */ u32 unk3DC;
    /* +0x3E0 */ u32 unk3E0;
    /* +0x3E4 */ u8 unk3E4[0x3EC - 0x3E4];
    /* +0x3EC */ s16 unk3EC;
    /* +0x3EE */ u8 unk3EE[0x3F2 - 0x3EE];
    /* +0x3F2 */ s16 unk3F2;
    /* +0x3F4 */ u8 unk3F4[0x3F8 - 0x3F4];
    /* +0x3F8 */ s16 unk3F8;
    /* +0x3FA */ u8 unk3FA[0x3FE - 0x3FA];
    /* +0x3FE */ s16 unk3FE;
    /* +0x400 */ u8 unk400[0x404 - 0x400];
    /* +0x404 */ s16 unk404;
    /* +0x406 */ u8 unk406[0x40E - 0x406];
    /* +0x40E */ s16 unk40E;
    /* +0x410 */ u8 unk410[0x414 - 0x410];
    /* +0x414 */ s16 unk414;
    /* +0x416 */ s16 unk416;
    /* +0x418 */ u8 unk418[0x41A - 0x418];
    /* +0x41A */ s16 unk41A;
    /* +0x41C */ s16 unk41C;
    /* +0x41E */ u8 unk41E[0x420 - 0x41E];
    /* +0x420 */ s16 unk420;
    /* +0x422 */ s16 unk422;
    /* +0x424 */ s16 unk424;
    /* +0x426 */ s16 unk426;
    /* +0x428 */ s16 unk428;
    /* +0x42A */ s16 unk42A;
    /* +0x42C */ s16 unk42C;
    /* +0x42E */ u8 pad_0x42E[0x18];
    /* +0x446 */ u8 unk446;
    /* +0x447 */ u8 unk447;
    /* +0x448 */ s8 unk448;
    /* +0x449 */ s8 unk449;
    /* +0x44A */ u8 unk44A[0x44C - 0x44A];
    /* +0x44C */ s8 unk44C;
    /* +0x44D */ s8 unk44D;
    /* +0x44E */ u8 unk44E[0x45A - 0x44E];
    /* +0x45A */ s16 unk45A;
    /* +0x45C */ u8 unk45C[0x466 - 0x45C];
    /* +0x466 */ s16 unk466;
    /* +0x468 */ s16 unk468;
    /* +0x46A */ s16 unk46A;
    /* +0x46C */ u8 unk46C;
    /* +0x46D */ u8 unk46D;
    /* +0x46E */ u8 unk46E;
    /* +0x46F */ u8 unk46F;
    /* +0x470 */ s16 unk470;
    /* +0x472 */ u8 unk472[0x489 - 0x472];
    /* +0x489 */ u8 unk489;
    /* +0x48A */ u8 unk48A[0x492 - 0x48A];
    /* +0x492 */ u8 unk492;
    /* +0x493 */ u8 pad_0x493[0x11];
    /* +0x4A4 */ u32 field_0x4A4;
    /* +0x4A8 */ u8 pad_0x4A8[0x34];
    /* +0x4DC */ u8 unk4DC;
    /* +0x4DD */ u8 unk4DD[0x4E5 - 0x4DD];
    /* +0x4E5 */ u8 unk4E5;
    /* +0x4E6 */ u8 unk4E6[0x4EE - 0x4E6];
    /* +0x4EE */ u8 unk4EE;
    /* +0x4EF */ u8 unk4EF[0x538 - 0x4EF];
    /* +0x538 */ u8 unk538;
    /* +0x539 */ u8 unk539[0x580 - 0x539];
    /* +0x580 */ s16 unk580;
    /* +0x582 */ u8 unk582;
    /* +0x583 */ s8 unk583;
    /* +0x584 */ u8 unk584[0x59C - 0x584];
    /* +0x59C */ u16 unk59C;
    /* +0x59E */ u16 unk59E;
    /* +0x5A0 */ u16 unk5A0;
    /* +0x5A2 */ u8 unk5A2[0x5A4 - 0x5A2];
    /* +0x5A4 */ u16 field_0x5A4;
    /* +0x5A6 */ u8 unk5A6;
    /* +0x5A7 */ u8 unk5A7[0x5BB - 0x5A7];
    /* +0x5BB */ u8 unk5BB;
    /* +0x5BC */ u8 unk5BC[0x5C4 - 0x5BC];
    /* +0x5C4 */ u8 unk5C4;
    /* +0x5C5 */ u8 unk5C5[0x5E5 - 0x5C5];
    /* +0x5E5 */ u8 unk5E5;
    /* +0x5E6 */ u8 unk5E6;
    /* +0x5E7 */ u8 unk5E7;
    /* +0x5E8 */ s16 unk5E8;
    /* +0x5EA */ u8 pad_0x5EA[0x8];
    /* +0x5F2 */ u16 unk5F2[8];
    /* +0x602 */ u8 unk602[8];
    /* +0x60A */ u8 unk60A[0x612 - 0x60A];
    /* +0x612 */ u16 deco_skill_id[4];
    /* +0x61A */ u16 unk61A[8];
    /* +0x62A */ u8 unk62A[8];
    /* +0x632 */ u8 unk632[0x634 - 0x632];
    /* +0x634 */ u32 unk634;
    /* +0x638 */ u32 unk638;
    /* +0x63C */ u32 unk63C;
    /* +0x640 */ u32 unk640;
    /* +0x644 */ u8 pad_0x644[0xB];
    /* +0x64F */ s8 unk64F;
    /* +0x650 */ u8 pad_0x650[0x5];
    /* +0x655 */ u8 field_0x655;
    /* +0x656 */ u8 pad_0x656[0x8];
    /* +0x65E */ u8 field_0x65E;
    /* +0x65F */ u8 unk65F[0x662 - 0x65F];
    /* +0x662 */ s16 field_0x662;
    /* +0x664 */ u16 field_0x664;
    /* +0x666 */ u16 field_0x666;
    /* +0x668 */ u8 pad_0x668[0x494];
    /* +0xAFC */ _se_w* field_0xAFC;
    /* +0xB00 */ u8 pad_0xB00[0x20];
};

#ifdef __cplusplus
}
#endif

#endif /* PL_H */
