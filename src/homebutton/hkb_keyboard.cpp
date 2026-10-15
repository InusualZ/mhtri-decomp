/* homebutton/hkb_keyboard.cpp - the software keyboard: the layout library's common helpers (nw4r::lyt::detail), the
 *   key tables (language names, kana and command keys, pane switches, accents, layouts, key animations, key
 *   definitions) and the keyboard classes (HkbKeyboardBase and HkbKeyState, the HkbKeyboard panel with its pane
 *   manager and sound bases, the key kinds, the shift/caps buttons and the pane handlers), ending in the static
 *   initialiser that fills the key definitions' animation group.
 * RANGE. .text 0x805352DC-0x8053E808 (145 functions); .ctors 0x8056F3F4-0x8056F3F8; .rodata 0x80579450-0x8057AB98;
 *   .data 0x8064A950-0x8064DE88; .bss 0x807901C0-0x807901F0; .sdata 0x807944C0-0x80794568;
 *   .sbss 0x80795A48-0x80795A50; .sdata2 0x8079D610-0x8079D620.
 *   Edges: the left edge follows homebutton/hkb_arc_resource_accessor.cpp (ends 0x805352DC); the float 0.0 is pooled
 *   again at 0x8079D610 (read by SetSize).  The right edge is the static initialiser 0x8053E5F8 (.ctors 0x8056F3F4)
 *   and the HkbKeyboard this-adjusting thunks to 0x8053E808.  One TU emits the whole .data: the class tables come out
 *   in reverse class-definition order (HbmEventHandler, HkbKeyboardBase, HkbKeyboardPaneHandler, HbmPaneEventHandler,
 *   HkbKeyButton, HkbKeyboard, HkbKey and its four kinds), which reproduces 0x8064DB54..0x8064DE88 exactly.
 * FLAGS. GC/3.0a5.2 + cflags_hkb (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).  `#pragma auto_inline on` from
 *   HkbKeyState::update on (retail inlines getMode, setCaseFlags, the caps toggle and the shift setters into their
 *   callers and keeps their out-of-line copies); scoped auto_inline for TexCoordAry::Free/Reserve.
 * NAMES. the nw4r::lyt::detail helpers carry the layout library's names (lyt_common); DrawQuad(basePt, size, num,
 *   texCoords, vtxColors) at 0x80535774 is the five-argument overload.  GUESS: the file name, every keyboard class,
 *   method, field and table name (read off the bodies: pane names, notification ids 0x12/0x13/0x14/0x1F/0x29, the
 *   HID usage codes, the kana marks); the classes live in homebutton/hkb_keyboard.h, the band's shared classes
 *   (HbmPaneManager, HbmKey, HbmEventHandler, HkbInputWidget, HkbKbdState) in their owners' headers; the view
 *   classes HkbContextView, HkbTextSink, HbmKeyView, HbmSoundPlayer and HbmComponent name only the slots this
 *   file calls.  getMode and isHalfWidth of HkbKeyboardBase are inline; their kept copies sit in
 *   menu/menu_sysmsg.cpp's range (0x804534B0, 0x80452C08).
 * RESIDUALS. unwritten: 0x8053658C, 0x805365D4, 0x805365DC, 0x80536DB8 (88 bytes): inline accessors of band classes
 *   (homebutton/hbm_text_panel.cpp's panel and three widget classes) emitted here first; their classes are not modelled.
 * RESIDUALS. HkbKeyboardBase::updateView (0x805366D0): retail merges the two lamp notifications of each switch into one
 *   call; no source shape tried (one or two lamp variables, a struct argument, an inline helper) merges them.
 * RESIDUALS. findCommandByName: the loop index and the scaled offset swap r30/r31; createKeys: retail keeps only the
 *   key kind 1 in a register (ours also 2); HkbRecentKeys::push: retail strength-reduces the second half of the
 *   shift loop through r3+8; HkbKeyButton::HkbKeyButton: retail stores the vptr after the three argument fields.
 * RESIDUALS. DrawQuad(alpha): the argument copy and the MultipleAlpha result swap stack slots (ut::Color's
 *   destructor); string and key-state references name our pooled `...data.0` and the hkbGetKbdState statics where
 *   the target names lbl_8064A950 and the .sbss words 0x80794D60/0x80794D64 of homebutton/hkb_widget_tail.cpp.
 * RESIDUALS. .sdata/.bss/.sbss: ours end without the target's trailing alignment padding (0xA5/0x2C/5 against
 *   0xA8/0x30/8); the .sbss also carries the hkbGetKbdState guard and object, which retail keeps once elsewhere.
 * RESIDUALS. lyt_common (0x805352DC..0x80535E3C) is very likely a TU of its own (the layout library's
 *   lyt_common.cpp): no data object proves the seam, so it stays here.
 * SHAPES. SetSize's corner table is a guarded function-local static built through CornerVEC2's constructor.  The
 *   pane-switch helpers are file-scope statics so their names join the unit's string pool; single-case `switch`es
 *   and `||` early returns give retail's `beq; b` branch shapes; the shift/caps code toggles the case word through
 *   mask expressions and bit fields (HkbCaseBits) where retail does.
 */
#include "homebutton/hkb_keyboard.h"

#include "homebutton/hbm_value.h"
#include "homebutton/hkb_resource_accessor.h"
#include "homebutton/hkb_widget_tail.h"

#include "homebutton/hbm_hermite.h"
#include "homebutton/hbm_kb_event.h"
#include "homebutton/hbm_text_panel.h"
#include "homebutton/hkb_layout.h"
#include "homebutton/hkb_qwerty_keys.h"
#include "homebutton/hkb_textbox.h"
#include "homebutton/tiHKBManager.h"
#include "MEM/MEMAllocFromAllocator.h"
#include "MSL_C/strstr.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXGeometry.h"
#include "gx.h"

namespace {

/* GX primitive, vertex format and attribute selectors the quad drawing uses (the SDK's enumerations). */
enum {
    GX_QUADS = 0x80,
    GX_VTXFMT0 = 0,
    GX_VA_POS = 9,
    GX_VA_CLR0 = 11,
    GX_VA_TEX0 = 13,
    GX_DIRECT = 1,
    GX_POS_XY = 0,
    GX_CLR_RGBA = 1,
    GX_TEX_ST = 1,
    GX_RGBA8 = 5,
    GX_F32 = 4
};

/* Writes a two-component position to the GX FIFO. */
inline void GXPosition2f32(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Writes a packed RGBA colour to the GX FIFO. */
inline void GXColor1u32(u32 clr) {
    GXWGFifo.u32 = clr;
}

/* Writes a two-component texture coordinate to the GX FIFO. */
inline void GXTexCoord2f32(f32 s, f32 t) {
    GXWGFifo.f32 = s;
    GXWGFifo.f32 = t;
}

/* Closes a GX primitive (nothing to flush on this target). */
inline void GXEnd() {}

/* A texture coordinate built by a constructor, as the layout library's VEC2(x, y) builds one. size: 0x8 */
struct CornerVEC2 : public nw4r::math::VEC2 {
    CornerVEC2(f32 fx, f32 fy) {
        x = fx;
        y = fy;
    }
};

}  // namespace

namespace nw4r {
namespace lyt {
namespace detail {

/* 0x805352DC (0x2C): Returns whether two pane or group names agree in their first 16 characters. */
bool EqualsResName(const char* name1, const char* name2) {
    return strncmp(name1, name2, 16) == 0;
}

/* 0x80535308 (0x2C): Returns whether two material names agree in their first 20 characters. */
bool EqualsMaterialName(const char* name1, const char* name2) {
    return strncmp(name1, name2, 20) == 0;
}

/* 0x80535334 (0x28): Returns whether a resource header carries `signature` and the big-endian byte-order mark. */
bool TestFileHeader(const ut::BinaryFileHeader& fileHeader, u32 signature) {
    return signature == fileHeader.signature && fileHeader.byteOrder == 0xFEFF;
}

/* 0x8053535C (0x14): Starts with no quads. */
TexCoordAry::TexCoordAry() : mCap(0), mNum(0), mpData(NULL) {}

#pragma push
#pragma auto_inline on

/* 0x80535370 (0x50): Returns the quad block to the layout allocator. */
void TexCoordAry::Free() {
    if (mpData != NULL) {
        Layout::DeleteArray(mpData);
        mpData = NULL;
        mCap = 0;
        mNum = 0;
    }
}

/* 0x805353C0 (0x94): Grows the quad block to hold `num` quads. */
void TexCoordAry::Reserve(u8 num) {
    if (mCap < num) {
        Free();
        mpData = Layout::NewArray<TexCoordQuad>(num);
        if (mpData != NULL) {
            mCap = num;
        }
    }
}

#pragma pop

/* 0x80535454 (0xEC): Sets the quad count, filling new quads with the unit square's corners. */
void TexCoordAry::SetSize(u8 num) {
    if (mpData != NULL && num <= mCap) {
        static const CornerVEC2 texCoords[4] = {
            CornerVEC2(0.0f, 0.0f), CornerVEC2(1.0f, 0.0f), CornerVEC2(0.0f, 1.0f), CornerVEC2(1.0f, 1.0f)};

        for (int j = mNum; j < num; ++j) {
            for (int i = 0; i < 4; ++i) {
                mpData[j].coord[i] = texCoords[i];
            }
        }
        mNum = num;
    }
}

/* 0x80535540 (0x94): Copies `texCoordNum` quads out of a resource block. */
/* untyped: byte range - the resource's packed TexCoordQuad array */
void TexCoordAry::Copy(const void* pResTexCoord, u8 texCoordNum) {
    mNum = ut::Max(mNum, texCoordNum);
    const TexCoordQuad* src = static_cast<const TexCoordQuad*>(pResTexCoord);
    for (int j = 0; j < texCoordNum; ++j) {
        for (int i = 0; i < 4; ++i) {
            mpData[j].coord[i] = src[j].coord[i];
        }
    }
}

/* 0x805355D4 (0x68): Returns whether the vertex colours or the alpha are not plain white. */
bool IsModulateVertexColor(ut::Color* vtxColors, u8 glbAlpha) {
    if (glbAlpha != 0xFF) {
        return true;
    }
    if (vtxColors != NULL) {
        if (vtxColors[0].ToU32() != 0xFFFFFFFF || vtxColors[1].ToU32() != 0xFFFFFFFF ||
            vtxColors[2].ToU32() != 0xFFFFFFFF || vtxColors[3].ToU32() != 0xFFFFFFFF) {
            return true;
        }
    }
    return false;
}

/* 0x8053563C (0x54): Returns a colour with its alpha scaled by `alpha`. */
const ut::Color MultipleAlpha(const ut::Color col, u8 alpha) {
    ut::Color ret = col;
    if (alpha != 0xFF) {
        ret.a = col.a * alpha / 255;
    }
    return ret;
}

/* 0x80535690 (0xE4): Sets the GX vertex format: position, optional colour and `texCoordNum` texture coordinates. */
void SetVertexFormat(bool bModulate, u8 texCoordNum) {
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    if (bModulate) {
        GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    }
    for (int i = 0; i < texCoordNum; ++i) {
        GXSetVtxDesc(GX_VA_TEX0 + i, GX_DIRECT);
    }

    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    if (bModulate) {
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    }
    for (int i = 0; i < texCoordNum; ++i) {
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0 + i, GX_TEX_ST, GX_F32, 0);
    }
}

/* 0x80535774 (0x560): Draws one quad of `size` from `basePt` with its texture coordinates and corner colours. */
void DrawQuad(const math::VEC2& basePt, const Size& size, u8 num, const TexCoordQuad* texCoords,
              const ut::Color* vtxColors) {
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    GXPosition2f32(basePt.x, basePt.y);
    if (vtxColors != NULL) {
        GXColor1u32(vtxColors[0].ToU32());
    }
    for (int i = 0; i < num; ++i) {
        GXTexCoord2f32(texCoords[i].coord[0].x, texCoords[i].coord[0].y);
    }

    GXPosition2f32(basePt.x + size.width, basePt.y);
    if (vtxColors != NULL) {
        GXColor1u32(vtxColors[1].ToU32());
    }
    for (int i = 0; i < num; ++i) {
        GXTexCoord2f32(texCoords[i].coord[1].x, texCoords[i].coord[1].y);
    }

    GXPosition2f32(basePt.x + size.width, basePt.y - size.height);
    if (vtxColors != NULL) {
        GXColor1u32(vtxColors[3].ToU32());
    }
    for (int i = 0; i < num; ++i) {
        GXTexCoord2f32(texCoords[i].coord[3].x, texCoords[i].coord[3].y);
    }

    GXPosition2f32(basePt.x, basePt.y - size.height);
    if (vtxColors != NULL) {
        GXColor1u32(vtxColors[2].ToU32());
    }
    for (int i = 0; i < num; ++i) {
        GXTexCoord2f32(texCoords[i].coord[2].x, texCoords[i].coord[2].y);
    }

    GXEnd();
}

/* 0x80535CD4 (0xF8): Draws one quad with its corner colours' alpha scaled by `alpha`. */
void DrawQuad(const math::VEC2& basePt, const Size& size, u8 num, const TexCoordQuad* texCoords,
              const ut::Color* vtxColors, u8 alpha) {
    ut::Color wkVtxColors[4];
    if (vtxColors != NULL) {
        for (int i = 0; i < 4; ++i) {
            wkVtxColors[i] = MultipleAlpha(vtxColors[i], alpha);
        }
    }
    DrawQuad(basePt, size, num, texCoords, vtxColors != NULL ? wkVtxColors : NULL);
}

/* 0x80535DCC (0x70): Binds an animation to every pane of a group. */
void BindAnimation(Group* pGroup, AnimTransform* pAnimTrans, bool bRecursive, bool bDisable) {
    ut::LinkList<PaneLink, 0>* paneList = pGroup->GetPaneList();
    for (ut::LinkList<PaneLink, 0>::Iterator it = paneList->GetBeginIter(); it != paneList->GetEndIter(); ++it) {
        it->mTarget->BindAnimation(pAnimTrans, bRecursive, bDisable);
    }
}

}  // namespace detail
}  // namespace lyt
}  // namespace nw4r

/* ---------------------------------------------------------------------------------------------------
 * The keyboard tables.
 * ------------------------------------------------------------------------------------------------ */

/* The prediction language names (.data 0x8064A950). */
const wchar_t* hkbLanguageNames[11] = {
    L"",
    L"",
    L"Eng",
    L"Fra",
    L"Esp",
    L"EN",
    L"DE",
    L"FR",
    L"ES",
    L"IT",
    L"NL",
};

