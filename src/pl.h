#ifndef PL_H
#define PL_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef.h"
#include "Pl/plw.h"

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

/* The actor's joint/model block, the one definition (`sound/mhchar.h` includes this header).  Union of
 * `ef/eft002.cpp`, `ef/eft007.cpp`, `ef/fn_80101DF4.cpp`, `ef/fn_80114E34.cpp`, `ef/fn_8011722C.c`,
 * `ef/fn_803066F0.c`, `enemy/fn_80138074.cpp`, `sound/fn_800D7F54.cpp` and the sound band's view
 * (`sound/mhchar.cpp`).  They agree on every shared field; the band views that overlap are unions
 * above (+0x10, +0x44, +0x54).  The sound band's `rot_0x54` was a `VEC3` of floats and is the
 * `_CP_VECTOR` triple here (no consumer of the sound view reads it).  The record ends at +0x140:
 * `ai/ai_npc.h` embeds it at +0x008 with the next field at +0x148, and the sound band's
 * `pad_0x140`/`field_0x160` (+0x140..0x164, read by no source) belong to the 0x164-byte model record
 * `Pl/fn_80224AC4.h` builds around it.  size: 0x140 */
#ifdef __cplusplus
/* The GX channel selector the `get/setTevKColor` and `get/setMatColor` members take; it is an enum
 * because MWCC mangles the enum's name into the symbol (`...12_GXChannelID...`).  `_GXTevKColorID`
 * and the blend/logic selectors come from `gx.h`, which owns them (docs/plan.md 6.5 rule 1). */
