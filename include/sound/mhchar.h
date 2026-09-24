/* The `sound` band's `MHchar` view: the actor's joint/model block as the SE and model methods of
 * `src/sound/fn_800DD1F0.cpp` read it.
 *
 * docs/plan.md 6.5 rules 1/4/5: `MHchar` is a shared record (it also lives in `include/pl.h` as the
 * union of the other bands' views and in several `src/` copies), so this unit includes one definition
 * here rather than adding a ninth `struct MHchar { ... }` to `src/`.  The offsets and names below are
 * the ones this unit's methods touch; the fields the other bands name keep that spelling where the two
 * agree (`pos_0x04`, `scale_0x1C`, `field_0x114`).  The record is at least 0x140 bytes (include/pl.h)
 * and the methods here reach +0x160 (`fn_800E1C2C`), so the size below is the union.
 * size: 0x164 */
#ifndef MHTRI_SOUND_MHCHAR_H
#define MHTRI_SOUND_MHCHAR_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"

#ifdef __cplusplus

/* The GX selectors the material methods take.  They are named after the SDK tags because MWCC encodes
 * the tag in the mangling (`setMatColor__6MHcharFUl12_GXChannelID8_GXColorb`).  Values are the SDK's. */
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

enum _GXTevKColorID {
    GX_KCOLOR0,
    GX_KCOLOR1,
    GX_KCOLOR2,
    GX_KCOLOR3,
    GX_KCOLOR0A0,
    GX_KCOLOR1A1,
    GX_KCOLOR2A2,
    GX_KCOLOR3A3
};

enum _GXBlendMode {
    GX_BM_NONE,
    GX_BM_BLEND,
    GX_BM_LOGIC,
    GX_BM_SUBTRACT,
    GX_MAX_BLENDMODE
};

enum _GXBlendFactor {
    GX_BL_ZERO,
    GX_BL_ONE,
    GX_BL_SRCCLR,
    GX_BL_INVSRCCLR,
    GX_BL_SRCALPHA,
    GX_BL_INVSRCALPHA,
    GX_BL_DSTALPHA,
    GX_BL_INVDSTALPHA
};

enum _GXLogicOp {
    GX_LO_CLEAR,
    GX_LO_AND,
    GX_LO_REVAND,
    GX_LO_COPY,
    GX_LO_INVAND,
    GX_LO_NOOP,
    GX_LO_XOR,
    GX_LO_OR,
    GX_LO_NOR,
    GX_LO_EQUIV,
    GX_LO_INV,
    GX_LO_REVOR,
    GX_LO_INVCOPY,
    GX_LO_INVOR,
    GX_LO_NAND,
    GX_LO_SET
};

class MHchar {
public:
    /* +0x000 */ u32 field_0x00;
    /* +0x004 */ nw4r::math::VEC3 pos_0x04;
    /* +0x010 */ nw4r::math::VEC3 field_0x10;
    /* +0x01C */ nw4r::math::VEC3 scale_0x1C;
    /* +0x028 */ s32 field_0x28;
    /* +0x02C */ s32 field_0x2C;
    /* +0x030 */ s32 field_0x30;
    /* +0x034 */ u8 field_0x34;
    /* +0x035 */ u8 ready;
    /* +0x036 */ u8 pad_0x36[2];
    /* +0x038 */ s32 field_0x38;
    /* +0x03C */ u32 field_0x3C;
    /* +0x040 */ s32 field_0x40;
    /* +0x044 */ u8 pad_0x44[8];
    /* +0x04C */ u32 field_0x4C;
    /* +0x050 */ u8 pad_0x50[4];
    /* +0x054 */ nw4r::math::VEC3 rot_0x54;
    /* +0x060 */ u8 pad_0x60[4];
    /* +0x064 */ f32 field_0x64;
    /* +0x068 */ u8 pad_0x68[0x6C];
    /* +0x0D4 */ f32 field_0xD4[6];
    /* +0x0EC */ s32 field_0xEC;
    /* +0x0F0 */ u8 pad_0xF0[0x20];
    /* +0x110 */ s32 field_0x110;
    /* +0x114 */ s32 field_0x114;
    /* +0x118 */ s32 field_0x118;
    /* +0x11C */ s32 field_0x11C;
    /* +0x120 */ u16 field_0x120;
    /* +0x122 */ u16 field_0x122;
    /* +0x124 */ u8 pad_0x124[0x18];
    /* +0x13C */ u8* field_0x13C;
    /* +0x140 */ u8 pad_0x140[0x20];
    /* +0x160 */ void* field_0x160;

    void move(u16 frame);
    void move2(nw4r::math::MTX34* mtx, u16 frame);
    void setScaleAll(f32 scale);
    void get_joint_wpos(u32 joint, nw4r::math::VEC3* out);
    int get_joint_num(void);
    void frame_init(long a, u16 b, f32 c, u32 d, f32 e);
    void setMatColor(u32 idx, _GXChannelID channel, _GXColor color, bool keep);
    void getMatColor(u32 idx, _GXChannelID channel, _GXColor* out);
    void setAmbColor(u32 idx, _GXChannelID channel, _GXColor color, bool keep);
    void getTevKColor(u32 idx, _GXTevKColorID id, _GXColor* out);
    void setTevKColor(u32 idx, _GXTevKColorID id, _GXColor color);
    void setMatAlphaBlendMode(u32 idx, _GXBlendMode mode, _GXBlendFactor src, _GXBlendFactor dst,
                              _GXLogicOp op);
    void setVisibility(u32 idx, bool visible);
};

#endif /* __cplusplus */

#endif /* MHTRI_SOUND_MHCHAR_H */