/* The kana (grid) keys of every layout (.rodata 0x80579450). */
const HkbKanaKeyDef hkbKanaKeyDefs[65] = {
    {"P_Gkey_00", {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
    {"P_Gkey_11", {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
    {"P_Gkey_22", {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
    {"P_Gkey_33", {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
    {"P_Gkey_44", {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
    {"P_Gkey_01", {0x005E, 0x00C2, 0x00E2, 0x0031, 0x0031, 0x0031, 0x0031}},
    {"P_Gkey_12", {0x0060, 0x00CA, 0x00EA, 0x0032, 0x0032, 0x0032, 0x0032}},
    {"P_Gkey_23", {0x00B4, 0x00CE, 0x00EE, 0x0033, 0x0033, 0x0033, 0x0033}},
    {"P_Gkey_34", {0x007E, 0x00D4, 0x00F4, 0x0034, 0x0034, 0x0034, 0x0034}},
    {"P_Gkey_45", {0x00A8, 0x00DB, 0x00FB, 0x0035, 0x0035, 0x0035, 0x0035}},
    {"P_Gkey_05", {0x002C, 0x0000, 0x0000, 0x0036, 0x0036, 0x0036, 0x0036}},
    {"P_Gkey_16", {0x002E, 0x0000, 0x0000, 0x0037, 0x0037, 0x0037, 0x0037}},
    {"P_Gkey_27", {0x0040, 0x0000, 0x0000, 0x0038, 0x0038, 0x0038, 0x0038}},
    {"P_Gkey_38", {0x003A, 0x0000, 0x0000, 0x0039, 0x0039, 0x0039, 0x0039}},
    {"P_Gkey_49", {0x003B, 0x0000, 0x0000, 0x0030, 0x0030, 0x0030, 0x0030}},
    {"P_Gkey_09", {0x00A3, 0x0000, 0x0000, 0x0021, 0x0021, 0x0000, 0x0000}},
    {"P_Gkey_20", {0x00B5, 0x0000, 0x0000, 0x003F, 0x003F, 0x0000, 0x0000}},
    {"P_Gkey_31", {0x00AC, 0x0000, 0x0000, 0x00A1, 0x00A1, 0x0000, 0x0000}},
    {"P_Gkey_42", {0x00A4, 0x0000, 0x0000, 0x00BF, 0x00BF, 0x0000, 0x0000}},
    {"P_Gkey_53", {0x00A7, 0x0000, 0x0000, 0x0026, 0x0026, 0x0000, 0x0000}},
    {"P_Gkey_02", {0x0021, 0x00C4, 0x00E4, 0x0041, 0x0061, 0x0041, 0x0061}},
    {"P_Gkey_13", {0x003F, 0x00D6, 0x00F6, 0x0042, 0x0062, 0x0042, 0x0062}},
    {"P_Gkey_24", {0x00A1, 0x00DC, 0x00FC, 0x0043, 0x0063, 0x0043, 0x0063}},
    {"P_Gkey_35", {0x00BF, 0x00C0, 0x00E0, 0x0044, 0x0064, 0x0044, 0x0064}},
    {"P_Gkey_46", {0x0026, 0x00C8, 0x00E8, 0x0045, 0x0065, 0x0045, 0x0065}},
    {"P_Gkey_06", {0x007B, 0x0000, 0x0000, 0x0050, 0x0070, 0x0050, 0x0070}},
    {"P_Gkey_17", {0x007D, 0x0000, 0x0000, 0x0051, 0x0071, 0x0051, 0x0071}},
    {"P_Gkey_28", {0x0024, 0x0000, 0x0000, 0x0052, 0x0072, 0x0052, 0x0072}},
    {"P_Gkey_39", {0x0027, 0x0000, 0x0000, 0x0053, 0x0073, 0x0053, 0x0073}},
    {"P_Gkey_50", {0x0022, 0x0000, 0x0000, 0x0054, 0x0074, 0x0054, 0x0074}},
    {"P_Gkey_10", {0x002B, 0x0000, 0x0000, 0x002B, 0x002B, 0x0000, 0x0000}},
    {"P_Gkey_21", {0x002D, 0x0000, 0x0000, 0x002D, 0x002D, 0x0000, 0x0000}},
    {"P_Gkey_32", {0x003D, 0x0000, 0x0000, 0x003D, 0x003D, 0x0000, 0x0000}},
    {"P_Gkey_43", {0x005F, 0x0000, 0x0000, 0x005F, 0x005F, 0x0000, 0x0000}},
    {"P_Gkey_54", {0x0025, 0x0000, 0x0000, 0x002A, 0x002A, 0x0000, 0x0000}},
    {"P_Gkey_03", {0x002A, 0x00D9, 0x00F9, 0x0046, 0x0066, 0x0046, 0x0066}},
    {"P_Gkey_14", {0x003C, 0x00C9, 0x00E9, 0x0047, 0x0067, 0x0047, 0x0067}},
    {"P_Gkey_25", {0x003E, 0x00D1, 0x00F1, 0x0048, 0x0068, 0x0048, 0x0068}},
    {"P_Gkey_36", {0x007C, 0x00C7, 0x00E7, 0x0049, 0x0069, 0x0049, 0x0069}},
    {"P_Gkey_47", {0x002F, 0x00DF, 0x00FF, 0x004A, 0x006A, 0x004A, 0x006A}},
    {"P_Gkey_07", {0x005B, 0x0000, 0x0000, 0x0055, 0x0075, 0x0055, 0x0075}},
    {"P_Gkey_18", {0x005D, 0x0000, 0x0000, 0x0056, 0x0076, 0x0056, 0x0076}},
    {"P_Gkey_29", {0x0028, 0x0000, 0x0000, 0x0057, 0x0077, 0x0057, 0x0077}},
    {"P_Gkey_40", {0x0029, 0x0000, 0x0000, 0x0058, 0x0078, 0x0058, 0x0078}},
    {"P_Gkey_51", {0x0000, 0x0000, 0x0000, 0x0059, 0x0079, 0x0059, 0x0079}},
    {"P_Gkey_56", {0x0023, 0x0000, 0x0000, 0x005E, 0x005E, 0x0000, 0x0000}},
    {"P_Gkey_58", {0x00B2, 0x0000, 0x0000, 0x0027, 0x0027, 0x0000, 0x0000}},
    {"P_Gkey_60", {0x00B3, 0x0000, 0x0000, 0x0022, 0x0022, 0x0000, 0x0000}},
    {"P_Gkey_62", {0x0000, 0x0000, 0x0000, 0x003A, 0x003A, 0x0000, 0x0000}},
    {"P_Gkey_64", {0x0000, 0x0000, 0x0000, 0x003B, 0x003B, 0x0000, 0x0000}},
    {"P_Gkey_04", {0x0000, 0x0000, 0x0000, 0x004B, 0x006B, 0x004B, 0x006B}},
    {"P_Gkey_15", {0x0000, 0x0000, 0x0000, 0x004C, 0x006C, 0x004C, 0x006C}},
    {"P_Gkey_26", {0x0000, 0x0000, 0x0000, 0x004D, 0x006D, 0x004D, 0x006D}},
    {"P_Gkey_37", {0x0000, 0x0000, 0x0000, 0x004E, 0x006E, 0x004E, 0x006E}},
    {"P_Gkey_48", {0x0000, 0x0000, 0x0000, 0x004F, 0x006F, 0x004F, 0x006F}},
    {"P_Gkey_08", {0x0000, 0x0000, 0x0000, 0x005A, 0x007A, 0x005A, 0x007A}},
    {"P_Gkey_19", {0x0000, 0x0000, 0x0000, 0x002C, 0x002C, 0x0000, 0x0000}},
    {"P_Gkey_30", {0x0000, 0x0000, 0x0000, 0x002E, 0x002E, 0x0000, 0x0000}},
    {"P_Gkey_41", {0x0000, 0x0000, 0x0000, 0x0040, 0x0040, 0x0000, 0x0000}},
    {"P_Gkey_52", {0x0000, 0x0000, 0x0000, 0x0023, 0x0023, 0x0000, 0x0000}},
    {"P_Gkey_57", {0x0000, 0x0000, 0x0000, 0x0028, 0x0028, 0x0000, 0x0000}},
    {"P_Gkey_59", {0x0000, 0x0000, 0x0000, 0x0029, 0x0029, 0x0000, 0x0000}},
    {"P_Gkey_61", {0x0000, 0x0000, 0x0000, 0x003C, 0x003C, 0x0000, 0x0000}},
    {"P_Gkey_63", {0x0000, 0x0000, 0x0000, 0x003E, 0x003E, 0x0000, 0x0000}},
    {"P_Gkey_65", {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}},
};

/* The command keys of every layout (.rodata 0x80579C70). */
const HkbCommandKeyDef hkbCommandKeyDefs[27] = {
    {"P_key_LF", 0},
    {"P_key_DELETE", 1},
    {"P_key_CAPS", 3},
    {"P_key_SHIFT", 4},
    {"P_key_SPACE", 2},
    {"P_Gkey_LF", 0},
    {"P_Gkey_DELETE", 1},
    {"P_Gkey_SPACE", 2},
    {"P_Gkey_55", 31},
    {"W_USEU_prdc_lang", 10},
    {"W_USEU_Chng_sign", 13},
    {"W_JP_Chng_ABC", 11},
    {"W_JP_Chng_KANA", 12},
    {"W_JP_Chng_sign", 13},
    {"W_JP_Chng_action", 32},
    {"P_Mode_roma_hira", 16},
    {"P_Mode_roma_kata", 17},
    {"P_Mode_direct", 15},
    {"P_hiragana", 18},
    {"P_katakana", 19},
    {"P_Gkey_komoji", 22},
    {"P_Gkey_handaku", 21},
    {"P_Gkey_dakuten", 20},
    {"P_abc", 27},
    {"P_sign", 28},
    {"P_hanzen", 29},
    {"P_komoji", 30},
};

/* The panes a layout shows and hides (.rodata 0x80579EF8). */
const HkbPaneSwitch hkbPaneSwitch0 = {
    13,
    23,
    {
        "N_modeSelect_all",
        "P_Mode_direct",
        "P_Mode_roma_hira",
        "P_Mode_roma_kata",
        "N_KeyChange_JP",
        "W_JP_Chng_ABC",
        "W_JP_Chng_KANA",
        "W_JP_Chng_sign",
        "T_JP_Chng_ABC",
        "T_JP_Chng_KANA",
        "T_JP_Chng_sign",
        "T_key_CAPS",
        "T_key_SHIFT",
    },
    {
        "P_key_41",
        "T_key_41",
        "P_key_42",
        "T_key_42",
        "P_key_43",
        "T_key_43",
        "N_KeyChange_USEU",
        "P_Gkey_00",
        "P_Gkey_11",
        "P_Gkey_22",
        "P_Gkey_33",
        "P_Gkey_44",
        "T_Gkey_00",
        "T_Gkey_11",
        "T_Gkey_22",
        "T_Gkey_33",
        "T_Gkey_44",
        "W_USEU_prdc_lang",
        "W_USEU_Chng_sign",
        "T_USEU_prdc_lang",
        "T_USEU_Chng_sign",
        "P_SHIFTMark",
        "P_CAPSMark",
    },
};

/* The panes a layout shows and hides (.rodata 0x80579FBC). */
const HkbPaneSwitch hkbPaneSwitch1 = {
    15,
    21,
    {
        "N_KeyChange_JP",
        "P_Gkey_00",
        "P_Gkey_11",
        "P_Gkey_22",
        "P_Gkey_33",
        "P_Gkey_44",
        "T_Gkey_00",
        "T_Gkey_11",
        "T_Gkey_22",
        "T_Gkey_33",
        "T_Gkey_44",
        "T_key_CAPS",
        "T_key_SHIFT",
        "W_JP_Chng_sign",
        "T_JP_Chng_sign",
    },
    {
        "P_key_41",
        "T_key_41",
        "P_key_42",
        "T_key_42",
        "P_key_43",
        "T_key_43",
        "N_modeSelect_all",
        "P_Mode_direct",
        "P_Mode_roma_hira",
        "P_Mode_roma_kata",
        "N_KeyChange_USEU",
        "W_JP_Chng_ABC",
        "W_JP_Chng_KANA",
        "T_JP_Chng_ABC",
        "T_JP_Chng_KANA",
        "P_SHIFTMark",
        "P_CAPSMark",
        "W_USEU_prdc_lang",
        "W_USEU_Chng_sign",
        "T_USEU_prdc_lang",
        "T_USEU_Chng_sign",
    },
};

/* The panes a layout shows and hides (.rodata 0x8057A080). */
const HkbPaneSwitch hkbPaneSwitch2 = {
    19,
    17,
    {
        "P_key_42",
        "T_key_42",
        "P_key_43",
        "T_key_43",
        "N_KeyChange_JP",
        "P_Gkey_00",
        "P_Gkey_11",
        "P_Gkey_22",
        "P_Gkey_33",
        "P_Gkey_44",
        "T_Gkey_00",
        "T_Gkey_11",
        "T_Gkey_22",
        "T_Gkey_33",
        "T_Gkey_44",
        "T_key_CAPS",
        "T_key_SHIFT",
        "W_JP_Chng_sign",
        "T_JP_Chng_sign",
    },
    {
        "P_key_41",
        "T_key_41",
        "N_modeSelect_all",
        "P_Mode_direct",
        "P_Mode_roma_hira",
        "P_Mode_roma_kata",
        "N_KeyChange_USEU",
        "W_JP_Chng_ABC",
        "W_JP_Chng_KANA",
        "T_JP_Chng_ABC",
        "T_JP_Chng_KANA",
        "P_SHIFTMark",
        "P_CAPSMark",
        "W_USEU_prdc_lang",
        "W_USEU_Chng_sign",
        "T_USEU_prdc_lang",
        "T_USEU_Chng_sign",
    },
};

/* The panes a layout shows and hides (.rodata 0x8057A144). */
const HkbPaneSwitch hkbPaneSwitch6 = {
    19,
    17,
    {
        "P_key_42",
        "T_key_42",
        "P_key_43",
        "T_key_43",
        "N_KeyChange_JP",
        "P_Gkey_00",
        "P_Gkey_11",
        "P_Gkey_22",
        "P_Gkey_33",
        "P_Gkey_44",
        "T_Gkey_00",
        "T_Gkey_11",
        "T_Gkey_22",
        "T_Gkey_33",
        "T_Gkey_44",
        "P_SHIFTMark",
        "P_CAPSMark",
        "W_JP_Chng_sign",
        "T_JP_Chng_sign",
    },
    {
        "P_key_41",
        "T_key_41",
        "N_modeSelect_all",
        "P_Mode_direct",
        "P_Mode_roma_hira",
        "P_Mode_roma_kata",
        "N_KeyChange_USEU",
        "W_JP_Chng_ABC",
        "W_JP_Chng_KANA",
        "T_JP_Chng_ABC",
        "T_JP_Chng_KANA",
        "T_key_CAPS",
        "T_key_SHIFT",
        "W_USEU_prdc_lang",
        "W_USEU_Chng_sign",
        "T_USEU_prdc_lang",
        "T_USEU_Chng_sign",
    },
};

/* The panes a layout shows and hides (.rodata 0x8057A208). */
const HkbPaneSwitch hkbPaneSwitch3 = {
    21,
    15,
    {
        "P_key_41",
        "T_key_41",
        "P_key_42",
        "T_key_42",
        "P_key_43",
        "T_key_43",
        "N_KeyChange_JP",
        "P_Gkey_00",
        "P_Gkey_11",
        "P_Gkey_22",
        "P_Gkey_33",
        "P_Gkey_44",
        "T_Gkey_00",
        "T_Gkey_11",
        "T_Gkey_22",
        "T_Gkey_33",
        "T_Gkey_44",
        "W_JP_Chng_sign",
        "T_JP_Chng_sign",
        "P_SHIFTMark",
        "P_CAPSMark",
    },
    {
        "N_modeSelect_all",
        "P_Mode_direct",
        "P_Mode_roma_hira",
        "P_Mode_roma_kata",
        "N_KeyChange_USEU",
        "W_JP_Chng_ABC",
        "W_JP_Chng_KANA",
        "T_JP_Chng_ABC",
        "T_JP_Chng_KANA",
        "T_key_CAPS",
        "T_key_SHIFT",
        "W_USEU_prdc_lang",
        "W_USEU_Chng_sign",
        "T_USEU_prdc_lang",
        "T_USEU_Chng_sign",
    },
};

/* The panes a layout shows and hides (.rodata 0x8057A2CC). */
const HkbPaneSwitch hkbPaneSwitch7 = {
    17,
    19,
    {
        "P_key_43",
        "T_key_43",
        "N_KeyChange_JP",
        "P_Gkey_00",
        "P_Gkey_11",
        "P_Gkey_22",
        "P_Gkey_33",
        "P_Gkey_44",
        "T_Gkey_00",
        "T_Gkey_11",
        "T_Gkey_22",
        "T_Gkey_33",
        "T_Gkey_44",
        "T_key_CAPS",
        "T_key_SHIFT",
        "W_JP_Chng_sign",
        "T_JP_Chng_sign",
    },
    {
        "P_key_41",
        "T_key_41",
        "P_key_42",
        "T_key_42",
        "N_modeSelect_all",
        "P_Mode_direct",
        "P_Mode_roma_hira",
        "P_Mode_roma_kata",
        "N_KeyChange_USEU",
        "W_JP_Chng_ABC",
        "W_JP_Chng_KANA",
        "T_JP_Chng_ABC",
        "T_JP_Chng_KANA",
        "P_SHIFTMark",
        "P_CAPSMark",
        "W_USEU_prdc_lang",
        "W_USEU_Chng_sign",
        "T_USEU_prdc_lang",
        "T_USEU_Chng_sign",
    },
};

/* The accented forms of the vowels a layout offers (.rodata 0x8057A390). */
const u16 hkbAccents6[12][8] = {
    {0x0061, 0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x0000, 0x0000},
    {0x0069, 0x00EC, 0x00ED, 0x00EE, 0x0000, 0x00EF, 0x0000, 0x0000},
    {0x0075, 0x00F9, 0x00FA, 0x00FB, 0x0000, 0x00FC, 0x0000, 0x0000},
    {0x0065, 0x00E8, 0x00E9, 0x00EA, 0x0000, 0x00EB, 0x0000, 0x0000},
    {0x006F, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x0000, 0x0000},
    {0x0041, 0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x0000, 0x0000},
    {0x0049, 0x00CC, 0x00CD, 0x00CE, 0x0000, 0x00CF, 0x0000, 0x0000},
    {0x0055, 0x00D9, 0x00DA, 0x00DB, 0x0000, 0x00DC, 0x0000, 0x0000},
    {0x0045, 0x00C8, 0x00C9, 0x00CA, 0x0000, 0x00CB, 0x0000, 0x0000},
    {0x004F, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x0000, 0x0000},
    {0x0079, 0x0000, 0x00FD, 0x0000, 0x0000, 0x00FF, 0x0000, 0x0000},
    {0x0059, 0x0000, 0x00DD, 0x0000, 0x0000, 0x0178, 0x0000, 0x0000},
};

/* The accented forms of the vowels a layout offers (.rodata 0x8057A450). */
const u16 hkbAccents7[12][8] = {
    {0x0061, 0x00E0, 0x0000, 0x00E2, 0x00E3, 0x0000, 0x00E1, 0x00E4},
    {0x0069, 0x00EC, 0x0000, 0x00EE, 0x0000, 0x0000, 0x00ED, 0x00EF},
    {0x0075, 0x00F9, 0x0000, 0x00FB, 0x0000, 0x0000, 0x00FA, 0x00FC},
    {0x0065, 0x00E8, 0x0000, 0x00EA, 0x0000, 0x0000, 0x00E9, 0x00EB},
    {0x006F, 0x00F2, 0x0000, 0x00F4, 0x00F5, 0x0000, 0x00F3, 0x00F6},
    {0x0041, 0x00C0, 0x0000, 0x00C2, 0x00C3, 0x0000, 0x00C1, 0x00C4},
    {0x0049, 0x00CC, 0x0000, 0x00CE, 0x0000, 0x0000, 0x00CD, 0x00CF},
    {0x0055, 0x00D9, 0x0000, 0x00DB, 0x0000, 0x0000, 0x00DA, 0x00DC},
    {0x0045, 0x00C8, 0x0000, 0x00CA, 0x0000, 0x0000, 0x00C9, 0x00CB},
    {0x004F, 0x00D2, 0x0000, 0x00D4, 0x00D5, 0x0000, 0x00D3, 0x00D6},
    {0x0079, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00FD, 0x00FF},
    {0x0059, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00DD, 0x0178},
};

/* The accented forms of the vowels a layout offers (.rodata 0x8057A510). */
const u16 hkbAccents4[12][8] = {
    {0x0061, 0x00E0, 0x00E1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0069, 0x00EC, 0x00ED, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0075, 0x00F9, 0x00FA, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0065, 0x00E8, 0x00E9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x006F, 0x00F2, 0x00F3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0041, 0x00C0, 0x00C1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0049, 0x00CC, 0x00CD, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0055, 0x00D9, 0x00DA, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0045, 0x00C8, 0x00C9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x004F, 0x00D2, 0x00D3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0079, 0x0000, 0x00FD, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0059, 0x0000, 0x00DD, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
};

/* The accented forms of the vowels a layout offers (.rodata 0x8057A5D0). */
const u16 hkbAccents3[12][8] = {
    {0x0061, 0x0000, 0x0000, 0x00E2, 0x0000, 0x00E4, 0x0000, 0x0000},
    {0x0069, 0x0000, 0x0000, 0x00EE, 0x0000, 0x00EF, 0x0000, 0x0000},
    {0x0075, 0x0000, 0x0000, 0x00FB, 0x0000, 0x00FC, 0x0000, 0x0000},
    {0x0065, 0x0000, 0x0000, 0x00EA, 0x0000, 0x00EB, 0x0000, 0x0000},
    {0x006F, 0x0000, 0x0000, 0x00F4, 0x0000, 0x00F6, 0x0000, 0x0000},
    {0x0041, 0x0000, 0x0000, 0x00C2, 0x0000, 0x00C4, 0x0000, 0x0000},
    {0x0049, 0x0000, 0x0000, 0x00CE, 0x0000, 0x00CF, 0x0000, 0x0000},
    {0x0055, 0x0000, 0x0000, 0x00DB, 0x0000, 0x00DC, 0x0000, 0x0000},
    {0x0045, 0x0000, 0x0000, 0x00CA, 0x0000, 0x00CB, 0x0000, 0x0000},
    {0x004F, 0x0000, 0x0000, 0x00D4, 0x0000, 0x00D6, 0x0000, 0x0000},
    {0x0079, 0x0000, 0x0000, 0x0000, 0x0000, 0x00FF, 0x0000, 0x0000},
    {0x0059, 0x0000, 0x0000, 0x0000, 0x0000, 0x0178, 0x0000, 0x0000},
};

/* The keyboard layouts (.rodata 0x8057A690). */
const HkbKeyLayout hkbKeyLayouts[8] = {
    {hkb_qwerty_map_english_us, hkbKanaKeyDefs, hkbCommandKeyDefs, &hkbPaneSwitch0, NULL},
    {hkb_qwerty_map_english_us, hkbKanaKeyDefs, hkbCommandKeyDefs, &hkbPaneSwitch1, NULL},
    {hkb_qwerty_map_english_uk, hkbKanaKeyDefs, hkbCommandKeyDefs, &hkbPaneSwitch2, NULL},
    {hkb_qwerty_map_french, hkbKanaKeyDefs, hkbCommandKeyDefs, &hkbPaneSwitch3, hkbAccents3},
    {hkb_qwerty_map_german, hkbKanaKeyDefs, hkbCommandKeyDefs, &hkbPaneSwitch3, hkbAccents4},
    {hkb_qwerty_map_italian, hkbKanaKeyDefs, hkbCommandKeyDefs, &hkbPaneSwitch3, NULL},
    {hkb_qwerty_map_spanish, hkbKanaKeyDefs, hkbCommandKeyDefs, &hkbPaneSwitch6, hkbAccents6},
    {hkb_qwerty_map_dutch, hkbKanaKeyDefs, hkbCommandKeyDefs, &hkbPaneSwitch7, hkbAccents7},
};

/* The symbol layout (.rodata 0x8057A730). */
const HkbKeyLayout hkbSymbolLayout = {hkb_qwerty_map_japanese, hkbKanaKeyDefs, hkbCommandKeyDefs, &hkbPaneSwitch0, NULL};

/* The key animations (.rodata 0x8057A748). */
const HkbAnimDef hkbKeyAnimDefs[16] = {
    {0, "fs_VK_ascii_keytop_a_normal.brlan"},
    {1, "fs_VK_ascii_keytop_a_Focus-IN.brlan"},
    {2, "fs_VK_ascii_keytop_a_Focus-OUT.brlan"},
    {3, "fs_VK_ascii_keytop_a_Roll_over.brlan"},
    {4, "fs_VK_ascii_keytop_a_Pushed.brlan"},
    {5, "fs_VK_ascii_keytop_a_toggle-ON.brlan"},
    {6, "fs_VK_ascii_keytop_a_toggle-OFF.brlan"},
    {7, "fs_VK_ascii_keytop_a_Target_ON.brlan"},
    {8, "fs_VK_ascii_keytop_a_toggleON_Focus-IN.brlan"},
    {9, "fs_VK_ascii_keytop_a_toggleON_Focus-OUT.brlan"},
    {10, "fs_VK_ascii_keytop_a_normal_toggle-ON.brlan"},
    {11, "fs_VK_ascii_keytop_a_toggleON_Pushed.brlan"},
    {12, "fs_VK_ascii_keytop_a_active-ON.brlan"},
    {13, "fs_VK_ascii_keytop_a_active-OFF.brlan"},
    {14, "fs_VK_ascii_keytop_a_active_normal.brlan"},
    {15, "fs_VK_ascii_keytop_a_not_active_normal.brlan"},
};

/* The key whose animations the character keys share. */
const char* hkbKeyAnimGroup = "P_key_00";

/* The key definitions (.data 0x8064B180). */
HkbKeyDef hkbKeyDefs[142] = {
    {0, "P_key_00", 5, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_01", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_02", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_03", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_04", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_05", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_06", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_07", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_08", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_09", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_10", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_11", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_12", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_13", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_15", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_16", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_17", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_18", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_14", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_19", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_20", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_21", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_22", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_23", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_24", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_25", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_26", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_27", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_28", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_29", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_30", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_31", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_32", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_33", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_34", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_35", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_36", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_37", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_38", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_39", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_40", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_41", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_42", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_43", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_44", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_45", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_46", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_47", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_48", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_49", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_DELETE", 5, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_LF", 5, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_key_SPACE", 5, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {1, "P_key_SHIFT", 12, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[7], &hkbKeyAnimDefs[8], &hkbKeyAnimDefs[9], &hkbKeyAnimDefs[10], &hkbKeyAnimDefs[11]}},
    {1, "P_key_CAPS", 12, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[7], &hkbKeyAnimDefs[8], &hkbKeyAnimDefs[9], &hkbKeyAnimDefs[10], &hkbKeyAnimDefs[11]}},
    {0, "P_Gkey_00", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_01", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_02", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_03", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_04", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_05", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_06", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_07", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_08", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_09", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_10", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_11", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_12", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_13", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_14", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_15", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_16", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_17", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_18", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_19", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_20", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_21", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_22", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_23", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_24", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_25", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_26", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_27", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_28", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_29", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_30", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_31", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_32", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_33", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_34", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_35", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_36", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_37", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_38", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_39", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_40", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_41", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_42", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_43", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_44", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_45", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_46", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_47", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_48", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_49", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_50", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_51", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_52", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_53", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_54", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_55", 5, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_56", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_57", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_58", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_59", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_60", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_61", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_62", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_63", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_64", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_65", 5, hkbKeyAnimGroup, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_DELETE", 5, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_LF", 5, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "P_Gkey_SPACE", 5, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "W_USEU_prdc_lang", 5, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {0, "W_USEU_Chng_sign", 5, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {2, "W_JP_Chng_ABC", 7, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[7]}},
    {2, "W_JP_Chng_KANA", 7, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[7]}},
    {0, "W_JP_Chng_sign", 5, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
    {2, "P_hiragana", 7, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[7]}},
    {2, "P_katakana", 7, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[7]}},
    {3, "P_Gkey_dakuten", 11, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[12], &hkbKeyAnimDefs[13], &hkbKeyAnimDefs[14], &hkbKeyAnimDefs[15]}},
    {3, "P_Gkey_handaku", 11, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[12], &hkbKeyAnimDefs[13], &hkbKeyAnimDefs[14], &hkbKeyAnimDefs[15]}},
    {3, "P_Gkey_komoji", 11, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[12], &hkbKeyAnimDefs[13], &hkbKeyAnimDefs[14], &hkbKeyAnimDefs[15]}},
    {2, "P_Mode_roma_hira", 7, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[7]}},
    {2, "P_Mode_roma_kata", 7, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[7]}},
    {2, "P_Mode_direct", 7, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[7]}},
    {2, "P_abc", 7, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[7]}},
    {2, "P_sign", 7, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[7]}},
    {3, "P_hanzen", 11, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[12], &hkbKeyAnimDefs[13], &hkbKeyAnimDefs[14], &hkbKeyAnimDefs[15]}},
    {3, "P_komoji", 11, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5], &hkbKeyAnimDefs[6], &hkbKeyAnimDefs[12], &hkbKeyAnimDefs[13], &hkbKeyAnimDefs[14], &hkbKeyAnimDefs[15]}},
    {0, "W_JP_Chng_action", 5, NULL, {&hkbKeyAnimDefs[0], &hkbKeyAnimDefs[1], &hkbKeyAnimDefs[2], &hkbKeyAnimDefs[3], &hkbKeyAnimDefs[4], &hkbKeyAnimDefs[5]}},
};