enum _GXChannelID {
    GX_COLOR0,
    GX_COLOR1,
    GX_ALPHA0,
    GX_ALPHA1,
    GX_COLOR0A0,
    GX_COLOR1A1,
    GX_COLORZERO,
    GX_ALPHA0A0,
    GX_ALPHA1A1,
    GX_ALPHAZERO
};
#endif
typedef struct MHchar {
    /* +0x000 */ u32 field_0x00;
    /* +0x004 */ VEC3 pos_0x04;
    /* +0x010 */ union {   /* the 0x0C-byte run: a vector for the sound band, the area byte for the AI band */
        /* +0x010 */ VEC3 field_0x10;
        struct {
            /* +0x010 */ u8 pad_0x10[0x6];
            /* +0x016 */ u8 area_0x16;
            /* +0x017 */ u8 pad_0x17[0x5];
        };
    };
    /* +0x01C */ VEC3 scale_0x1C;
    /* +0x028 */ s32 field_0x28;
    /* +0x02C */ s32 field_0x2C;
    /* +0x030 */ s32 field_0x30;
    /* +0x034 */ u8 field_0x34;
    /* +0x035 */ u8 ready;
    /* +0x036 */ u8 pad_0x36[0x2];
    /* +0x038 */ s32 field_0x38;
    /* +0x03C */ u32 field_0x3C;
    /* +0x040 */ s32 field_0x40;
    /* +0x044 */ union {   /* the 0x0C-byte run stays whole, with the two fields `ai/fn_802D0F34.cpp`
                           * reads named inside it (same byte total; nothing moves) */
        /* +0x044 */ u8 pad_0x44[0xC];
        struct {
            /* +0x044 */ f32 field_0x44;   /* the AI band's aim/blend frame factor (`+0x04C` of the
                                           * NPC work record, = this + 8) */
            /* +0x048 */ u8 pad_0x48[0x4];
            /* +0x04C */ u32 field_0x4C;   /* its flag word (bit 0 latches the clamp above) */
        };
    };
    /* +0x050 */ u16 motion_no_0x50;   /* the motion number `Get_motion_no` returns (0x8026A308) */
    /* +0x052 */ u8 pad_0x52[0x2];
    /* +0x054 */ union {   /* the rotation is one 0x0C-byte run; the AI band reads its z word as an
                           * f32 (`+0x064` of the NPC work record) */
        /* +0x054 */ _CP_VECTOR rot_0x54;
        struct {
            /* +0x054 */ u32 rot_x_0x54;
            /* +0x058 */ u32 rot_y_0x58;
            /* +0x05C */ f32 field_0x5C;
        };
    };
    /* +0x060 */ u8 pad_0x60[0x4];
    /* +0x064 */ f32 field_0x64;
    /* +0x068 */ u8 pad_0x68[0xC];
    /* +0x074 */ f32 field_0x74;   /* read by `fn_8026A34C` (0x8026A34C) */
    /* +0x078 */ u8 pad_0x78[0x2C];
    /* +0x0A4 */ f32 field_0xA4;   /* read by `pl_rig_get_float_a4` (0x8026A358) */
    /* +0x0A8 */ u8 pad_0xA8[0x14];
    /* +0x0BC */ f32 field_0xBC;   /* the value `fn_8026A364` tests against 0 */
    /* +0x0C0 */ u8 pad_0xC0[0x14];
    /* +0x0D4 */ f32 field_0xD4[6];
    /* +0x0EC */ s32 field_0xEC;
    /* +0x0F0 */ u8 pad_0xF0[0x1];
    /* +0x0F1 */ u8 field_0xF1;     /* the model visibility flag `fn_8026A2D0`/`fn_8026A2DC` set */
    /* +0x0F2 */ u8 field_0xF2;     /* the second flag, `pl_model_set_state`/`fn_8026A2F8` */
    /* +0x0F3 */ u8 pad_0xF3[0x19];
    /* +0x10C */ struct _g3d_work* g3d_0x10C;   /* the pooled g3d work `push_shell_chara_heap` releases */
    /* +0x110 */ s32 field_0x110;
    /* +0x114 */ s32 field_0x114;
    /* +0x118 */ s32 field_0x118;
    /* +0x11C */ s32 field_0x11C;
    /* +0x120 */ u16 field_0x120;
    /* +0x122 */ u16 field_0x122;
    /* +0x124 */ u8 pad_0x124[0x18];
    /* +0x13C */ u8* field_0x13C;

#ifdef __cplusplus
    /* The engine model/joint entry points the effect units drive; their map names are `MHchar` members
     * (`setVisibility__6MHcharFUlb`, ...), so they are declared here once and called as members
     * (docs/plan.md 6.5 rule 9).  A member adds no storage, so the C view is unchanged. */
    void setVisibility(u32 index, bool visible);                                   /* ef/eft001.cpp, ef/eft007.cpp */
    int get_joint_num(void);                                                       /* ef/fn_8030681C.cpp */
    void get_joint_wpos(u32 joint, nw4r::math::VEC3* out);                         /* ef/eft001.cpp, ef/eft007.cpp */
    void move(u16 flags);                                                          /* ef/eft001.cpp */
    void move2(nw4r::math::MTX34* mtx, u16 flags);                                 /* ef/eft001.cpp */
    void getTevKColor(u32 index, _GXTevKColorID id, _GXColor* out);                /* ef/effect.cpp */
    void setTevKColor(u32 index, _GXTevKColorID id, _GXColor* color);              /* ef/eft001.cpp, ef/effect.cpp */
    /* Returns the material-found status in r3 (0 on the no-resource and no-material paths, the inner
     * reader's value otherwise); `enemy/em025_prog.cpp` compares it against 1. */
    u32 getMatColor(u32 index, _GXChannelID channel, _GXColor* out);               /* ef/effect.cpp */
    void setMatColor(u32 index, _GXChannelID channel, _GXColor color, bool keep);  /* ef/effect.cpp */
    void setAmbColor(u32 index, _GXChannelID channel, _GXColor color, bool keep);  /* sound/mhchar.cpp */
    void setScaleAll(f32 scale);                                                   /* sound/mhchar.cpp */
    void frame_init(long a, u16 b, f32 c, u32 d, f32 e);                           /* sound/mhchar.cpp */
    void setTevKColor(u32 index, _GXTevKColorID id, _GXColor color);               /* by-value overload, enemy/em004_act.cpp */
    void setMatAlphaBlendMode(u32 index, _GXBlendMode mode, _GXBlendFactor src,
                              _GXBlendFactor dst, _GXLogicOp op);                  /* ef/eft001.cpp */
#endif
} MHchar;


#ifdef __cplusplus
}
#endif

#endif /* PL_H */