/* 0x80535E3C (0x8): Keeps the allocator the keyboard's objects come from. */
void HkbKeyboardBase::create(MEMAllocator* pAllocator) {
    mpAllocator = pAllocator;
}

/* 0x80535E44 (0x108): Resets the status, selects the layout of the current type and announces the case mode. */
void HkbKeyboardBase::init() {
    mStatus.bInputDone = 0;
    mpLayout = &hkbKeyLayouts[getLayoutType()];
    mKeyState.mbFullWidth = 0;
    if (mKeyState.mLayoutIndex == 0 && mKeyState.mpOwner->mbActive == 0) {
        mKeyState.mFlags = 1;
    } else {
        mKeyState.mFlags = 0;
    }
    mKeyState.mKanaFlags = 0;
    mKeyState.update();
    HkbModeEvent ev;
    ev.mode = mKeyState.getMode();
    sendEvent(0x12, &ev);
}

/* 0x80535F4C (0x8): Returns the layout type. */
s32 HkbInputWidget::getLayoutType() {
    return mLayoutType;
}

/* 0x80535F54 (0x20): Passes an event to the event sink, when one is set. */
/* untyped: caller-owned payload - the argument record of event `id` */
void HkbInputWidget::sendEvent(u32 id, void* data) {
    if (mpEventSink != NULL) {
        mpEventSink->onEvent(id, data);
    }
}

/* 0x80535F74 (0x14): Types one character with no flags. */
void HkbKeyboardBase::inputChar(u16 c) {
    inputChar(c, 0);
}

/* 0x80535F88 (0x604): Handles a press on a key pane: types its character, or runs its command. */
void HkbKeyboardBase::onPaneEvent(u32 event, const char* paneName) {
    u16 c = findCharByName(paneName);
    if (event == 4) {
        if (c == 0) {
            switch (findCommandByName(paneName)) {
            case 1:
                sendEvent(1, NULL);
                break;
            case 0:
                sendEvent(7, NULL);
                sendEvent(0x27, NULL);
                break;
            case 2:
                inputChar(0x20, 0);
                break;
            case 31:
                if (mpContext->v_0x068()->v_0x0E0()->v_0x0E4()) {
                    sendEvent(7, NULL);
                } else if (mpContext->v_0x068()->v_0x140()) {
                    sendEvent(0x28, NULL);
                } else {
                    sendEvent(0x2E, NULL);
                }
                break;
            case 11:
                setHalfWidth(1);
                break;
            case 12:
                setHalfWidth(0);
                break;
            case 13:
                setCursor(0);
                break;
            case 32:
                setCursor(1);
                break;
            case 10:
                submit();
                break;
            case 14:
                setKanaLock(4);
                break;
            case 15: {
                HkbModeEvent ev = {0};
                sendEvent(0x12, &ev);
                setMode(0);
                break;
            }
            case 16: {
                HkbModeEvent ev = {1};
                sendEvent(0x12, &ev);
                setMode(1);
                break;
            }
            case 17: {
                HkbModeEvent ev = {2};
                sendEvent(0x12, &ev);
                setMode(2);
                break;
            }
            case 18:
                mKeyState.setKanaMode(0);
                HkbKeyboardBase::setMode(1);
                if (mpContext->v_0x090()->v_0x034() == 0) {
                    mpContext->v_0x090()->v_0x028();
                }
                break;
            case 19:
                mKeyState.setKanaMode(5);
                mKeyState.toggleVoiced();
                HkbKeyboardBase::setMode(0);
                break;
            case 27:
                mKeyState.setKanaMode(6);
                mKeyState.toggleVoiced();
                HkbKeyboardBase::setMode(0);
                break;
            case 28:
                mKeyState.setKanaMode(3);
                HkbKeyboardBase::setMode(0);
                break;
            case 29:
                mKeyState.toggleSmall();
                switch (mKeyState.mKanaFlags & 0xF) {
                case 5:
                case 6:
                case 13:
                case 14:
                    break;
                default:
                    if (mpContext->v_0x090()->v_0x034() == 0) {
                        mpContext->v_0x090()->v_0x028();
                    }
                    break;
                }
                break;
            case 30:
                mKeyState.toggleVoiced();
                break;
            case 20:
                if (mKeyState.mKanaFlags & 0x20) {
                    sendEvent(0x19, NULL);
                }
                break;
            case 21:
                if (mKeyState.mKanaFlags & 0x40) {
                    sendEvent(0x1A, NULL);
                }
                break;
            case 22:
                if (mKeyState.mKanaFlags & 0x80) {
                    sendEvent(0x1C, NULL);
                }
                break;
            }
        } else {
            inputChar(c);
        }
    }
}

/* 0x805365CC (0x4): Moves the text cursor (nothing to do in the common part). */
void HkbKeyboardBase::setCursor(u32 position) {}

/* 0x805365D0 (0x4): Submits the text (nothing to do in the common part). */
void HkbKeyboardBase::submit() {}

/* 0x805365E4 (0xEC): Runs a command: queries the text, clears the kana flags unless the command keeps them, and
 * marks the input done on command 0x24. */
void HkbKeyboardBase::onCommand(u32 command, u32 arg) {
    if (command != 0x1F) {
        HkbTextQuery query;
        query.value = 0;
        *(u32*)&query.bFlag = 0;
        sendEvent(0x1F, &query);
        if ((command != 0x19 && command != 0x1A && command != 0x1C && command != 0x18 && command != 0x14 &&
             command != 0x13 && command != 0xF && command != 0x10 && command != 0x2A && command != 0x2B &&
             command != 0x23) ||
            command == 0x24) {
            mKeyState.clearKanaFlags();
        }
        if (command == 0x24) {
            mStatus.bInputDone = 1;
        }
    }
}

/* 0x805366D0 (0x208): Announces the case mode and the case/kana lamp state, then refreshes the enter key. */
void HkbKeyboardBase::updateView() {
    HkbModeEvent ev;
    u8 bCaseLamp;
    u8 bKanaLamp;
    ev.mode = mKeyState.getMode();
    sendEvent(0x12, &ev);
    sendEvent(0x27, NULL);
    if (mKeyState.mbFullWidth == 0) {
        switch (mKeyState.getMode()) {
        case 1:
            bCaseLamp = 1;
            sendEvent(0x13, &bCaseLamp);
            break;
        case 2:
            bCaseLamp = 0;
            sendEvent(0x13, &bCaseLamp);
            break;
        }
    } else {
        switch (mKeyState.getKanaFrame()) {
        case 6:
            bKanaLamp = 1;
            sendEvent(0x13, &bKanaLamp);
            break;
        case 7:
            bKanaLamp = 0;
            sendEvent(0x13, &bKanaLamp);
            break;
        }
    }
    updateEnterKey();
}

/* 0x805368D8 (0x4): Closes the keyboard (nothing to do in the common part). */
void HkbKeyboardBase::close() {}

/* 0x805368DC (0x8): Returns the character of the key pane `paneName`, or 0. */
u16 HkbKeyboardBase::findCharByName(const char* paneName) {
    return mKeyState.findChar(paneName);
}

/* 0x805368E4 (0x90): Returns the command of the key pane `paneName`, or 33 when it is no command key. */
u32 HkbKeyboardBase::findCommandByName(const char* paneName) {
    for (u16 i = 0; i < 27; i++) {
        if (hbmNameMatches(mKeyState.mpLayout->commandKeys[i].paneName, paneName)) {
            return mKeyState.mpLayout->commandKeys[i].command;
        }
    }
    return 33;
}

/* 0x80536974 (0x114): Sets the case lock (0 none, 1 shift, 2 caps, 3 both). */
void HkbKeyboardBase::setCaseLock(s32 lock) {
    switch (lock) {
    case 0:
        mKeyState.setCaseFlags(0);
        break;
    case 1:
        mKeyState.setCaseFlags(0x40);
        break;
    case 2:
        mKeyState.setCaseFlags(0x80);
        break;
    case 3:
        mKeyState.setCaseFlags(0xC0);
        break;
    }
}

/* 0x80536A88 (0x58): Sets the kana lock: 6 clears the kana sub-mode, 7 selects sub-mode 1. */
void HkbKeyboardBase::setKanaLock(s32 lock) {
    switch (lock) {
    case 6:
        mKeyState.setKanaMode(0);
        break;
    case 7:
        mKeyState.setKanaMode(1);
        break;
    }
}

/* 0x80536AE0 (0x10): Moves the text cursor. */
void HkbKeyboardBase::selectCursor(u32 position) {
    setCursor(position);
}

/* 0x80536AF0 (0x1C8): Types one character: sends it with the case flags, drops the caps lock when the keyboard's
 * LED state asks for it, and recomputes the kana marks the character accepts. */
void HkbKeyboardBase::inputChar(u16 c, u32 flags) {
    mStatus.bInputDone = 0;
    u32 caseFlags = 0;
    if (mKeyState.mFlags & 0x40) {
        caseFlags |= 2;
    }
    if (mKeyState.mFlags & 0x80) {
        caseFlags |= 1;
    }
    HkbCharEvent ev = {c, 0, caseFlags, 1, 0, 0};
    sendEvent(0, &ev);
    if (mKeyState.mFlags & 0x80) {
        if (!hkbGetKbdState().isShiftHeld()) {
            mKeyState.clearShift();
        }
    }
    if (mStatus.bInputDone == 0) {
        u32 kanaFlags = 0;
        if (c != hbmKanaToggleDakuten(c) && c != 0x3063 && c != 0x30A5 && c != 0x30C3) {
            kanaFlags |= 0x20;
        }
        if (c != hbmKanaToggleHandakuten(c)) {
            kanaFlags |= 0x40;
        }
        if (c != hbmKanaToggleSmall(c)) {
            kanaFlags |= 0x80;
        }
        if ((mKeyState.mKanaFlags & ~0xF) != kanaFlags) {
            mKeyState.mKanaFlags = (mKeyState.mKanaFlags & 0xF) | (kanaFlags & ~0xF);
            mKeyState.update();
        }
    }
}

/* 0x80536CB8 (0x10): Sets the layout type and the layout index it selects. */
void HkbKeyboardBase::setLayoutType(s32 type) {
    mLayoutType = type;
    mKeyState.mLayoutIndex = type;
    mKeyState.update();
}

/* 0x80536CC8 (0xF0): Lights the enter key when the context accepts input and the keyboard is idle. */
void HkbKeyboardBase::updateEnterKey() {
    if (mpContext->v_0x098() == NULL || mpContext->v_0x098()->v_0x010() != 0) {
        switch (getLayoutType()) {
        case 0: {
            u8 bEnable = 0;
            if (isHalfWidth() && (mKeyState.mFlags & 0xF) == 0) {
                bEnable = 1;
            } else {
                bEnable = 0;
            }
            sendEvent(0x14, &bEnable);
            break;
        }
        }
    }
}

/* 0x80536DC0 (0x64): Switches between half-width and full-width input. */
void HkbKeyboardBase::setHalfWidth(u32 bHalfWidth) {
    u32 bFullWidth = bHalfWidth == 0;
    if (bFullWidth != mKeyState.mbFullWidth) {
        mKeyState.mbFullWidth = bFullWidth;
        mKeyState.update();
    }
    sendEvent(0x29, NULL);
}

/* 0x80536E24 (0xE4): Selects input mode `mode` (0, 1 or 2), announcing the change and resetting the layout. */
void HkbKeyboardBase::setMode(s32 mode) {
    u32 caseMode;
    switch (mode) {
    case 0:
        caseMode = 0;
        break;
    case 1:
        caseMode = 1;
        break;
    case 2:
        caseMode = 2;
        break;
    }
    if (caseMode != mKeyState.getModeBits()) {
        mKeyState.setMode(caseMode);
        HkbModeEvent ev;
        ev.mode = mode;
        sendEvent(0x12, &ev);
        mpLayout = &hkbSymbolLayout;
        sendEvent(0x29, NULL);
    }
}

#pragma push
#pragma auto_inline on

/* Shows the small-kana label of the voiced-mark state. */
static void updateSmallLabel(HkbKeyboard* pKeyboard) {
    if (pKeyboard->mKeyState.mKanaFlags & 8) {
        pKeyboard->setPaneVisible("T_komoji", 0);
        pKeyboard->setPaneVisible("T_omoji", 1);
    } else {
        pKeyboard->setPaneVisible("T_komoji", 1);
        pKeyboard->setPaneVisible("T_omoji", 0);
    }
}

/* Shows the width label of the small-kana state and its button. */
static void updateWidthLabel(HkbKeyboard* pKeyboard) {
    if (pKeyboard->mKeyState.mKanaFlags & 4) {
        pKeyboard->setPaneVisible("T_hankaku", 0);
        pKeyboard->setPaneVisible("T_zenkaku", 1);
    } else {
        pKeyboard->setPaneVisible("T_hankaku", 1);
        pKeyboard->setPaneVisible("T_zenkaku", 0);
    }
    pKeyboard->setPaneVisible("B_hanzen", 1);
}

/* Plays the kana mark keys' animations for the marks the last character accepts. */
static void updateMarkAnims(HkbKeyboard* pKeyboard) {
    if (pKeyboard->mKeyState.mKanaFlags & 0x20) {
        pKeyboard->getPaneAnim("P_Gkey_dakuten")->setState(14);
    } else {
        pKeyboard->getPaneAnim("P_Gkey_dakuten")->setState(15);
    }
    if (pKeyboard->mKeyState.mKanaFlags & 0x40) {
        pKeyboard->getPaneAnim("P_Gkey_handaku")->setState(14);
    } else {
        pKeyboard->getPaneAnim("P_Gkey_handaku")->setState(15);
    }
    if (pKeyboard->mKeyState.mKanaFlags & 0x80) {
        pKeyboard->getPaneAnim("P_Gkey_komoji")->setState(14);
    } else {
        pKeyboard->getPaneAnim("P_Gkey_komoji")->setState(15);
    }
}

#pragma pop

/* 0x80536F08 (0xC24): Switches between half-width and full-width input and rebuilds the mode, kana and label
 * panes for the new width. */
void HkbKeyboard::setHalfWidth(u32 bHalfWidth) {
    resetKeys();
    if (bHalfWidth != isHalfWidth()) {
        if (bHalfWidth != 0) {
            getPaneAnim("W_JP_Chng_ABC")->setState(4);
            getPaneAnim("W_JP_Chng_KANA")->setState(6);
            switch (mKeyState.getModeBits()) {
            case 0:
            default:
                getPaneAnim("P_Mode_roma_hira")->setState(0);
                getPaneAnim("P_Mode_roma_kata")->setState(0);
                getPaneAnim("P_Mode_direct")->setState(5);
                break;
            case 1:
                getPaneAnim("P_Mode_roma_hira")->setState(5);
                getPaneAnim("P_Mode_roma_kata")->setState(0);
                getPaneAnim("P_Mode_direct")->setState(0);
                break;
            case 2:
                getPaneAnim("P_Mode_roma_hira")->setState(0);
                getPaneAnim("P_Mode_roma_kata")->setState(5);
                getPaneAnim("P_Mode_direct")->setState(0);
                break;
            }
        } else {
            getPaneAnim("W_JP_Chng_ABC")->setState(6);
            getPaneAnim("W_JP_Chng_KANA")->setState(4);
            u32 kanaType = mKeyState.mKanaFlags & 3;
            if (kanaType == 0) {
                HkbKeyboardBase::setMode(1);
                getPaneAnim("P_hiragana")->setState(5);
                getPaneAnim("P_katakana")->setState(0);
                getPaneAnim("P_abc")->setState(0);
                getPaneAnim("P_sign")->setState(0);
                getPaneAnim("P_komoji")->setState(14);
                getPaneAnim("P_hanzen")->setState(15);
                updateSmallLabel(this);
                setPaneVisible("B_komoji", 1);
                if (mPanelKind != 14) {
                    setPaneVisible("T_hankaku", 0);
                    setPaneVisible("T_zenkaku", 0);
                    setPaneVisible("B_hanzen", 0);
                }
            } else if (kanaType == 1) {
                HkbKeyboardBase::setMode(0);
                getPaneAnim("P_hiragana")->setState(0);
                getPaneAnim("P_katakana")->setState(5);
                getPaneAnim("P_abc")->setState(0);
                getPaneAnim("P_sign")->setState(0);
                getPaneAnim("P_komoji")->setState(14);
                updateSmallLabel(this);
                setPaneVisible("B_komoji", 1);
            } else if (kanaType == 2) {
                HkbKeyboardBase::setMode(0);
                getPaneAnim("P_hiragana")->setState(0);
                getPaneAnim("P_katakana")->setState(0);
                getPaneAnim("P_abc")->setState(5);
                getPaneAnim("P_sign")->setState(0);
                getPaneAnim("P_komoji")->setState(14);
                getPaneAnim("P_hanzen")->setState(15);
                if (mInputStyle == 1) {
                    getPaneAnim("P_komoji")->setState(15);
                    setPaneVisible("T_omoji", 0);
                    setPaneVisible("T_komoji", 0);
                    setPaneVisible("B_komoji", 0);
                } else {
                    updateSmallLabel(this);
                    setPaneVisible("B_komoji", 1);
                }
            } else if (kanaType == 3) {
                HkbKeyboardBase::setMode(0);
                getPaneAnim("P_hiragana")->setState(0);
                getPaneAnim("P_katakana")->setState(0);
                getPaneAnim("P_abc")->setState(0);
                getPaneAnim("P_sign")->setState(5);
                getPaneAnim("P_komoji")->setState(15);
                getPaneAnim("P_hanzen")->setState(15);
                setPaneVisible("T_komoji", 0);
                setPaneVisible("T_omoji", 0);
                setPaneVisible("B_komoji", 0);
                if (mPanelKind != 14) {
                    setPaneVisible("T_hankaku", 0);
                    setPaneVisible("T_zenkaku", 0);
                    setPaneVisible("B_hanzen", 0);
                }
            }
            updateMarkAnims(this);
        }
    }
    mKeyState.setFullWidthInline(bHalfWidth == 0);
    sendEvent(0x29, NULL);
}

#pragma auto_inline on

/* 0x80537B2C (0x320): Selects the layout table for the case mode and pushes the case and lamp state to the
 * keyboard. */
void HkbKeyState::update() {
    u8 bEnterKey;
    u8 bLamp;
    if (mLayoutIndex != 0 || mpOwner->mbActive) {
        setMode(0);
    }
    if ((mFlags & 0xF) == 0) {
        mpLayout = &hkbKeyLayouts[mLayoutIndex];
    } else {
        mpLayout = &hkbSymbolLayout;
    }
    if (mbFullWidth == 0) {
        switch (mFlags & 0xF) {
        case 0:
            bEnterKey = 1;
            mpOwner->sendEvent(0x14, &bEnterKey);
            break;
        case 1:
            bEnterKey = 0;
            bLamp = 1;
            mpOwner->sendEvent(0x14, &bEnterKey);
            mpOwner->sendEvent(0x13, &bLamp);
            break;
        case 2:
            bEnterKey = 0;
            bLamp = 0;
            mpOwner->sendEvent(0x14, &bEnterKey);
            mpOwner->sendEvent(0x13, &bLamp);
            break;
        }
    } else {
        switch (mFlags & 0xF) {
        case 0: {
            u32 kanaMode = mKanaFlags & 0xF;
            bEnterKey = 0;
            bLamp = 0;
            mpOwner->sendEvent(0x14, &bEnterKey);
            if (kanaMode & 4) {
                mpOwner->sendEvent(0x30, &bLamp);
            } else {
                mpOwner->sendEvent(0x13, &bLamp);
            }
            break;
        }
        case 1:
        case 2: {
            u32 kanaMode = mKanaFlags & 0xF;
            switch (kanaMode) {
            case 0:
            case 8:
                bEnterKey = 0;
                bLamp = 1;
                mpOwner->sendEvent(0x14, &bEnterKey);
                mpOwner->sendEvent(0x13, &bLamp);
                break;
            case 1:
            case 5:
            case 9:
            case 13:
                bEnterKey = 0;
                bLamp = 0;
                mpOwner->sendEvent(0x14, &bEnterKey);
                if (kanaMode & 4) {
                    mpOwner->sendEvent(0x2F, &bLamp);
                } else {
                    mpOwner->sendEvent(0x13, &bLamp);
                }
                break;
            }
            break;
        }
        }
    }
    if (mpOwner != NULL) {
        mpOwner->onStateChanged();
    }
}

/* 0x80537E4C (0x4): Does nothing when the keyboard state changes (the common part has no view). */
void HkbKeyboardBase::onStateChanged() {}

/* 0x80537E50 (0x3D0): Writes the layout's characters into the key text panes under `pRoot`, hiding the kana keys
 * the current column leaves empty. */
void HkbKeyState::setupPanes(nw4r::lyt::Pane* pRoot) {
    for (u16 i = 0; i < 50; i++) {
        char textName[17];
        hbmNameCopyWithChar(textName, sizeof(textName), mpLayout->charKeys[i].name, 0, 'T');
        nw4r::lyt::TextBox* pText =
            nw4r::ut::DynamicCast<nw4r::lyt::TextBox>(pRoot->FindPaneByName(textName, true));
        if (pText != NULL) {
            wchar_t str[2];
            str[0] = getChar(i);
            str[1] = 0;
            pText->SetString(str, 0);
        }
    }
    for (u16 j = 0; j < 65; j++) {
        char name[17];
        hbmNameCopyWithChar(name, sizeof(name), mpLayout->kanaKeys[j].paneName, 0, 'T');
        nw4r::lyt::TextBox* pText = nw4r::ut::DynamicCast<nw4r::lyt::TextBox>(pRoot->FindPaneByName(name, true));
        if (pText != NULL) {
            wchar_t str[2];
            str[0] = mpLayout->kanaKeys[j].chars[getKanaColumn()];
            str[1] = 0;
            if (str[0] == 0) {
                pText->SetVisible(false);
                hbmNameCopyWithChar(name, sizeof(name), mpLayout->kanaKeys[j].paneName, 0, 'B');
                nw4r::lyt::Pane* pBack = pRoot->FindPaneByName(name, true);
                if (pBack != NULL) {
                    pBack->SetVisible(false);
                }
            } else {
                pText->SetString(str, 0);
                if (!pText->IsVisible()) {
                    pText->SetVisible(true);
                    hbmNameCopyWithChar(name, sizeof(name), mpLayout->kanaKeys[j].paneName, 0, 'B');
                    nw4r::lyt::Pane* pBack = pRoot->FindPaneByName(name, true);
                    if (pBack != NULL) {
                        pBack->SetVisible(true);
                    }
                }
            }
        }
    }
}

/* 0x80538228 (0x40): Sets the case flags above the input mode, telling the keyboard on a change. */
void HkbKeyState::setCaseFlags(u32 flags) {
    if ((mFlags & ~0xF) != flags) {
        mFlags = (mFlags & 0xF) | (flags & ~0xF);
        if (mpOwner != NULL) {
            mpOwner->onStateChanged();
        }
    }
}

/* 0x80538268 (0x4C): Returns the input mode (0, 1 or 2). */
s32 HkbKeyState::getMode() {
    switch (mFlags & 0xF) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    }
    return 0;
}

/* 0x805382B4 (0xA8): Returns the character of character key `index` for the case flags, as ASCII punctuation in
 * the half-width symbol-free mode. */
u16 HkbKeyState::getChar(u16 index) {
    u32 column = 0;
    if (mFlags & 0x40) {
        column |= 1;
    }
    if (mFlags & 0x80) {
        column |= 2;
    }
    u16 c = mpLayout->charKeys[index].chars[column];
    if (mbFullWidth == 0) {
        if (getModeBits() == 0) {
            if (c == 0x3001) {
                c = ',';
            } else if (c == 0x3002) {
                c = '.';
            } else if (c == 0x30FC) {
                c = '-';
            } else if (c == 0x300C) {
                c = '[';
            } else if (c == 0x300D) {
                c = ']';
            }
        }
    }
    return c;
}

/* 0x8053835C (0x1E4): Returns the character of the key pane `paneName` in the current case and kana column, or 0. */
u16 HkbKeyState::findChar(const char* paneName) {
    u16 c = 0;
    for (u16 i = 0; i < 50; i++) {
        if (hbmNameMatches(mpLayout->charKeys[i].name, paneName)) {
            c = getChar(i);
        }
    }
    for (u16 j = 0; j < 65; j++) {
        if (hbmNameMatches(mpLayout->kanaKeys[j].paneName, paneName)) {
            c = mpLayout->kanaKeys[j].chars[getKanaColumn()];
            if ((mKanaFlags & 0xF) != 3) {
                if (c == 0xFF0F) {
                    c = '/';
                }
                if (c == 0xFF0D) {
                    c = '-';
                }
            }
        }
    }
    return c;
}

/* 0x80538540 (0x90): Activates or deactivates the keyboard: half-width input with no case lock, or full-width
 * input with the kana lock cleared. */
void HkbKeyboardBase::setActive(bool bActive) {
    mbActive = bActive;
    if (bActive) {
        setHalfWidth(1);
        setCaseLock(0);
    } else {
        setHalfWidth(0);
        setKanaLock(6);
    }
}

/* 0x805387F8 (0x200): Creates the pane handler, the pane manager and the key components, finds the shift and caps
 * buttons, then initialises the keyboard. */
void HkbKeyboard::create(MEMAllocator* pAllocator) {
    mpAllocator = pAllocator;
    HkbKeyboardPaneHandler* pHandler = new (MEMAllocFromAllocator(pAllocator, sizeof(HkbKeyboardPaneHandler)))
        HkbKeyboardPaneHandler(this);
    mpPaneHandler = pHandler;
    HbmPaneManager::create(pAllocator, pHandler);
    mpRoot->setVisible(0);
    mpRoot->setEnable(1);
    createKeys(pAllocator);
    mShiftButton.attach(this, "P_key_SHIFT", "B_key_SHIFT");
    mCapsButton.attach(this, "P_key_CAPS", "B_key_CAPS");
    mShiftButton.mpBase->setTriggerTarget(1);
    mCapsButton.mpBase->setTriggerTarget(1);
    init();
    setCaseLock(0);
}

/* 0x805389F8 (0x300): Creates one key component per key definition, with its animations, and lists them in the
 * pane manager. */
void HkbKeyboard::createKeys(MEMAllocator* pAllocator) {
    for (u16 i = 0; i < 142; i++) {
        const HkbKeyDef* def = &hkbKeyDefs[i];
        HkbKey* pKey = NULL;
        switch (def->type) {
        case 0:
            pKey = new (MEMAllocFromAllocator(pAllocator, sizeof(HkbCharKey))) HkbCharKey(findTextBox(def->name));
            break;
        case 1:
            pKey = new (MEMAllocFromAllocator(pAllocator, sizeof(HkbKanaKey))) HkbKanaKey(findTextBox(def->name));
            break;
        case 2:
            pKey = new (MEMAllocFromAllocator(pAllocator, sizeof(HkbFuncKey))) HkbFuncKey(findTextBox(def->name));
            break;
        case 3:
            pKey = new (MEMAllocFromAllocator(pAllocator, sizeof(HkbSpaceKey))) HkbSpaceKey(findTextBox(def->name));
            break;
        }
        nw4r::ut::List_Append(&mComponents, pKey);
        const char* group = def->group;
        for (u16 j = 0; j < def->numAnims; j++) {
            const HkbAnimDef* pAnimDef = def->anims[j];
            const void* res = mpResAccessor->GetResource(0, pAnimDef->name, NULL);
            nw4r::lyt::AnimTransform* anim = getLayout()->CreateAnimTransform(res, mpResAccessor);
            if (group == NULL) {
                pKey->addAnim(pAllocator, pAnimDef->kind, anim, 0, 1);
            } else {
                pKey->addGroupAnim(pAllocator, pAnimDef->kind, anim, group, 0, 1);
            }
        }
    }
}

/* 0x80538D50 (0x8): Returns the layout the pane manager draws. */
nw4r::lyt::Layout* HbmPaneManager::getLayout() {
    return mpLyt;
}

/* 0x80538D58 (0x4A0): Resets the keyboard: the case state, the layout's panes and labels, the input mode and the
 * prediction. */
void HkbKeyboard::init() {
    mStatus.bInputDone = 0;
    mpLayout = &hkbKeyLayouts[getLayoutType()];
    mKeyState.mbFullWidth = 0;
    if (mKeyState.mLayoutIndex == 0 && mKeyState.mpOwner->mbActive == 0) {
        mKeyState.mFlags = 1;
    } else {
        mKeyState.mFlags = 0;
    }
    mKeyState.mKanaFlags = 0;
    mKeyState.update();
    HkbModeEvent ev;
    ev.mode = mKeyState.getMode();
    sendEvent(0x12, &ev);
    setEnterKeyVisible(1);
    hideLineFeed();
    setLanguageKeys(1);
    getPaneAnim("P_key_SHIFT")->setState(0);
    getPaneAnim("P_key_CAPS")->setState(0);
    const HkbPaneSwitch* pSwitch = mpLayout->paneSwitch;
    for (u16 i = 0; i < pSwitch->showNum; i++) {
        setPaneVisible(pSwitch->show[i], 1);
    }
    for (u16 i = 0; i < pSwitch->hideNum; i++) {
        setPaneVisible(pSwitch->hide[i], 0);
    }
    if (getLayoutType() != 0) {
        setPaneText("T_USEU_Chng_sign", hkbLayoutLabels[7][getLayoutType()]);
    }
    setPaneText("T_key_SPACE", hkbLayoutLabels[8][getLayoutType()]);
    setPaneText("T_JP_Chng_sign", hkbLayoutLabels[10][getLayoutType()]);
    setPaneText("T_JP_Chng_action", hkbLayoutLabels[11][getLayoutType()]);
    setPaneText("T_Gkey_SPACE", hkbLayoutLabels[12][getLayoutType()]);
    setActive(0);
    HkbTextQuery query;
    query.value = 0;
    *(u32*)&query.bFlag = 0;
    sendEvent(0x1F, &query);
    updatePrediction(&query);
    mpLyt->Animate(0);
    mpLyt->CalculateMtx(mDrawInfo);
    setPaneVisible("T_key_SPACE", 1);
    setPaneVisible("T_Gkey_SPACE", 1);
    setPaneVisible("P_key_HENKAN", 0);
    setPaneVisible("P_Gkey_HENKAN", 0);
    setPaneVisible("B_Gkey_SPACE", 1);
}

/* 0x805391F8 (0x1438): Rebuilds the input-mode panes for the current width, kana type and case mode. */
void HkbKeyboard::updateModePanes() {
    if (getLayoutType() == 0) {
        if (isHalfWidth()) {
            getPaneAnim("W_JP_Chng_ABC")->setState(5);
            getPaneAnim("W_JP_Chng_KANA")->setState(0);
        } else {
            getPaneAnim("W_JP_Chng_ABC")->setState(0);
            getPaneAnim("W_JP_Chng_KANA")->setState(5);
        }
        u32 kanaType = mKeyState.mKanaFlags & 3;
        if (kanaType == 0) {
            HkbKeyboardBase::setMode(1);
            getPaneAnim("P_hiragana")->setState(5);
            getPaneAnim("P_katakana")->setState(0);
            getPaneAnim("P_abc")->setState(0);
            getPaneAnim("P_sign")->setState(0);
            getPaneAnim("P_komoji")->setState(14);
            getPaneAnim("P_hanzen")->setState(15);
            updateSmallLabel(this);
            setPaneVisible("B_komoji", 1);
            if (mPanelKind != 14) {
                setPaneVisible("T_hankaku", 0);
                setPaneVisible("T_zenkaku", 0);
                setPaneVisible("B_hanzen", 0);
            }
        } else if (kanaType == 1) {
            HkbKeyboardBase::setMode(2);
            mKeyState.toggleVoiced();
            getPaneAnim("P_hiragana")->setState(0);
            getPaneAnim("P_katakana")->setState(5);
            getPaneAnim("P_abc")->setState(0);
            getPaneAnim("P_sign")->setState(0);
            getPaneAnim("P_komoji")->setState(14);
            getPaneAnim("P_hanzen")->setState(14);
            updateSmallLabel(this);
            setPaneVisible("B_komoji", 1);
            if (mPanelKind != 14) {
                updateWidthLabel(this);
            }
        } else if (kanaType == 2) {
            HkbKeyboardBase::setMode(0);
            if (mInputStyle == 0) {
                mKeyState.toggleVoiced();
            }
            getPaneAnim("P_hiragana")->setState(0);
            getPaneAnim("P_katakana")->setState(0);
            getPaneAnim("P_abc")->setState(5);
            getPaneAnim("P_sign")->setState(0);
            getPaneAnim("P_komoji")->setState(14);
            if (mPanelKind != 14) {
                getPaneAnim("P_hanzen")->setState(14);
            } else {
                getPaneAnim("P_hanzen")->setState(15);
                if (mInputStyle == 1) {
                    getPaneAnim("P_komoji")->setState(15);
                }
            }
            if (mInputStyle == 1) {
                setPaneVisible("T_omoji", 0);
                setPaneVisible("T_komoji", 0);
                setPaneVisible("B_komoji", 0);
                setPaneVisible("P_key_SPACE", 0);
                setPaneVisible("T_key_SPACE", 0);
                setPaneVisible("T_Gkey_SPACE", 0);
                setPaneVisible("B_Gkey_SPACE", 0);
            } else {
                updateSmallLabel(this);
                setPaneVisible("B_komoji", 1);
            }
            if (mPanelKind != 14) {
                updateWidthLabel(this);
            }
        } else if (kanaType == 3) {
            HkbKeyboardBase::setMode(0);
            getPaneAnim("P_hiragana")->setState(0);
            getPaneAnim("P_katakana")->setState(0);
            getPaneAnim("P_abc")->setState(0);
            getPaneAnim("P_sign")->setState(5);
            getPaneAnim("P_komoji")->setState(15);
            getPaneAnim("P_hanzen")->setState(15);
            setPaneVisible("T_komoji", 0);
            setPaneVisible("T_omoji", 0);
            setPaneVisible("B_komoji", 0);
            if (mPanelKind != 14) {
                setPaneVisible("T_hankaku", 0);
                setPaneVisible("T_zenkaku", 0);
                setPaneVisible("B_hanzen", 0);
            }
        }
        updateMarkAnims(this);
        switch (mKeyState.getModeBits()) {
        case 0:
            getPaneAnim("P_Mode_roma_hira")->setState(0);
            getPaneAnim("P_Mode_roma_kata")->setState(0);
            getPaneAnim("P_Mode_direct")->setState(5);
            break;
        case 1:
            getPaneAnim("P_Mode_roma_hira")->setState(5);
            getPaneAnim("P_Mode_roma_kata")->setState(0);
            getPaneAnim("P_Mode_direct")->setState(0);
            break;
        case 2:
            getPaneAnim("P_Mode_roma_hira")->setState(0);
            getPaneAnim("P_Mode_roma_kata")->setState(5);
            getPaneAnim("P_Mode_direct")->setState(0);
            break;
        }
    } else {
        u32 kanaType = mKeyState.mKanaFlags & 3;
        if (kanaType == 2) {
            HkbKeyboardBase::setMode(0);
            if (mInputStyle == 0) {
                mKeyState.toggleVoiced();
            }
            getPaneAnim("P_hiragana")->setState(0);
            getPaneAnim("P_katakana")->setState(0);
            getPaneAnim("P_abc")->setState(5);
            getPaneAnim("P_sign")->setState(0);
            getPaneAnim("P_hanzen")->setState(15);
            if (mInputStyle == 1) {
                getPaneAnim("P_komoji")->setState(15);
                setPaneVisible("T_omoji", 0);
                setPaneVisible("T_komoji", 0);
                setPaneVisible("B_komoji", 0);
                setPaneVisible("P_key_SPACE", 0);
                setPaneVisible("T_key_SPACE", 0);
                setPaneVisible("T_Gkey_SPACE", 0);
                setPaneVisible("B_Gkey_SPACE", 0);
            } else {
                getPaneAnim("P_komoji")->setState(14);
                updateSmallLabel(this);
                setPaneVisible("B_komoji", 1);
            }
        } else if (kanaType == 3) {
            HkbKeyboardBase::setMode(0);
            mKeyState.toggleVoiced();
            getPaneAnim("P_hiragana")->setState(0);
            getPaneAnim("P_katakana")->setState(0);
            getPaneAnim("P_abc")->setState(0);
            getPaneAnim("P_sign")->setState(5);
            getPaneAnim("P_komoji")->setState(15);
            getPaneAnim("P_hanzen")->setState(15);
            setPaneVisible("T_komoji", 0);
            setPaneVisible("T_omoji", 0);
            setPaneVisible("B_komoji", 0);
        }
        if (mKeyState.mKanaFlags & 0x80) {
            getPaneAnim("P_Gkey_komoji")->setState(14);
        } else {
            getPaneAnim("P_Gkey_komoji")->setState(15);
        }
        switch (mKeyState.mFlags & 0xF) {
        case 0:
            getPaneAnim("P_Mode_roma_hira")->setState(0);
            getPaneAnim("P_Mode_roma_kata")->setState(0);
            getPaneAnim("P_Mode_direct")->setState(5);
            break;
        }
    }
    mpLyt->Animate(0);
    mpLyt->CalculateMtx(mDrawInfo);
    if (mPanelKind == 14) {
        setMode(0);
    }
}

/* 0x8053A630 (0x8): Draws the pane manager's components. */
void HkbKeyboard::drawComponents() {
    HbmPaneManager::drawComponents();
}

/* 0x8053A638 (0x8): Calculates the pane manager's components. */
void HkbKeyboard::calcComponents() {
    HbmPaneManager::calcComponents();
}

/* 0x8053A640 (0x14): Types one character with no flags. */
void HkbKeyboard::inputChar(u16 c) {
    inputChar(c, 0);
}

/* Toggles the caps lock (and the USB keyboard's caps lamp), dropping a one-shot shift. */
inline void HkbKeyboard::switchCapsLock() {
    HkbKbdState& kbd = hkbGetKbdState();
    if (!kbd.isShiftHeld()) {
        mKeyState.clearShift();
        if (mShiftButton.mbOn) {
            mShiftButton.mbOn = 0;
            mShiftButton.mpAnim->setAnim(5);
        }
    }
    u8 bCapsLock = kbd.mbCapsLock == 0;
    kbd.setCapsLock(bCapsLock);
    mKeyState.mFlags = (mKeyState.mFlags & ~0x40) | ((mKeyState.mFlags ^ 0x40) & 0x40);
    mKeyState.update();
    mCapsButton.setLit(mKeyState.mBits.bCaps);
    mpSound->play(13);
}

/* 0x8053A654 (0x170): Toggles the caps lock (and the USB keyboard's caps lamp), dropping a one-shot shift. */
void HkbKeyboard::toggleCapsLock() {
    switchCapsLock();
}

/* 0x8053A7C4 (0xC4): Turns the one-shot shift on, optionally with the key sound. */
void HkbKeyboard::setShiftOn(s32 bSound) {
    mKeyState.setShiftFlag(1);
    mShiftButton.setLit(1);
    if (bSound) {
        mpSound->play(13);
    }
}

/* 0x8053A888 (0x9C): Turns the one-shot shift off. */
void HkbKeyboard::setShiftOff() {
    mKeyState.setShiftFlag(0);
    if (mShiftButton.mbOn) {
        mShiftButton.mbOn = 0;
        mShiftButton.mpAnim->setAnim(5);
    }
}

/* 0x8053A924 (0xE78): Handles a press on a key pane: the shift, caps, width and kana keys of the panel, then the
 * common handling. */
void HkbKeyboard::onPaneEvent(u32 event, const char* paneName) {
    findCharByName(paneName);
    if (event == 4 && findCharByName(paneName) == 0) {
        switch (findCommandByName(paneName)) {
        case 3:
            switchCapsLock();
            break;
        case 4: {
            HkbKbdState& kbd = hkbGetKbdState();
            mKeyState.clearCaps();
            if (mCapsButton.mbOn) {
                mCapsButton.mbOn = 0;
                mCapsButton.mpAnim->setAnim(5);
            }
            if (!kbd.isShiftHeld()) {
                mKeyState.mBits.bShift ^= 1;
                mKeyState.update();
            }
            mShiftButton.setLit(mKeyState.mBits.bShift);
            mpSound->play(13);
            break;
        }
        case 0:
            mpSound->play(9);
            break;
        case 11:
            if (!isHalfWidth()) {
                mpSound->play(13);
                getPaneAnim("W_JP_Chng_KANA")->setState(6);
            }
            break;
        case 12:
            if (isHalfWidth()) {
                mpSound->play(13);
                getPaneAnim("W_JP_Chng_ABC")->setState(6);
            }
            break;
        case 18:
            if ((mKeyState.mKanaFlags & 3) == 1) {
                mpSound->play(13);
                getPaneAnim("P_katakana")->setState(6);
            }
            if ((mKeyState.mKanaFlags & 3) == 2) {
                mpSound->play(13);
                getPaneAnim("P_abc")->setState(6);
            }
            if ((mKeyState.mKanaFlags & 3) == 3) {
                mpSound->play(13);
                getPaneAnim("P_sign")->setState(6);
            }
            getPaneAnim("P_hanzen")->setState(15);
            getPaneAnim("P_komoji")->setState(14);
            setPaneVisible("T_komoji", 1);
            setPaneVisible("T_omoji", 0);
            setPaneVisible("T_hankaku", 0);
            setPaneVisible("T_zenkaku", 0);
            setPaneVisible("B_komoji", 1);
            setPaneVisible("B_hanzen", 0);
            break;
        case 19:
            if ((mKeyState.mKanaFlags & 3) == 0) {
                mpSound->play(13);
                getPaneAnim("P_hiragana")->setState(6);
            }
            if ((mKeyState.mKanaFlags & 3) == 2) {
                mpSound->play(13);
                getPaneAnim("P_abc")->setState(6);
            }
            if ((mKeyState.mKanaFlags & 3) == 3) {
                mpSound->play(13);
                getPaneAnim("P_sign")->setState(6);
            }
            getPaneAnim("P_komoji")->setState(14);
            setPaneVisible("T_komoji", 0);
            setPaneVisible("T_omoji", 1);
            setPaneVisible("T_hankaku", 0);
            setPaneVisible("T_zenkaku", 0);
            setPaneVisible("B_komoji", 1);
            setPaneVisible("B_hanzen", 0);
            break;
        case 27:
            mKeyState.toggleVoiced();
            if ((mKeyState.mKanaFlags & 3) == 0) {
                mpSound->play(13);
                getPaneAnim("P_hiragana")->setState(6);
            }
            if ((mKeyState.mKanaFlags & 3) == 1) {
                mpSound->play(13);
                getPaneAnim("P_katakana")->setState(6);
            }
            if ((mKeyState.mKanaFlags & 3) == 3) {
                mpSound->play(13);
                getPaneAnim("P_sign")->setState(6);
            }
            getPaneAnim("P_komoji")->setState(14);
            setPaneVisible("T_komoji", 0);
            setPaneVisible("T_omoji", 1);
            setPaneVisible("T_hankaku", 0);
            setPaneVisible("T_zenkaku", 0);
            setPaneVisible("B_komoji", 1);
            setPaneVisible("B_hanzen", 0);
            break;
        case 28:
            if ((mKeyState.mKanaFlags & 3) == 0) {
                mpSound->play(13);
                getPaneAnim("P_hiragana")->setState(6);
            }
            if ((mKeyState.mKanaFlags & 3) == 1) {
                mpSound->play(13);
                getPaneAnim("P_katakana")->setState(6);
            }
            if ((mKeyState.mKanaFlags & 3) == 2) {
                mpSound->play(13);
                getPaneAnim("P_abc")->setState(6);
            }
            getPaneAnim("P_hanzen")->setState(15);
            getPaneAnim("P_komoji")->setState(15);
            setPaneVisible("T_komoji", 0);
            setPaneVisible("T_omoji", 0);
            setPaneVisible("T_hankaku", 0);
            setPaneVisible("T_zenkaku", 0);
            setPaneVisible("B_komoji", 0);
            setPaneVisible("B_hanzen", 0);
            break;
        case 29:
            if (mKeyState.mKanaFlags & 4) {
                setPaneVisible("T_hankaku", 1);
                setPaneVisible("T_zenkaku", 0);
            } else {
                setPaneVisible("T_hankaku", 0);
                setPaneVisible("T_zenkaku", 1);
            }
            mpSound->play(18);
            break;
        case 30:
            if (mKeyState.mKanaFlags & 8) {
                setPaneVisible("T_komoji", 1);
                setPaneVisible("T_omoji", 0);
            } else {
                setPaneVisible("T_komoji", 0);
                setPaneVisible("T_omoji", 1);
            }
            mpSound->play(18);
            break;
        case 15:
            if ((mKeyState.mFlags & 0xF) == 2) {
                getPaneAnim("P_Mode_roma_kata")->setState(6);
            }
            if ((mKeyState.mFlags & 0xF) == 1) {
                getPaneAnim("P_Mode_roma_hira")->setState(6);
            }
            break;
        case 16:
            if ((mKeyState.mFlags & 0xF) == 0) {
                getPaneAnim("P_Mode_direct")->setState(6);
            }
            if ((mKeyState.mFlags & 0xF) == 2) {
                getPaneAnim("P_Mode_roma_kata")->setState(6);
            }
            break;
        case 17:
            if ((mKeyState.mFlags & 0xF) == 0) {
                getPaneAnim("P_Mode_direct")->setState(6);
            }
            if ((mKeyState.mFlags & 0xF) == 1) {
                getPaneAnim("P_Mode_roma_hira")->setState(6);
            }
            break;
        }
    }
    HkbKeyboardBase::onPaneEvent(event, paneName);
}

/* 0x8053B79C (0x4C): Sets the layout type and re-initialises the keyboard for it. */
void HkbKeyboard::setLayoutType(s32 type) {
    mLayoutType = type;
    mKeyState.mLayoutIndex = type;
    mKeyState.update();
    init();
}

/* 0x8053B7E8 (0x7C): Runs a command, refreshes the kana keys and, on command 0x1D, the prediction. */
void HkbKeyboard::onCommand(u32 command, u32 arg) {
    HkbKeyboardBase::onCommand(command, arg);
    updateKanaKeys();
    switch (command) {
    case 0x1D:
        updatePrediction((HkbTextQuery*)arg);
        break;
    }
}

/* 0x8053B864 (0x194): Refreshes the common view, the components and the kana mark keys. */
void HkbKeyboard::updateView() {
    HkbKeyboardBase::updateView();
    HbmPaneManager::init();
    updateMarkAnims(this);
    mpSound->setEnable(1);
}

/* 0x8053B9F8 (0x74): Closes the keyboard: resets the cursor, refreshes the keys and closes the context. */
void HkbKeyboard::close() {
    resetCursor();
    refreshKeys();
    mpContext->v_0x068()->v_0x128();
}

/* 0x8053BA6C (0x10): Resets the cursor. */
void HbmPaneManager::resetCursor() {
    mCursor.reset();
}

/* 0x8053BA7C (0x1C): Forgets every remembered key. */
void HkbRecentKeys::clear() {
    mKeys[0] = 0;
    mKeys[1] = 0;
    mKeys[2] = 0;
    mKeys[3] = 0;
    mKeys[4] = 0;
}

/* 0x8053BA98 (0x90): Puts every character key back to its idle animation. */
void HkbKeyboard::releaseKeys() {
    for (HbmKeyView* pKey = (HbmKeyView*)nw4r::ut::List_GetNext(&mComponents, NULL); pKey != NULL;
         pKey = (HbmKeyView*)nw4r::ut::List_GetNext(&mComponents, pKey)) {
        if (pKey->getType() == 0) {
            pKey->setAnim(2);
        }
    }
}

/* 0x8053BB28 (0x8): Returns the key's kind. */
s32 HkbKey::getType() {
    return mType;
}

/* 0x8053BB30 (0x4): Sets the key's animation (nothing to do for a plain key). */
void HbmKey::setAnim(s32 anim) {}

/* 0x8053BB34 (0x240): Settles every key's state after a layout change. */
void HkbKeyboard::refreshKeys() {
    for (HbmKeyView* pKey = (HbmKeyView*)nw4r::ut::List_GetNext(&mComponents, NULL); pKey != NULL;
         pKey = (HbmKeyView*)nw4r::ut::List_GetNext(&mComponents, pKey)) {
        switch (pKey->getType()) {
        case 0:
            switch (pKey->getState()) {
            case 2:
                break;
            case 1:
            case 3:
            case 4:
                pKey->setState(2);
                break;
            case 16:
                pKey->setState(0);
                break;
            }
            break;
        case 1:
            switch (pKey->getState()) {
            case 1:
            case 3:
            case 11:
                pKey->setState(2);
                break;
            case 4:
            case 5:
            case 8:
                pKey->setState(9);
                break;
            }
            break;
        case 2:
            switch (pKey->getState()) {
            case 3:
            case 1:
                pKey->setState(2);
                break;
            case 4:
                pKey->setState(5);
                break;
            }
            break;
        case 3:
            switch (pKey->getState()) {
            case 2:
                break;
            case 1:
            case 3:
            case 4:
                pKey->setState(2);
                break;
            }
            break;
        }
    }
}

/* 0x8053BD74 (0x8): Returns the key's state. */
s32 HkbKey::getState() {
    return mState;
}

/* 0x8053BD7C (0x24): Moves the text cursor through the cursor sink. */
void HkbKeyboard::setCursor(u32 position) {
    mpCursorSink->moveCursor(this, 0, position);
}

/* 0x8053BDA0 (0x7C): Releases the keys and hands the current text to the text sink. */
void HkbKeyboard::submit() {
    releaseKeys();
    HkbTextQuery query;
    query.value = 0;
    *(u32*)&query.bFlag = 0;
    sendEvent(0x1F, &query);
    mpPaneSink->submitText(query.value, this);
}

/* 0x8053BE1C (0x118): Shows the prediction switch and the prediction language for the text `pQuery`. */
void HkbKeyboard::updatePrediction(HkbTextQuery* pQuery) {
    if (getLayoutType() != 0) {
        if (pQuery->bFlag == 0) {
            setPaneVisible("P_prdc_ON", 0);
            setPaneVisible("P_prdc_OFF", 1);
        } else {
            setPaneVisible("P_prdc_ON", 1);
            setPaneVisible("P_prdc_OFF", 0);
        }
        findTextBox("T_USEU_prdc_lang")->SetString(hkbLanguageNames[pQuery->value], 0);
    }
}

/* 0x8053BF34 (0xB0): Types one character, keeping the cursor on its target while the shift is held. */
void HkbKeyboard::inputChar(u16 c, u32 flags) {
    u32 bShift = 0;
    u32 target = 0;
    if (mKeyState.mFlags & 0x80) {
        target = mCursor.getTarget();
        bShift = 1;
    }
    HkbKeyboardBase::inputChar(c, flags);
    if (bShift && target) {
        setCursorTarget(target);
    }
    updateKanaKeys();
}

/* 0x8053BFE4 (0x40): Returns the most recently remembered key, or 0. */
u32 HkbRecentKeys::getLast() {
    for (s32 i = 4; i >= 0; i--) {
        if (mKeys[i] != 0) {
            return mKeys[i];
        }
    }
    return 0;
}

/* 0x8053C024 (0x10): Moves the cursor to `target`. */
void HbmPaneManager::setCursorTarget(u32 target) {
    mCursor.setTarget(target);
}

/* 0x8053C034 (0xA8): Remembers `key`: in the first free entry, or by shifting the oldest out. */
void HkbRecentKeys::push(u32 key) {
    if (mKeys[4] == 0) {
        for (s32 i = 0; i < 5; i++) {
            if (mKeys[i] == 0) {
                mKeys[i] = key;
                return;
            }
        }
    } else {
        for (s32 i = 0; i < 4; i++) {
            mKeys[i] = mKeys[i + 1];
            if (key == mKeys[i]) {
                mKeys[i] = 0;
            }
        }
        mKeys[4] = key;
    }
}

/* 0x8053C0DC (0x168): Sets the kana mark keys' frames for the marks the last character accepts. */
void HkbKeyboard::updateKanaKeys() {
    if (mKeyState.mKanaFlags & 0x20) {
        getPaneAnim("P_Gkey_dakuten")->setAnim(6);
    } else {
        getPaneAnim("P_Gkey_dakuten")->setAnim(7);
    }
    if (mKeyState.mKanaFlags & 0x40) {
        getPaneAnim("P_Gkey_handaku")->setAnim(6);
    } else {
        getPaneAnim("P_Gkey_handaku")->setAnim(7);
    }
    if (mKeyState.mKanaFlags & 0x80) {
        getPaneAnim("P_Gkey_komoji")->setAnim(6);
    } else {
        getPaneAnim("P_Gkey_komoji")->setAnim(7);
    }
}

/* 0x8053C244 (0x6C): Shows or hides the enter keys. */
void HkbKeyboard::setEnterKeyVisible(u32 bVisible) {
    mbEnterKeyVisible = bVisible;
    setPaneVisible("P_key_LF", mbEnterKeyVisible);
    setPaneVisible("P_Gkey_LF", mbEnterKeyVisible);
}

/* 0x8053C2B0 (0x1C): Hides the prediction language window. */
void HkbKeyboard::hideLineFeed() {
    setPaneVisible("W_USEU_prdc_lang", 0);
}

/* 0x8053C2CC (0x118): Shows the language switch window of the current layout, or hides both. */
void HkbKeyboard::setLanguageKeys(s32 bVisible) {
    if (bVisible == 0) {
        setPaneVisible("W_USEU_Chng_sign", bVisible);
        setPaneVisible("W_JP_Chng_sign", bVisible);
    } else {
        setPaneVisible("W_USEU_Chng_sign", 0);
        setPaneVisible("W_JP_Chng_sign", 0);
        if (getLayoutType() == 0) {
            setPaneVisible("W_JP_Chng_sign", 1);
        } else {
            setPaneVisible("W_USEU_Chng_sign", 1);
        }
    }
}

/* 0x8053C3E4 (0x170): Activates or deactivates the keyboard and shows or hides the mode windows. */
void HkbKeyboard::setActive(bool bActive) {
    mbActive = bActive;
    if (bActive) {
        setHalfWidth(1);
        setCaseLock(0);
    } else {
        setHalfWidth(0);
        setKanaLock(6);
    }
    updateModePanes();
    s32 bVisible = bActive == 0;
    setPaneVisible("W_JP_Chng_ABC", bVisible);
    setPaneVisible("W_JP_Chng_KANA", bVisible);
    setPaneVisible("P_Mode_roma_hira", bVisible);
    setPaneVisible("P_Mode_roma_kata", bVisible);
    setPaneVisible("P_Mode_direct", bVisible);
    setPaneVisible("P_romajiBox", bVisible);
}

/* 0x8053C554 (0x134): Sets the width, the input mode and the kana sub-mode together. */
void HkbKeyboard::setModes(u32 bHalfWidth, u32 mode, u32 kanaMode) {
    setHalfWidth(bHalfWidth);
    mKeyState.setMode(mode);
    mKeyState.setKanaMode(kanaMode);
    switch (mode) {
    case 0:
        mpLayout = &hkbKeyLayouts[getLayoutType()];
        break;
    case 1:
        mpLayout = &hkbSymbolLayout;
        break;
    case 2:
        mpLayout = &hkbSymbolLayout;
        break;
    }
    updateModePanes();
    sendEvent(0x29, NULL);
}

/* 0x8053C688 (0x1E0): Selects input mode `mode`, playing the mode windows' change animations. */
void HkbKeyboard::setMode(s32 mode) {
    s32 oldMode = mKeyState.getModeBits();
    HkbKeyboardBase::setMode(mode);
    if (oldMode != mKeyState.getModeBits()) {
        mpSound->play(13);
        switch (oldMode) {
        case 0:
            getPaneAnim("P_Mode_direct")->setState(6);
            break;
        case 1:
            getPaneAnim("P_Mode_roma_hira")->setState(6);
            break;
        case 2:
            getPaneAnim("P_Mode_roma_kata")->setState(6);
            break;
        }
        switch (mode) {
        case 0:
            getPaneAnim("P_Mode_direct")->setAnim(0);
            break;
        case 1:
            getPaneAnim("P_Mode_roma_hira")->setAnim(0);
            break;
        case 2:
            getPaneAnim("P_Mode_roma_kata")->setAnim(0);
            break;
        }
    }
}

/* 0x8053C868 (0x164): Updates the components, then releases the one-shot shift when a physical shift key let go. */
u32 HkbKeyboard::updateComponents() {
    u32 result = HbmPaneManager::updateComponents();
    HkbKbdState& kbd = hkbGetKbdState();
    bool bShiftHeld = kbd.isShiftHeld();
    kbd.mbRightShift = 0;
    hkbSetKeyRepeat(&hkbManager, 0, 0);
    if (bShiftHeld && !kbd.isShiftHeld()) {
        setShiftOff();
    }
    return result;
}

/* 0x8053C9CC (0x250): Puts the keys the manager reports released back to their first frame. */
s32 HkbKeyboard::onKeyRelease(HkbManager* pManager) {
    HkbKeyIter it(hkbKeyIterBeginWord1(pManager));
    while (hkbKeyIterValid(&it)) {
        HbmKey* pAnim = NULL;
        u8 usage = hkbKeyIterGetUsage(&it);
        u16 c = hkbKeyIterGetChar(&it);
        switch (usage) {
        case 0x2C:
            break;
        case 0x58:
        case 0x28:
            pAnim = getPaneAnim("P_key_LF");
            break;
        default:
            pAnim = getKeyAnim(hbmKbDeadKeyToSpacing(c));
            break;
        }
        if (pAnim != NULL) {
            pAnim->setAnim(0);
        }
        it = hkbKeyIterSeek(&it);
    }
    it = hkbKeyIterBeginWord3(pManager);
    while (hkbKeyIterValid(&it)) {
        HbmKey* pAnim = NULL;
        u8 usage = hkbKeyIterGetUsage(&it);
        hkbKeyIterGetChar(&it);
        switch (usage) {
        case 0x4C:
        case 0x2A:
            pAnim = getPaneAnim("P_key_DELETE");
            break;
        case 0x2C:
            pAnim = getPaneAnim("P_key_SPACE");
            break;
        }
        if (pAnim != NULL) {
            pAnim->setAnim(0);
        }
        it = hkbKeyIterSeek(&it);
    }
    return 0;
}

/* 0x8053CC1C (0x160): Releases the character keys and shows the input-type marker. */
void HkbKeyboard::resetKeys() {
    for (HbmKeyView* pKey = (HbmKeyView*)nw4r::ut::List_GetNext(&mComponents, NULL); pKey != NULL;
         pKey = (HbmKeyView*)nw4r::ut::List_GetNext(&mComponents, pKey)) {
        if (pKey->getType() == 0) {
            pKey->setState(0);
        }
    }
    if ((mKeyState.mKanaFlags & 0xF) == 0) {
        getPaneAnim("P_hiragana")->setState(5);
        getPaneAnim("P_katakana")->setState(0);
    } else {
        getPaneAnim("P_hiragana")->setState(0);
        getPaneAnim("P_katakana")->setState(5);
    }
}

/* 0x8053CD7C (0x53C): Refreshes the panes after a state change: the key grid or the character list for the width,
 * the language and mode windows, the key labels and the shift and caps buttons. */
void HkbKeyboard::onStateChanged() {
    u8 bVisible;
    if (isHalfWidth()) {
        if (getPaneVisible("N_VK_grid", &bVisible)) {
            bVisible = bVisible != 0;
        } else {
            bVisible = 0;
        }
        setPaneVisible("N_VK_grid", 0);
        setPaneVisible("N_VK_grd_Bnd_ALL", 0);
        setPaneVisible("N_VK_ascii", 1);
        setPaneVisible("N_VK_asc_Bnd_ALL", 1);
        setPaneVisible("W_JP_Chng_sign", 0);
        setPaneVisible("W_JP_Chng_action", 0);
    } else {
        if (getPaneVisible("N_VK_grid", &bVisible)) {
            bVisible = bVisible != 1;
        } else {
            bVisible = 0;
        }
        setPaneVisible("N_VK_grid", 1);
        setPaneVisible("N_VK_grd_Bnd_ALL", 1);
        setPaneVisible("N_VK_ascii", 0);
        setPaneVisible("N_VK_asc_Bnd_ALL", 0);
        setPaneVisible("W_JP_Chng_sign", 1);
        setPaneVisible("W_JP_Chng_action", 1);
        if (mPanelStyle == 1) {
            setPaneVisible("T_JP_Chng_action", 0);
            setPaneVisible("B_JP_Chng_action", 0);
            setPaneVisible("T_JP_Chng_sign", 0);
            setPaneVisible("B_JP_Chng_sign", 0);
        } else if (mPanelStyle == 2) {
            setPaneVisible("T_JP_Chng_action", 0);
            setPaneVisible("B_JP_Chng_action", 0);
        } else if (mPanelStyle == 3) {
            setPaneVisible("T_JP_Chng_sign", 0);
            setPaneVisible("B_JP_Chng_sign", 0);
        }
        if (mPanelKind == 14) {
            setPaneVisible("T_JP_Chng_action", 0);
            setPaneVisible("B_JP_Chng_action", 0);
            setPaneVisible("T_JP_Chng_sign", 0);
            setPaneVisible("B_JP_Chng_sign", 0);
        }
    }
    mKeyState.setupPanes(mpLyt->mpRootPane);
    if (getLayoutType() == 0) {
        u32 kanaMode = mKeyState.mKanaFlags & 0xF;
        if (kanaMode == 0) {
            findTextBox("T_JP_Chng_KANA")->SetString(L"\x3042\x3044\x3046", 0);
        } else if (kanaMode == 1) {
            findTextBox("T_JP_Chng_KANA")->SetString(L"\x30a2\x30a4\x30a6", 0);
        }
    }
    if (isShift() == 0 && mShiftButton.mbOn) {
        mShiftButton.mbOn = 0;
        mShiftButton.mpAnim->setAnim(5);
    }
    if (isCapsLock() == 0) {
        if (mCapsButton.mbOn) {
            mCapsButton.mbOn = 0;
            mCapsButton.mpAnim->setAnim(5);
        }
        hkbGetKbdState().setCapsLock(0);
    }
    if (bVisible) {
        resetCursor();
    }
}

/* 0x8053D2B8 (0xC): Returns whether the one-shot shift is on. */
s32 HkbKeyboardBase::isShift() {
    return mKeyState.mBits.bShift;
}

/* 0x8053D2C4 (0xC): Returns whether the caps lock is on. */
s32 HkbKeyboardBase::isCapsLock() {
    return mKeyState.mBits.bCaps;
}

/* 0x8053D2D0 (0x24): Plays the sound a pane event asks for. */
void HkbKeyboard::onSoundEvent(u32 id, u32 event, u32 sound) {
    if (event == 0) {
        mpSound->play(sound);
    }
}

/* 0x8053D2F4 (0x8): Sets the panel kind (14 for the compact panel). */
void HkbKeyboard::setPanelKind(u8 kind) {
    mPanelKind = kind;
}

/* 0x8053D2FC (0x14): Returns whether the panel is the compact one. */
s32 HkbKeyboard::isCompactPanel() {
    return mPanelKind == 14;
}
/* 0x80538220 (0x8): Returns the keyboard's panel kind (14 for the compact panel). */
u8 HkbKeyboard::getPanelKind() {
    return mPanelKind;
}


/* 0x8053D310 (0x314): Turns a press on a key's base pane into a press on its key pane: the animation, the click
 * sound, the key repeat of the delete and space keys, and the keyboard's pane event. */
void HkbKeyboardPaneHandler::onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger) {
    const char* baseName = pComponent->mpPane->mName;
    if (hbmNameMatches("B_key_SHIFT", baseName) || hbmNameMatches("B_key_CAPS", baseName)) {
        return;
    }
    if (baseName[0] == 'B') {
        char keyName[17];
        hbmNameCopyWithChar(keyName, sizeof(keyName), baseName, 0, 'P');
        if (strncmp(baseName + 5, "Chng", 4) == 0 || strncmp(baseName + 7, "Chng", 4) == 0 ||
            strncmp(baseName + 7, "prdc", 4) == 0) {
            hbmNameCopyWithChar(keyName, sizeof(keyName), baseName, 0, 'W');
        }
        HbmKey* pAnim = mpKeyboard->getPaneAnim(keyName);
        switch (event) {
        case 4:
            if (pTrigger->trigger & 0x10) {
                if (pAnim != NULL) {
                    pAnim->setAnim(0);
                }
                mpKeyboard->onPaneEvent(4, keyName);
            }
            break;
        case 1:
            if (pAnim != NULL) {
                pAnim->setAnim(2);
            }
            mpKeyboard->onPaneEvent(1, keyName);
            break;
        case 0:
            if (pAnim != NULL) {
                mpSound->play(4);
                mpKeyboard->setCursorTarget((u32)pAnim->mpPane);
                pAnim->setAnim(1);
            }
            break;
        }
        if (event == 2 && (pTrigger->hold & 0x10) && !(pTrigger->trigger & 0x10) &&
            (hbmNameMatches("P_key_DELETE", keyName) || hbmNameMatches("P_Gkey_DELETE", keyName) ||
             hbmNameMatches("P_key_SPACE", keyName) || hbmNameMatches("P_Gkey_SPACE", keyName)) &&
            pComponent->isPointed(pTrigger->channel)) {
            u32 frames = mpKeyboard->getHoldFrames(pTrigger->channel, baseName);
            if (frames >= 30 && frames % 9 == 0) {
                pAnim->setAnim(0);
                mpKeyboard->onPaneEvent(4, keyName);
            }
        }
    }
}

/* 0x8053D624 (0xC): Returns whether the component is pointed at by channel `channel`. */
bool HbmComponent::isPointed(u32 channel) {
    return mabPointed[channel];
}

/* 0x8053D630 (0x1C): Sets the key's state; the held state 16 plays as the pressed state 2. */
void HkbKey::setState(u32 state) {
    mState = state;
    HbmKey::setState(state == 16 ? 2 : state);
}

/* 0x8053D64C (0x254): Moves a character key through its press states for pane event `anim`. */
void HkbCharKey::setAnim(s32 anim) {
    switch (mState) {
    case 0:
        if (anim == 1) {
            setState(1);
        }
        if (anim == 0) {
            setState(16);
        }
        break;
    case 1:
        if (anim == 4) {
            setState(3);
        }
        if (anim == 2) {
            setState(2);
        }
        if (anim == 0) {
            setState(4);
        }
        break;
    case 3:
        if (anim == 2) {
            setState(2);
        }
        if (anim == 0) {
            setState(4);
        }
        break;
    case 2:
        if (anim == 4) {
            setState(0);
        }
        if (anim == 1) {
            setState(1);
        }
        if (anim == 0) {
            setState(16);
        }
        break;
    case 4:
        if (anim == 4) {
            setState(3);
        }
        if (anim == 2) {
            setState(2);
        }
        if (anim == 0) {
            setState(4);
        }
        break;
    case 16:
        if (anim == 4) {
            setState(0);
        }
        if (anim == 1) {
            setState(1);
        }
        if (anim == 0) {
            setState(16);
        }
        break;
    }
}

/* 0x8053D8A0 (0x34): Returns whether the key shows a lit state. */
s32 HkbKey::isLit() {
    switch (mState) {
    case 0:
    case 2:
    case 6:
    case 9:
    case 10:
        return 0;
    }
    return 1;
}

/* 0x8053D8D4 (0x490): Moves a kana key through its press and hold states for pane event `anim`. */
void HkbKanaKey::setAnim(s32 anim) {
    if (anim == 10) {
        mbHeld = 1;
        if (isLit() == 0) {
            setState(9);
        } else {
            setState(4);
        }
    } else if (anim == 11) {
        mbHeld = 0;
        if (isLit() == 0) {
            setState(2);
        } else {
            setState(11);
        }
    } else if (anim == 5) {
        mbHeld = 0;
        if (mState == 9) {
        } else if (mState == 10) {
            setState(6);
        } else if (mState == 8 || mState == 4 || mState == 5) {
            setState(3);
        }
    } else if (mbHeld == 0) {
        switch (mState) {
        case 9:
            if (anim == 4) {
                setState(0);
            }
            break;
        case 6:
            if (anim == 4) {
                setState(0);
            }
        case 0:
            if (anim == 1) {
                setState(1);
            }
            break;
        case 1:
            if (anim == 4) {
                setState(3);
            }
            if (anim == 2) {
                setState(2);
            }
            break;
        case 3:
            if (anim == 2) {
                setState(2);
            }
            break;
        case 2:
            if (anim == 4) {
                setState(0);
            }
            if (anim == 1) {
                setState(1);
            }
            break;
        case 11:
            if (anim == 4) {
                setState(3);
            }
            if (anim == 2) {
                setState(2);
            }
            break;
        }
    } else {
        switch (mState) {
        case 6:
            if (anim == 4) {
                setState(0);
            }
        case 0:
            if (anim == 1) {
                setState(1);
            }
            if (anim == 0) {
                setState(9);
            }
            break;
        case 10:
            if (anim == 1) {
                setState(8);
            }
            break;
        case 8:
            if (anim == 4) {
                setState(5);
            }
            if (anim == 2) {
                setState(9);
            }
            break;
        case 5:
            if (anim == 2) {
                setState(9);
            }
            break;
        case 9:
            if (anim == 4) {
                setState(10);
            }
            if (anim == 1) {
                setState(8);
            }
            break;
        case 4:
            if (anim == 4) {
                setState(5);
            }
            if (anim == 2) {
                setState(9);
            }
            break;
        case 11:
            break;
        }
    }
}

/* 0x8053DD64 (0x1BC): Moves a function key through its press states for pane event `anim`. */
void HkbFuncKey::setAnim(s32 anim) {
    if (anim == 0 && mState != 5 && mState != 4) {
        setState(4);
    }
    switch (mState) {
    case 0:
        if (anim == 1) {
            setState(1);
        }
        break;
    case 1:
        if (anim == 4) {
            setState(3);
        }
        if (anim == 2) {
            setState(2);
        }
        break;
    case 3:
        if (anim == 2) {
            setState(2);
        }
        break;
    case 2:
        if (anim == 4) {
            setState(0);
        }
        if (anim == 1) {
            setState(1);
        }
        break;
    case 4:
        if (anim == 4) {
            setState(5);
        }
        break;
    case 6:
        if (anim == 1) {
            setState(1);
        }
        if (anim == 4) {
            setState(0);
        }
        break;
    }
}

/* 0x8053DF20 (0x294): Moves the space key through its press and conversion states for pane event `anim`. */
void HkbSpaceKey::setAnim(s32 anim) {
    if (anim == 0 && mState != 15 && mState != 2 && mState != 13) {
        setState(4);
    } else if (anim == 6 && mState != 14 && mState != 4 && mState != 3 && mState != 12 && mState != 1 &&
               mState != 2) {
        setState(12);
    } else if (anim == 7 && mState != 15) {
        setState(13);
    } else {
        switch (mState) {
        case 12:
            if (anim == 4) {
                setState(14);
            }
            if (anim == 1) {
                setState(1);
            }
            break;
        case 13:
            if (anim == 4) {
                setState(15);
            }
            break;
        case 14:
            if (anim == 1) {
                setState(1);
            }
            break;
        case 1:
            if (anim == 4) {
                setState(3);
            }
            if (anim == 2) {
                setState(2);
            }
            break;
        case 3:
            if (anim == 2) {
                setState(2);
            }
            break;
        case 2:
            if (anim == 4) {
                setState(14);
            }
            if (anim == 1) {
                setState(1);
            }
            break;
        case 4:
            if (anim == 4) {
                setState(3);
            }
            if (anim == 2) {
                setState(2);
            }
            break;
        }
    }
}

/* 0x8053E1B4 (0x34): Builds a shift or caps button for `pKeyboard`, its components still to be found. */
HkbKeyButton::HkbKeyButton(u32 type, HkbKeyboard* pKeyboard, HbmSoundListener* pSound)
    : mType(type), mpKeyboard(pKeyboard), mpSound(pSound) {
    mpKey = NULL;
    mpBase = NULL;
    mpAnim = NULL;
    mbOn = 0;
}

/* 0x8053E1E8 (0x8): Returns the pane manager the components live in. */
HbmPaneLayout* HbmPaneManager::getRoot() {
    return mpRoot;
}

/* 0x8053E1F0 (0x8): Sets the component's trigger flag. */
void HbmComponent::setTriggerFlag(u8 flag) {
    mTriggerFlag = flag;
}

/* 0x8053E1F8 (0x130): Handles a press on the button's base pane: the click sound, the animation and the
 * keyboard's pane event for the shift or caps key. */
void HkbKeyButton::onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger) {
    const char* keyName = "P_key_CAPS";
    if (mType == 1) {
        keyName = "P_key_SHIFT";
    }
    switch (event) {
    case 4:
        if (pTrigger->trigger & 0x10) {
            mpKeyboard->onPaneEvent(4, keyName);
        }
        break;
    case 1:
        mpAnim->setAnim(2);
        mpKeyboard->onPaneEvent(1, keyName);
        break;
    case 0:
        if (mpSound != NULL) {
            mpSound->onSoundEvent((u32)this, 0, 4);
        }
        mpKeyboard->setCursorTarget((u32)mpAnim->mpPane);
        mpAnim->setAnim(1);
        break;
    }
}

/* 0x8053E328 (0x6C): Returns the key's animation of kind `kind`, or NULL. */
HbmKeyAnim* HbmKey::findAnim(u32 kind) {
    for (HbmKeyAnim* pAnim = (HbmKeyAnim*)nw4r::ut::List_GetNext(&mAnims, NULL); pAnim != NULL;
         pAnim = (HbmKeyAnim*)nw4r::ut::List_GetNext(&mAnims, pAnim)) {
        if (pAnim->kind == kind) {
            return pAnim;
        }
    }
    return NULL;
}

/* 0x8053E394 (0x14): Returns whether the key plays an animation. */
s32 HbmKey::isAnimating() {
    return mpCurAnim != NULL;
}

/* 0x8053E3A8 (0x4): Calculates the keyboard (nothing to do in the common part). */
void HkbInputWidget::calc() {}

/* 0x8053E3AC (0x8): Returns the component list. */
nw4r::ut::List* HbmPaneManager::getComponentList() {
    return &mComponents;
}

/* 0x8053E3B4 (0x8): Sets whether the pane manager takes input. */
void HbmPaneManager::setInputActive(u8 bActive) {
    mbInputActive = bActive;
}

/* 0x8053E3BC (0x8): Returns the manager the handler reports to. */
HbmPaneManager* HbmEventHandler::getManager() {
    return mpManager;
}

/* 0x8053E3C4 (0x8): Sets the manager the handler reports to. */
void HbmEventHandler::setManager(HbmPaneManager* pManager) {
    mpManager = pManager;
}

/* 0x8053E3CC (0x64): Returns the animation frame of the kana keys for the kana sub-mode. */
s32 HkbKeyboardBase::getKanaAnimFrame() {
    return mKeyState.getKanaFrame();
}

/* 0x8053E430 (0x8): Returns the focused key (none in the common part). */
s32 HkbKeyboardBase::getFocusKey() {
    return 0;
}

/* 0x8053E438 (0x8): Returns the input status. */
HkbInputStatus* HkbKeyboardBase::getStatus() {
    return &mStatus;
}

/* 0x8053E440 (0x8): Returns whether the keyboard is busy (never in the common part). */
s32 HkbKeyboardBase::isBusy() {
    return 0;
}

/* 0x8053E448 (0x8): Sets the receiver of cursor moves. */
void HkbKeyboard::setCursorSink(HkbTextSink* pSink) {
    mpCursorSink = pSink;
}

/* 0x8053E450 (0x8): Sets the receiver of submitted text. */
void HkbKeyboard::setTextSink(HkbTextSink* pSink) {
    mpPaneSink = pSink;
}

/* 0x8053E458 (0xC): Clears the key's state. */
void HkbKey::reset() {
    mState = 0;
}

/* 0x8053E5D4 (0x10): Hands an event on as a pane event. */
/* untyped: caller-owned payload - the argument record of the event */
void HbmPaneEventHandler::onEvent(u32 id, u32 event, void* data) {
    onPaneEvent((HbmComponent*)id, event, (HbmPaneTrigger*)data);
}

/* 0x8053E5E4 (0x4): Handles a pane event (nothing to do in the plain handler). */
void HbmPaneEventHandler::onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger) {}

/* 0x8053E5E8 (0x8): Sets the sound player of the handler. */
void HbmPaneHandler::setSound(HbmSoundPlayer* pSound) {
    mpSound = pSound;
}

/* 0x8053E5F0 (0x4): Sets the width and modes together (nothing to do in the common part). */
void HkbKeyboardBase::setModes(u32 bHalfWidth, u32 mode, u32 kanaMode) {}

/* 0x8053E5F4 (0x4): Handles an event (nothing to do in the plain handler). */
/* untyped: caller-owned payload - the argument record of the event */
void HbmEventHandler::onEvent(u32 id, u32 event, void* data) {}

/* 0x805385D0 (0xE8): Destroys the pane handler and the key components, then the pane manager. */
HkbKeyboard::~HkbKeyboard() {
    mpPaneHandler->~HbmEventHandler();
    MEMFreeToAllocator(mpAllocator, mpPaneHandler);
    for (HbmKey* pKey = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL); pKey != NULL;
         pKey = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL)) {
        nw4r::ut::List_Remove(&mComponents, pKey);
        pKey->destroy(mpAllocator);
    }
}

/* 0x805386B8 (0x40): Destroys the button. */
HkbKeyButton::~HkbKeyButton() {}

/* 0x805386F8 (0x40): Destroys the pane event handler. */
HbmPaneEventHandler::~HbmPaneEventHandler() {}

/* 0x80538738 (0x40): Destroys the event handler. */
HbmEventHandler::~HbmEventHandler() {}

/* 0x80538778 (0x40): Destroys the keyboard's common part. */
HkbKeyboardBase::~HkbKeyboardBase() {}

/* 0x805387B8 (0x40): Destroys the keyboard's pane handler. */
HkbKeyboardPaneHandler::~HkbKeyboardPaneHandler() {}

/* 0x80538CF8 (0x58): Destroys the key. */
HkbKey::~HkbKey() {}

/* 0x8053E464 (0x5C): Destroys the space key. */
HkbSpaceKey::~HkbSpaceKey() {}

/* 0x8053E4C0 (0x5C): Destroys the function key. */
HkbFuncKey::~HkbFuncKey() {}

/* 0x8053E51C (0x5C): Destroys the kana key. */
HkbKanaKey::~HkbKanaKey() {}

/* 0x8053E578 (0x5C): Destroys the character key. */
HkbCharKey::~HkbCharKey() {}
