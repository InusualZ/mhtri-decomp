/* auto/80073398_fn_80073398.cpp - nw4r g3d: the `ResVtx*` accessor inlines (`g3d_resvtx_ac.h`), the
 * `g3d_calcworld.cpp` node/matrix pass and the `g3d_camera.cpp` camera methods, `.text`
 * 0x80073398-0x80075DCC (75 functions).
 *
 * What it is.  The attribution pass claimed the range in bulk (docs/plan.md 12 item 5); the panic
 * strings in the range's `.data` run name the original files: `g3d_resvtx_ac.h` (the first 23 functions,
 * out-of-line copies of the accessor inlines), `g3d_calcworld.cpp` (`fn_800737CC`/`fn_8007411C` and their
 * helpers), `g3d_camera.cpp` (`fn_800746DC` onwards, the `Camera::Set*`/`Get*` methods) and
 * `g3d_rescommon_ac.h` (the `ResCommon<T>` `mpData` accessors `fn_800748E4`/`fn_80074A54`).  The three
 * already-demangled map symbols (`SetPosition`, `SetPosture`, `SetPerspective`) are member functions of
 * `nw4r::g3d::Camera`; every other symbol is still `fn_XXXXXXXX` in the map, so this file keeps those
 * names (a rename needs the map and the source in one edit, playbook 31).
 *
 * Types.  `ResHandle` is the one-word `ResCommon<T>`-style handle the `ResVtx*` inlines operate on;
 * `ResVtxNrmBlock` is the resource block `fn_800732F0`/`fn_800696E4` hand back.  `CameraData` is
 * `Camera`'s payload: the view matrix at +0x00, the projection matrix at +0x30, the posture at +0x74,
 * the projection parameters at +0xA8 and the viewport/scissor rectangle at +0xDC, with the +0x70 flags
 * word deciding which of them are current.  `nw4r::math::VEC3` is spelled out here as it is in
 * `Pl/pl_act.cpp` (rules 1-2 are unchecked; the two belong in one `include/` header, which is a
 * `shared-file` request, not this unit's edit).
 *
 * Status.  Landed as `Object(NonMatching, ...)`, so the link keeps the original bytes.  Per-function
 * residuals are in the objdiff report.  The two `g3d_calcworld.cpp` state machines (`fn_800737CC`,
 * `fn_8007411C`) are the vector-heavy pair this file does not reproduce yet.
 *
 * Data runs in this range are recorded in `splits.txt` as comments and are **not** claimed (playbook 23):
 *   extabindex   0x800206B8..0x8002091C  51 labels  proposed
 *   .data        0x8058E070..0x8058E56F  36 labels  proposed
 *   .sdata       0x807911B8..0x807911E0   4 labels  proposed
 *   .sdata2      0x80795DC0..0x80795DFC  13 labels  proposed
 * Evidence: `python tools/units/ledger.py unit auto/80073398_fn_80073398.cpp` (inventory) and the
 * `tu-boundary-discovery` skill (the pinned `.sdata2` seam `-1.0f` -> `lbl_80795DFC`).
 */

#include "types.h"

/* The target object contains no fused multiply-add at all (`fmadds`/`fmsubs` count: 0) while
 * `cflags_main` passes `-fp_contract on`, so the original files carried the pragma. File-scoped, which
 * is the shape the whole unit needs (see the header comment). */
#pragma fp_contract off

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker. */
void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace db

namespace nw4r {
namespace math {

struct VEC3 {
    /* +0x0 */ f32 x;
    /* +0x4 */ f32 y;
    /* +0x8 */ f32 z;
}; /* size: 0xC */

}  // namespace db
}  // namespace nw4r

/* ------------------------------------------------------------------------------------------------ */
/* externs: the SDK and the neighbouring units this one calls (the map owns their names)             */
/* ------------------------------------------------------------------------------------------------ */

extern "C" void DCStoreRange(void* pBase, u32 size);
extern "C" void GXSetProjection(const f32* pMtx, s32 type);
extern "C" void GXSetScissor(u32 left, u32 top, u32 width, u32 height);
extern "C" void GXSetScissorBoxOffset(s32 x, s32 y);
extern "C" void GXSetViewport(f32 x, f32 y, f32 width, f32 height, f32 near, f32 far);

struct ResNodeData;
struct G3DWorkObj;
struct RenderModeObj;
struct ScissorSize;

extern "C" ScissorSize* fn_8004028C(void);
extern "C" f32 fn_8004029C(void);
extern "C" void fn_80041E40(void* pDst, const void* pSrc);
extern "C" void fn_80041E8C(void* pOut, f32 x, f32 y, f32 z);
extern "C" void fn_80043EA8(void* pOut);
extern "C" void fn_800504D4(void* pOut);
extern "C" void* fn_80050508(void* pMtx);
extern "C" void fn_8005050C(void* pOut);
extern "C" void fn_80050850(void* pOut, const void* pIn);
extern "C" s32 fn_800508A8(const void* pIn);
extern "C" void fn_800513CC(void* pOut, const void* pIn);
extern "C" void fn_80051820(void* pOut, const void* pA, const void* pB);
extern "C" f32 fn_80052214(const void* pA, const void* pB);
extern "C" s32 fn_8005AAEC(const void* p);
extern "C" f32* fn_8005CED0(void);
extern "C" u32* fn_8005CEDC(void);
extern "C" void* fn_8005CEF8(void);
extern "C" void* fn_8005D050(const void* p);
extern "C" ResNodeData* fn_8005D0C4(const void* p);
extern "C" void* fn_8005D218(const void* p);
extern "C" void fn_8005D2C0(void* pOut, const void* pIn);
extern "C" s32 fn_8005D2FC(const void* p);
extern "C" void fn_80061068(void* pOut);
extern "C" s32 fn_80067EE8(const void* p);
extern "C" void* fn_800696E4(const void* p);
extern "C" const char* fn_80069748(void);
extern "C" s32 fn_80069754(const void* p);
extern "C" u32 fn_8006FDCC(const void* p);
extern "C" G3DWorkObj* fn_8006FF50(void);
extern "C" s32 fn_8006FFDC(const void* p);
extern "C" s32 fn_80070020(const void* p);
extern "C" s32* fn_80070054(void* pOut, const void* pKey);
extern "C" void* fn_8007012C(void);
extern "C" void fn_8007100C(void* pDst, const void* pSrc);
extern "C" void fn_800710BC(void* pDst, const void* pSrc, const void* pMtx);
extern "C" void fn_800731EC(void* p, u32 v);
extern "C" void* fn_800732F0(const void* p);
extern "C" const char* fn_80073354(void);
extern "C" void fn_80075DCC(f32* pOutSin, f32* pOutCos, f32 angle);
extern "C" void fn_80075DD8(void* p);
extern "C" RenderModeObj* fn_80088584(void);
extern "C" void fn_8008E1C0(void* pOut, const void* pIn);
extern "C" void fn_8008F148(void* pOut, const void* pIn);
extern "C" void* fn_80097D40(void* pA, const void* pB);
extern "C" s32 fn_80097F18(void* pA, u32 idx);
extern "C" u32 fn_80097F80(void* pA);
extern "C" void fn_80098D5C(void* pA, const void* pB);
extern "C" void fn_80098F6C(void* pA, void* pOut);
extern "C" s32 fn_800D77B0(void* pA, void* pB, void* pC, void* pD, s32 e, void* pF);
extern "C" s32 fn_800D7D24(void* pA, void* pB, void* pC, void* pD, s32 e, void* pF);
extern "C" void fn_804BA230(const f32* pViewMtx, const f32* pParams, const f32* pFrustum, void* pA,
                            void* pB, void* pC, f32 x, f32 y, f32 z);
extern "C" void fn_804BA7A0(s32 a, f32 b, f32 c, f32 d, f32 e, f32 f, f32 g);
extern "C" void fn_804C64E0(s32 a, s32 b, s32 c);
extern "C" void fn_804C6660(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f, f32 g, f32 h, f32 i);
extern "C" void fn_804C6710(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);
extern "C" void fn_804C6810(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f, f32 g, f32 h);
extern "C" void fn_8050133C(void* pOut, const void* pA, const void* pB);
extern "C" void fn_8050168C(void* pOut, const void* pIn);

/* The panic file/format strings the target references as map symbols (`lbl_8058Exxx` in `.data`).
 * They are extern here rather than literals: MWCC's `-str reuse` would pool a literal into one blob and
 * address it through a shared base register, while the target loads each one with its own `lis`/`addi`. */
extern const char lbl_8058E070[];
extern const char lbl_8058E098[];
extern const char lbl_8058E0B4[];
extern const char lbl_8058E0D0[];
extern const char lbl_8058E110[];
extern const char lbl_8058E138[];
extern const char lbl_8058E148[];
extern const char lbl_8058E168[];
extern const char lbl_8058E178[];
extern const char lbl_8058E184[];
extern const char lbl_8058E198[];
extern const char lbl_8058E1E0[];
extern const char lbl_8058E208[];
extern const char lbl_8058E240[];
extern const char lbl_8058E2C8[];
extern const char lbl_8058E280[];
extern const char lbl_8058E300[];
extern const char lbl_8058E320[];
extern const char lbl_8058E340[];
extern const char lbl_8058E354[];
extern const char lbl_8058E380[];
extern const char lbl_8058E390[];
extern const char lbl_8058E3B8[];
extern const char lbl_8058E3C8[];
extern const char lbl_8058E3F0[];
extern const char lbl_8058E400[];
extern const char lbl_8058E420[];
extern const char lbl_8058E430[];
extern const char lbl_8058E440[];
extern const char lbl_8058E468[];
extern const char lbl_8058E488[];
extern const char lbl_8058E4E0[];
extern const char lbl_8058E4F8[];
extern const char lbl_8058E520[];
extern const char lbl_8058E534[];
extern const char lbl_8058E55C[];

/* The `.sdata2` pool the target reads (`@sda21`): the float constants are written as literals in the
 * source so MWCC pools them itself and CSEs one load across many stores. The two conversion magics
 * (`2^52` for u16 -> f32, `2^52 + 2^31` for s32 -> f32) are emitted implicitly by the casts. */

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* A `ResCommon<T>`-style one-word handle. The map names the two instantiations' out-of-line copies of
 * the accessor inlines separately (`fn_800733FC`/`fn_800736F0` and friends), so each keeps its `fn_`
 * name rather than being written as a member. */
struct ResHandle {
    /* +0x0 */ void* mpData;
}; /* size: 0x4 */

/* The resource block `fn_800732F0` / `fn_800696E4` hand back. Only the offsets this unit reads are
 * named; the leading words are the resource header (`g3d_rescommon.h`'s `ResCommonData`). */
struct ResVtxNrmBlock {
    /* +0x00 */ u32 mSize;
    /* +0x04 */ u32 mUnk04;
    /* +0x08 */ u32 mToArray; /* byte offset from the block start to the vertex array */
    /* +0x0C */ u32 mUnk0C;
    /* +0x10 */ u32 mUnk10;
    /* +0x14 */ u32 mUnk14;
    /* +0x18 */ u32 mUnk18;
    /* +0x1C */ u16 mUnk1C;
    /* +0x1E */ u16 mCount;
}; /* size: 0x20 */

/* One entry of the 0x10-byte array `fn_80073614` builds: a handle and two more one-word sub-objects
 * (`fn_800731EC` owns the third, it lives in the previous unit). */
struct VtxEntry {
    /* +0x0 */ ResHandle mNrm;
    /* +0x4 */ ResHandle mClr;
    /* +0x8 */ ResHandle mTexCoord;
    /* +0xC */ u32 mUnused0C;
}; /* size: 0x10 */

/* The block `fn_80073614` builds: the entry array's header plus the 32 entries at +0x18. */
struct VtxEntryArray {
    /* +0x00 */ u32 mUnk00;
    /* +0x04 */ u32 mUnk04;
    /* +0x08 */ VtxEntry mHeaderEntry;
    /* +0x18 */ VtxEntry mEntries[32];
}; /* size: 0x218 */

/* The three-word accessor `fn_800736F8` fills in: the value (three floats) and the flag word. */
struct VtxAttrRef {
    /* +0x0 */ u32 mUnused00;
    /* +0x4 */ f32* mpValue;
    /* +0x8 */ u32* mpFlags;
}; /* size: 0xC */

/* The polymorphic node-callback object `fn_80073CE0`/`fn_80073D34`/`fn_80073F00` dispatch through: a
 * vtable at +0x0, a flag byte at +0x4 and the node id at +0x6. */
struct NodeCallback {
    /* +0x0 */ void** mpVtbl;
    /* +0x4 */ u8 mFlags;
    /* +0x5 */ u8 mUnk05;
    /* +0x6 */ u16 mNodeID;
}; /* size: 0x8 */

/* The three-word record `fn_80073DC4` packs for the vtable's `+0x10` slot: the node's matrix, its
 * three-float offset and the matrix id slot. */
struct MtxArg {
    /* +0x0 */ void* mpMtx;
    /* +0x4 */ void* mpVec;
    /* +0x8 */ void* mpMtxID;
}; /* size: 0xC */

/* One node's matrix record (`fn_80073FA0` copies the whole 0x4C): a small header and the 0x30-byte
 * matrix at +0x1C.  Only the field boundaries the copy emits are named. */
struct NodeMtxRec {
    /* +0x00 */ u32 mUnk00;
    /* +0x04 */ f32 mUnk04[2];
    /* +0x0C */ u32 mUnk0C;
    /* +0x10 */ f32 mUnk10[2];
    /* +0x18 */ u32 mUnk18;
    /* +0x1C */ f32 mMtx[12];
}; /* size: 0x4C */

/* The scene/`ResMdl` root `fn_8005D0C4` hands back; only the +0x18 matrix id is read here. */
struct ResNodeData {
    /* +0x00 */ u32 mUnk00;
    /* +0x04 */ u32 mUnk04;
    /* +0x08 */ u32 mUnk08;
    /* +0x0C */ u32 mUnk0C;
    /* +0x10 */ u32 mUnk10;
    /* +0x14 */ u32 mUnk14;
    /* +0x18 */ u32 mMtxID;
}; /* size: 0x1C */

/* The model's resource data (`fn_800740A8` hands it back); +0x4C is the node table's owning object. */
struct ResMdlData {
    /* +0x00 */ u32 mUnk00[19];
    /* +0x4C */ u32 mNodeTableKey;
}; /* size: 0x50 */

/* The engine's shared node/matrix work object `fn_8006FF50` returns. */
struct G3DWorkObj {
    /* +0x00 */ u32 mUnk00;
    /* +0x04 */ u32 mUnk04;
    /* +0x08 */ s32 mNumNode;
    /* +0x0C */ u32 mUnk0C;
    /* +0x10 */ u32 mUnk10;
    /* +0x14 */ u32 mUnk14;
    /* +0x18 */ u32 mUnk18;
    /* +0x1C */ u32 mNumMtx;
    /* +0x20 */ u16 mUnk20;
    /* +0x22 */ u8 mUnk22;
    /* +0x23 */ u8 mEnvelopeMtxMode;
}; /* size: 0x24 */

/* The render-mode object `fn_80088584` returns (`GXRenderModeObj`-shaped). */
struct RenderModeObj {
    /* +0x00 */ u32 mUnk00;
    /* +0x04 */ u16 mUnk04;
    /* +0x06 */ u16 mUnk06;
    /* +0x08 */ u16 mUnk08;
    /* +0x0A */ u16 mUnk0A;
    /* +0x0C */ u16 mUnk0C;
    /* +0x0E */ u16 mUnk0E;
    /* +0x10 */ u16 mUnk10;
    /* +0x12 */ u16 mUnk12;
    /* +0x14 */ u32 mUnk14;
    /* +0x18 */ u8 mUnk18;
}; /* size: 0x1C */

/* The scissor-size source `fn_8004028C` returns. */
struct ScissorSize {
    /* +0x0 */ u16 mWidth;
    /* +0x2 */ u16 mHeight;
}; /* size: 0x4 */

/* The seven-float projection parameter block `fn_804BA230` reads at its second argument. */
struct ProjParams {
    /* +0x00 */ f32 mProjType;
    /* +0x04 */ f32 mUnk04;
    /* +0x08 */ f32 mUnk08;
    /* +0x0C */ f32 mUnk0C;
    /* +0x10 */ f32 mUnk10;
    /* +0x14 */ f32 mUnk14;
    /* +0x18 */ f32 mUnk18;
}; /* size: 0x1C */

/* The camera payload `Camera::mpData` points at (`g3d_camera.cpp`'s `CameraData`). The 0x70 flag word
 * records which posture/projection branch is current. */
struct CameraData {
    /* +0x00 */ f32 mViewMtx[3][4];
    /* +0x30 */ f32 mProjMtx[3][4];
    /* +0x60 */ u32 mUnk60[4];
    /* +0x70 */ u32 mFlags;
    /* +0x74 */ f32 mPosX;
    /* +0x78 */ f32 mPosY;
    /* +0x7C */ f32 mPosZ;
    /* +0x80 */ f32 mTargetX;
    /* +0x84 */ f32 mTargetY;
    /* +0x88 */ f32 mTargetZ;
    /* +0x8C */ f32 mUpX;
    /* +0x90 */ f32 mUpY;
    /* +0x94 */ f32 mUpZ;
    /* +0x98 */ f32 mUnk98;
    /* +0x9C */ f32 mUnk9C;
    /* +0xA0 */ f32 mUnkA0;
    /* +0xA4 */ f32 mUnkA4;
    /* +0xA8 */ s32 mProjType;
    /* +0xAC */ f32 mProjA;
    /* +0xB0 */ f32 mProjB;
    /* +0xB4 */ f32 mProjC;
    /* +0xB8 */ f32 mProjD;
    /* +0xBC */ f32 mUnkBC;
    /* +0xC0 */ f32 mUnkC0;
    /* +0xC4 */ f32 mUnkC4;
    /* +0xC8 */ f32 mUnkC8;
    /* +0xCC */ f32 mUnkCC;
    /* +0xD0 */ f32 mUnkD0;
    /* +0xD4 */ f32 mUnkD4;
    /* +0xD8 */ f32 mUnkD8;
    /* +0xDC */ f32 mViewportX;
    /* +0xE0 */ f32 mViewportY;
    /* +0xE4 */ f32 mViewportW;
    /* +0xE8 */ f32 mViewportH;
    /* +0xEC */ f32 mViewportNear;
    /* +0xF0 */ f32 mViewportFar;
    /* +0xF4 */ s32 mScissorX;
    /* +0xF8 */ s32 mScissorY;
    /* +0xFC */ s32 mScissorW;
    /* +0x100 */ s32 mScissorH;
    /* +0x104 */ s32 mScissorOffsetX;
    /* +0x108 */ s32 mScissorOffsetY;
}; /* size: 0x10C */

/* `nw4r::g3d::Camera` itself: a `ResCommon<CameraData>` handle, so the payload is the first word. The
 * three mangled map symbols require the real class, which is why this one is a class and not a struct. */
namespace nw4r {
namespace g3d {

class Camera {
public:
    struct PostureInfo {
        /* +0x00 */ s32 mType;
        /* +0x04 */ f32 mPosX;
        /* +0x08 */ f32 mPosY;
        /* +0x0C */ f32 mPosZ;
        /* +0x10 */ f32 mTargetX;
        /* +0x14 */ f32 mTargetY;
        /* +0x18 */ f32 mTargetZ;
        /* +0x1C */ f32 mUpX;
        /* +0x20 */ f32 mUpY;
        /* +0x24 */ f32 mUpZ;
        /* +0x28 */ f32 mUnk28;
    }; /* size: 0x2C */

    void SetPosition(const math::VEC3& rPos);
    void SetPosture(const PostureInfo& rInfo);
    void SetPerspective(f32 fovy, f32 aspect, f32 near, f32 far);

    /* +0x0 */ CameraData* mpData;
}; /* size: 0x4 */

}  // namespace g3d
}  // namespace nw4r

extern "C" {

/* ------------------------------------------------------------------------------------------------ */
/* forward declarations (one per function this unit defines; keeps the source order free)             */
/* ------------------------------------------------------------------------------------------------ */

void* fn_800733FC(ResHandle* pSelf);
void fn_80073468(ResHandle* pSelf, u32 value);
bool fn_80073494(ResHandle* pSelf);
void* fn_80073398(ResHandle* pSelf);
ResHandle* fn_80073404(ResHandle* pSelf, u32 pData);
void fn_800734D8(ResHandle* pSelf, const ResHandle* pRhs);
ResHandle* fn_800734A8(ResHandle* pSelf, const ResHandle* pRhs);
void fn_800734E4(void* pBase, u32 size);
u32 fn_80073470(ResHandle* pSelf);
u16 fn_800734E8(ResHandle* pSelf);
void* fn_800736F0(ResHandle* pSelf);
ResVtxNrmBlock* fn_80073544(ResHandle* pSelf);
void fn_8007360C(ResHandle* pSelf, u32 value);
ResHandle* fn_800735A8(ResHandle* pSelf, u32 pData);
void* fn_8007350C(ResHandle* pSelf);
VtxEntry* fn_800736A4(VtxEntry* pSelf);
VtxEntry* fn_80073674(VtxEntry* pSelf);
VtxEntryArray* fn_80073614(VtxEntryArray* pSelf);
void fn_800736F8(VtxAttrRef* pSelf, f32 x, f32 y, f32 z);
u32 fn_800737AC(u32 value);
u32 fn_800737B4(u32 value);
u32 fn_800737BC(u32 value);
u32 fn_800737C4(u32 value);
void fn_80073CE0(NodeCallback* pSelf, void* pMtx, s32* pMtxID);
void fn_80073D30(void);
void fn_80073D34(NodeCallback* pSelf, u32 id, void* pMtx, void* pVec, s32* pMtxID, s32* pArg5);
void fn_80073DC0(void);
MtxArg* fn_80073DC4(MtxArg* pSelf, void* pM, void* pS, void* pAttr);
u32 fn_80073E80(u32 value, u32 low);
u32 fn_80073E8C(void* pSelf);
void fn_80073F00(NodeCallback* pSelf, u32 id, s32* pMtx, s32* pMtxID);
void fn_80073F64(void);
nw4r::math::VEC3* fn_80073F68(nw4r::math::VEC3* pOut, const nw4r::math::VEC3* pIn);
void fn_80073FA0(NodeMtxRec* pDst, const NodeMtxRec* pSrc);
s32 fn_8007403C(u32 flags);
s32 fn_80074050(const void* p);
s32 fn_80074074(ResHandle* pMdl);
void fn_800737CC(u8* pMtxArray, s32* pMtxIDs, u8* pByteCode, const void* pMtx, ResHandle* pMdl,
                 NodeCallback* pNodeCallback, NodeCallback* pNodeCallback2, u32 flags);
ResMdlData* fn_800740A8(ResHandle* pMdl);
void* fn_8007410C(ResHandle* pSelf);
u32 fn_80074114(void);
u32 fn_80074620(const ResHandle* pSelf);
s32 fn_80074644(const ResHandle* pSelf);
void fn_80074698(ResHandle* pSelf, const ResHandle* pRhs);
ResHandle* fn_80074668(ResHandle* pSelf, const ResHandle* pRhs);
void fn_800746D4(ResHandle* pSelf, u32 value);
ResHandle* fn_800746A4(ResHandle* pSelf, u32 value);
CameraData* fn_800748E4(nw4r::g3d::Camera* pSelf);
CameraData* fn_80074A54(nw4r::g3d::Camera* pSelf);
void fn_800746DC(nw4r::g3d::Camera* pSelf);
void fn_80074758(nw4r::g3d::Camera* pSelf, u16 a1, u16 a2, u16 a3, u16 a4, u16 a5, u16 a6);
void fn_800749C8(nw4r::g3d::Camera* pSelf, nw4r::math::VEC3* pOut);
void fn_80074AA8(nw4r::g3d::Camera* pSelf, f32* pX, f32* pY, f32* pZ);
s32 fn_80074D38(const f32* pA, const f32* pB);
void fn_80074D78(nw4r::g3d::Camera* pSelf, nw4r::g3d::Camera::PostureInfo* pOut);
void fn_80074F24(nw4r::g3d::Camera* pSelf, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);
void fn_80074FEC(nw4r::g3d::Camera* pSelf, u32 x, u32 y, u32 w, u32 h);
void fn_8007507C(nw4r::g3d::Camera* pSelf, f32 a, f32 b, f32 c, f32 d);
void fn_80075170(nw4r::g3d::Camera* pSelf, f32* p1, f32* p2, f32* p3, f32* p4, f32* p5, f32* p6);
void fn_80075258(nw4r::g3d::Camera* pSelf, u8* pOut, const nw4r::math::VEC3* pVec);
void fn_80075390(void* pOut);
void fn_80075394(nw4r::g3d::Camera* pSelf, void* pOut);
void fn_80075440(nw4r::g3d::Camera* pSelf, void* pOut);
void fn_800754EC(nw4r::g3d::Camera* pSelf, void* pOut);
void fn_80075620(nw4r::g3d::Camera* pSelf, void* pOut);
void fn_800756DC(nw4r::g3d::Camera* pSelf);
void fn_800757A8(nw4r::g3d::Camera* pSelf);
void fn_80075844(void* pMtx);
void fn_80075848(nw4r::g3d::Camera* pSelf);
void fn_800758C8(nw4r::g3d::Camera* pSelf);
void fn_80075940(nw4r::g3d::Camera* pSelf);

/* ------------------------------------------------------------------------------------------------ */
/* g3d_resvtx_ac.h: the out-of-line accessor inlines                                                 */
/* ------------------------------------------------------------------------------------------------ */

void* fn_800733FC(ResHandle* pSelf) {
    return pSelf->mpData;
}

void fn_80073468(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

bool fn_80073494(ResHandle* pSelf) {
    return pSelf->mpData != NULL;
}

/* The line-98 instantiation: assert validity, then return the data word. */
void* fn_80073398(ResHandle* pSelf) {
    if (!fn_80073494(pSelf)) {
        nw4r::db::Panic(lbl_8058E0D0, 98, lbl_8058E0B4, fn_80073354(), "ref");
    }
    return fn_800733FC(pSelf);
}

/* The line-98 alignment-checked setter. */
ResHandle* fn_80073404(ResHandle* pSelf, u32 pData) {
    fn_80073468(pSelf, pData);
    if ((pData & 3) != 0) {
        nw4r::db::Panic(lbl_8058E098, 98, lbl_8058E070);
    }
    return pSelf;
}

/* The line-98 copy assignment. */
void fn_800734D8(ResHandle* pSelf, const ResHandle* pRhs) {
    pSelf->mpData = pRhs->mpData;
}

ResHandle* fn_800734A8(ResHandle* pSelf, const ResHandle* pRhs) {
    fn_800734D8(pSelf, pRhs);
    return pSelf;
}

void fn_800734E4(void* pBase, u32 size) {
    DCStoreRange(pBase, size);
}

u32 fn_80073470(ResHandle* pSelf) {
    ResVtxNrmBlock* pBlock = (ResVtxNrmBlock*)fn_800732F0(pSelf);
    return pBlock->mUnk10;
}

u16 fn_800734E8(ResHandle* pSelf) {
    ResVtxNrmBlock* pBlock = (ResVtxNrmBlock*)fn_800696E4(pSelf);
    return pBlock->mCount;
}

/* The line-39 instantiation: return the data word (the validity assert lives in `fn_80073544`). */
void* fn_800736F0(ResHandle* pSelf) {
    return pSelf->mpData;
}

ResVtxNrmBlock* fn_80073544(ResHandle* pSelf) {
    if (!fn_80069754(pSelf)) {
        nw4r::db::Panic(lbl_8058E168, 39, lbl_8058E148, fn_80069748(), "ref");
    }
    return (ResVtxNrmBlock*)fn_800736F0(pSelf);
}

/* The line-39 setter. */
void fn_8007360C(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

ResHandle* fn_800735A8(ResHandle* pSelf, u32 pData) {
    fn_8007360C(pSelf, pData);
    if ((pData & 3) != 0) {
        nw4r::db::Panic(lbl_8058E138, 39, lbl_8058E110);
    }
    return pSelf;
}

/* The resource's vertex array: the header's byte offset, or NULL when there is none. */
void* fn_8007350C(ResHandle* pSelf) {
    ResVtxNrmBlock* pBlock = fn_80073544(pSelf);
    if (pBlock->mToArray != 0) {
        return (u8*)pBlock + pBlock->mToArray;
    }
    return NULL;
}

/* Construct one 0x10-byte entry: the three sub-handles cleared. */
VtxEntry* fn_800736A4(VtxEntry* pSelf) {
    fn_800735A8(&pSelf->mNrm, 0);
    fn_80073404(&pSelf->mClr, 0);
    fn_800731EC(&pSelf->mTexCoord, 0);
    return pSelf;
}

VtxEntry* fn_80073674(VtxEntry* pSelf) {
    fn_800736A4(pSelf);
    return pSelf;
}

/* Construct the header entry and the 32-entry array. */
VtxEntryArray* fn_80073614(VtxEntryArray* pSelf) {
    fn_800736A4(&pSelf->mHeaderEntry);
    for (VtxEntry* pEntry = pSelf->mEntries; pEntry < pSelf->mEntries + 32; pEntry++) {
        fn_80073674(pEntry);
    }
    return pSelf;
}

/* Store the three scale values and pick the scale-mode flag word: 0x40000000 when all three are 1.0f,
 * 0x10000000 then 0x3FFFFFFF when they are equal but not 1.0f, otherwise just clear the mode bits. */
void fn_800736F8(VtxAttrRef* pSelf, f32 x, f32 y, f32 z) {
    pSelf->mpValue[0] = x;
    pSelf->mpValue[1] = y;
    pSelf->mpValue[2] = z;
    if (x == y && x == z) {
        if (1.0f == x) {
            *pSelf->mpFlags = fn_800737C4(*pSelf->mpFlags);
            return;
        }
        u32* pFlags = pSelf->mpFlags;
        *pFlags = fn_800737BC(*pFlags);
        *pSelf->mpFlags = fn_800737B4(*pSelf->mpFlags);
        return;
    }
    *pSelf->mpFlags = fn_800737AC(*pSelf->mpFlags);
}

u32 fn_800737AC(u32 value) {
    return value & 0x0FFFFFFF;
}

u32 fn_800737B4(u32 value) {
    return value & 0x3FFFFFFF;
}

u32 fn_800737BC(u32 value) {
    return value | 0x10000000;
}

u32 fn_800737C4(u32 value) {
    return value | 0x40000000;
}

/* ------------------------------------------------------------------------------------------------ */
/* g3d_calcworld.cpp: the node-callback helpers                                                      */
/* ------------------------------------------------------------------------------------------------ */

/* Walk a `ResByteCodeData` node tree: build each node's matrix record, compute its world matrix
 * (`fn_800D77B0` / `fn_800D7D24`), then scale the matrices the byte code marked. */
void fn_800737CC(u8* pMtxArray, s32* pMtxIDs, u8* pByteCode, const void* pMtx, ResHandle* pMdl,
                 NodeCallback* pNodeCallback, NodeCallback* pNodeCallback2, u32 flags) {
    s32 mayaDisable;
    u8 singleMtx;
    NodeMtxRec rec;
    nw4r::math::VEC3 vA;
    nw4r::math::VEC3 vB;
    s32 s1C;
    s32 s18;
    s32 s14;
    s32 s10;
    s32 sC;
    s32 s8;
    u8* pCode = pByteCode;
    fn_80061068(&rec);
    if (pCode == NULL) {
        pCode = (u8*)fn_80097D40(pMdl, lbl_8058E178);
    }
    if (pCode != NULL) {
        NodeMtxRec* pRec = NULL;
        s18 = fn_80074074(pMdl);
        singleMtx = (fn_80074050(&s18) == 1);
        mayaDisable = fn_8007403C(flags);
        f32* pScale = fn_8005CED0();
        pScale[0] = 1.0f;
        pScale[1] = 1.0f;
        pScale[2] = 1.0f;
        fn_8007100C(pMtxArray, pMtx);
        u32* pMtxIDList = fn_8005CEDC();
        u32 numMtx = 0;
        *pMtxIDs = (s32)flags;
        for (;;) {
            if (pCode[0] == 1) {
                break;
            }
            switch (pCode[0]) {
            case 2: {
                u32 nodeID = (pCode[1] << 8) + pCode[2];
                u32 targetID = (pCode[3] << 8) + pCode[4];
                s14 = fn_80097F18(pMdl, nodeID);
                fn_8005D2C0(&s1C, &s14);
                u32 mtxID = fn_8006FDCC(&s1C);
                if (numMtx >= 0x800) {
                    nw4r::db::Panic(lbl_8058E184, 137, lbl_8058E198);
                }
                pMtxIDList[numMtx++] = mtxID;
                u8* pDstMtx = pMtxArray + mtxID * 0x30;
                f32* pDstScale = pScale + mtxID * 3;
                u8* pSrcMtx = pMtxArray + targetID * 0x30;
                f32* pSrcScale = pScale + targetID * 3;
                s32 prev = pMtxIDs[targetID];
                if (pNodeCallback != NULL) {
                    void* (*pGetRec)(void*, NodeMtxRec*, void*) =
                        (void* (*)(void*, NodeMtxRec*, void*))pNodeCallback->mpVtbl[14];
                    pRec = (NodeMtxRec*)pGetRec(pNodeCallback, &rec, fn_8005D050(&s1C));
                }
                s32 reset;
                if (pNodeCallback == NULL || pRec->mUnk00 == 0) {
                    fn_80098F6C(&s1C, &rec);
                    pRec = &rec;
                    reset = 1;
                } else {
                    reset = 0;
                }
                if (nodeID != 0 && mayaDisable != 0 && reset == 0) {
                    fn_80043EA8(&vA);
                    fn_80043EA8(&vB);
                    if (pRec != &rec) {
                        fn_80073FA0(&rec, pRec);
                        pRec = &rec;
                    }
                    fn_8008E1C0(&rec, &vA);
                    rec.mUnk00 |= 0x200;
                    fn_80098D5C(&s1C, &rec);
                    fn_8008E1C0(&rec, &vB);
                    fn_80073F68(&vB, &vA);
                    fn_8008F148(&rec, &vB);
                }
                if (pNodeCallback2 != NULL) {
                    if (pRec != &rec) {
                        fn_80073FA0(&rec, pRec);
                        pRec = &rec;
                    }
                    s10 = *(s32*)pMdl;
                    fn_80073F00(pNodeCallback2, nodeID, (s32*)&rec, &s10);
                }
                if (singleMtx != 0) {
                    pMtxIDs[mtxID] =
                        fn_800D77B0(pDstMtx, pDstScale, pSrcMtx, pSrcScale, prev, pRec);
                } else if ((pRec->mUnk00 & 0x400) != 0) {
                    nw4r::db::Panic(lbl_8058E184, 245, lbl_8058E1E0);
                    pMtxIDs[mtxID] =
                        fn_800D7D24(pDstMtx, pDstScale, pSrcMtx, pSrcScale, prev, pRec);
                } else {
                    pMtxIDs[mtxID] =
                        fn_800D7D24(pDstMtx, pDstScale, pSrcMtx, pSrcScale, prev, pRec);
                }
                pMtxIDs[mtxID] = fn_80073E80(pMtxIDs[mtxID], fn_80073E8C(&s1C));
                if (pNodeCallback2 != NULL) {
                    sC = *(s32*)pMdl;
                    fn_80073D34(pNodeCallback2, nodeID, pDstMtx, pDstScale, &pMtxIDs[mtxID], &sC);
                }
                pCode += 5;
                break;
            }
            default:
                nw4r::db::Panic(lbl_8058E184, 273, lbl_8058E208);
                /* fallthrough */
            case 6: {
                u32 nodeID = (pCode[1] << 8) + pCode[2];
                u32 targetID = (pCode[3] << 8) + pCode[4];
                pMtxIDList[numMtx++] = nodeID;
                pMtxIDs[nodeID] = pMtxIDs[targetID];
                fn_8007100C(pMtxArray + nodeID * 0x30, pMtxArray + targetID * 0x30);
                fn_80041E40(pScale + nodeID * 3, pScale + targetID * 3);
                pCode += 5;
                break;
            }
            }
        }
        for (u32 i = 0; i < numMtx; i++) {
            u32 mtxID = pMtxIDList[i];
            if (mtxID >= 0x800) {
                nw4r::db::Panic(lbl_8058E184, 294, lbl_8058E240);
            }
            f32* pS = pScale + mtxID * 3;
            if (1.0f != pS[0] || 1.0f != pS[1] || 1.0f != pS[2]) {
                fn_8050133C(pMtxArray + mtxID * 0x30, pMtxArray + mtxID * 0x30, pS);
            }
        }
        if (pNodeCallback2 != NULL) {
            s8 = *(s32*)pMdl;
            fn_80073CE0(pNodeCallback2, pMtxArray, &s8);
        }
    }
}

/* Notify the callback object's `+0x14` vtable slot that one node's matrix is ready. */
void fn_80073CE0(NodeCallback* pSelf, void* pMtx, s32* pMtxID) {
    if ((pSelf->mFlags & 4) != 0) {
        s32 mtxID = *pMtxID;
        void (*pNotify)(void*, void*, s32*, void*) = (void (*)(void*, void*, s32*, void*))pSelf->mpVtbl[5];
        pNotify(pSelf->mpVtbl, pMtx, &mtxID, pSelf);
    }
}

void fn_80073D30(void) {
}

/* Notify the callback object's `+0x10` vtable slot for the matching node id. */
void fn_80073D34(NodeCallback* pSelf, u32 id, void* pMtx, void* pVec, s32* pMtxID, s32* pArg5) {
    if (id == pSelf->mNodeID && (pSelf->mFlags & 2) != 0) {
        MtxArg arg;
        fn_80073DC4(&arg, pMtx, pVec, pMtxID);
        s32 mtxID = *pArg5;
        void (*pNotify)(void*, void*, s32*, void*) = (void (*)(void*, void*, s32*, void*))pSelf->mpVtbl[4];
        pNotify(pSelf->mpVtbl, &arg, &mtxID, pSelf);
    }
}

void fn_80073DC0(void) {
}

MtxArg* fn_80073DC4(MtxArg* pSelf, void* pM, void* pS, void* pAttr) {
    pSelf->mpMtx = pM;
    pSelf->mpVec = pS;
    pSelf->mpMtxID = pAttr;
    if (pM == NULL) {
        nw4r::db::Panic(lbl_8058E3F0, 46, lbl_8058E3C8);
    }
    if (pS == NULL) {
        nw4r::db::Panic(lbl_8058E3B8, 47, lbl_8058E390);
    }
    if (pAttr == NULL) {
        nw4r::db::Panic(lbl_8058E380, 48, lbl_8058E354);
    }
    return pSelf;
}

u32 fn_80073E80(u32 value, u32 low) {
    return (value & 0xFFFFFF00) | low;
}

u32 fn_80073E8C(void* pSelf) {
    if (!fn_8005AAEC(pSelf)) {
        nw4r::db::Panic(lbl_8058E340, 90, lbl_8058E320);
    }
    if (fn_8005AAEC(pSelf)) {
        return ((ResNodeData*)fn_8005D0C4(pSelf))->mMtxID;
    }
    return 0;
}

/* Notify the callback object's `+0xC` vtable slot for the matching node id. */
void fn_80073F00(NodeCallback* pSelf, u32 id, s32* pMtx, s32* pMtxID) {
    if (id == pSelf->mNodeID && (pSelf->mFlags & 1) != 0) {
        s32 mtxID = *pMtxID;
        void (*pNotify)(void*, void*, s32*, void*) = (void (*)(void*, void*, s32*, void*))pSelf->mpVtbl[3];
        pNotify(pSelf->mpVtbl, pMtx, &mtxID, pSelf);
    }
}

void fn_80073F64(void) {
}

nw4r::math::VEC3* fn_80073F68(nw4r::math::VEC3* pOut, const nw4r::math::VEC3* pIn) {
    fn_800513CC(pOut, pIn);
    return pOut;
}

/* Copy one node's 0x4C-byte matrix record. */
void fn_80073FA0(NodeMtxRec* pDst, const NodeMtxRec* pSrc) {
    *pDst = *pSrc;
}

/* `#pragma peephole off` is load-bearing here: at `-O3` the peephole folds the masked compare into a
 * single `rlwinm r3,r3,5,31,31`, while the target keeps the generic `!= 0` conversion
 * (`rlwinm r3,r3,0,4,4; neg; or; srwi`). Scoped to this one function (playbook 32). */
#pragma peephole off
s32 fn_8007403C(u32 flags) {
    return (flags & 0x08000000) != 0;
}
#pragma peephole on

s32 fn_80074050(const void* p) {
    return fn_8006FF50()->mNumNode;
}

/* Look one entry up in the model's node table (`fn_800740A8` hands back the model data, +0x4C is the
 * table's owning object). */
s32 fn_80074074(ResHandle* pMdl) {
    ResMdlData* pData = fn_800740A8(pMdl);
    s32 idx;
    return *fn_80070054(&idx, &pData->mNodeTableKey);
}

/* The `ResMdl` root, with the `g3d_resmdl_ac.h` validity assert. */
ResMdlData* fn_800740A8(ResHandle* pMdl) {
    if (!fn_8005D2FC(pMdl)) {
        nw4r::db::Panic(lbl_8058E420, 120, lbl_8058E400, fn_8007012C(), "ref");
    }
    return (ResMdlData*)fn_8007410C(pMdl);
}

void* fn_8007410C(ResHandle* pSelf) {
    return pSelf->mpData;
}

u32 fn_80074114(void) {
    return 0xF0000000;
}

/* ------------------------------------------------------------------------------------------------ */
/* g3d_calcworld.cpp: the node-table accessors                                                       */
/* ------------------------------------------------------------------------------------------------ */

u32 fn_80074620(const ResHandle* pSelf) {
    return fn_8006FF50()->mNumMtx;
}

s32 fn_80074644(const ResHandle* pSelf) {
    return fn_8006FF50()->mEnvelopeMtxMode;
}

void fn_80074698(ResHandle* pSelf, const ResHandle* pRhs) {
    pSelf->mpData = pRhs->mpData;
}

ResHandle* fn_80074668(ResHandle* pSelf, const ResHandle* pRhs) {
    fn_80074698(pSelf, pRhs);
    return pSelf;
}

void fn_800746D4(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

ResHandle* fn_800746A4(ResHandle* pSelf, u32 value) {
    fn_800746D4(pSelf, value);
    return pSelf;
}

/* ------------------------------------------------------------------------------------------------ */
/* g3d_camera.cpp                                                                                    */
/* ------------------------------------------------------------------------------------------------ */

/* The `ResCommon<CameraData>::ref()`-style accessor (g3d_rescommon_ac.h:140). */
CameraData* fn_800748E4(nw4r::g3d::Camera* pSelf) {
    if (pSelf->mpData == NULL) {
        nw4r::db::Panic(lbl_8058E520, 140, lbl_8058E4F8);
    }
    return pSelf->mpData;
}

/* The same accessor's second instantiation (g3d_rescommon_ac.h:143). */
CameraData* fn_80074A54(nw4r::g3d::Camera* pSelf) {
    if (pSelf->mpData == NULL) {
        nw4r::db::Panic(lbl_8058E55C, 143, lbl_8058E534);
    }
    return pSelf->mpData;
}

/* Reset every camera field from the current render mode. */
void fn_800746DC(nw4r::g3d::Camera* pSelf) {
    RenderModeObj* pMode = (RenderModeObj*)fn_80088584();
    if (pMode == NULL) {
        nw4r::db::Panic(lbl_8058E430, 50, lbl_8058E440);
    }
    u16 width = pMode->mUnk04;
    fn_80074758(pSelf, width, pMode->mUnk06, width, pMode->mUnk08, pMode->mUnk0E, pMode->mUnk10);
}

void fn_80074758(nw4r::g3d::Camera* pSelf, u16 a1, u16 a2, u16 a3, u16 a4, u16 a5, u16 a6) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 62, lbl_8058E468);
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    ScissorSize* pSize = (ScissorSize*)fn_8004028C();
    CameraData* pData = fn_800748E4(pSelf);
    pData->mFlags = 0x21;
    pData->mPosX = 0.0f;
    pData->mPosY = 0.0f;
    pData->mPosZ = 15.0f;
    pData->mTargetX = 0.0f;
    pData->mTargetY = 1.0f;
    pData->mTargetZ = 0.0f;
    pData->mUpX = 0.0f;
    pData->mUpY = 0.0f;
    pData->mUpZ = 0.0f;
    pData->mUnk98 = 0.0f;
    pData->mUnk9C = 0.0f;
    pData->mUnkA0 = 0.0f;
    pData->mUnkA4 = 0.0f;
    pData->mProjType = 0;
    pData->mProjA = 60.0f;
    pData->mProjB = fn_8004029C();
    pData->mProjC = 0.1f;
    pData->mProjD = 1000.0f;
    pData->mUnkBC = 0.0f;
    pData->mUnkC0 = 448.0f;
    pData->mUnkC4 = 0.0f;
    pData->mUnkC8 = (f32)a5;
    pData->mUnkCC = 0.5f;
    pData->mUnkD0 = 0.5f;
    pData->mUnkD4 = 0.5f;
    pData->mUnkD8 = 0.5f;
    pData->mViewportX = 0.0f;
    pData->mViewportY = 0.0f;
    pData->mViewportW = (f32)a3;
    pData->mViewportH = 448.0f;
    pData->mViewportNear = 0.0f;
    pData->mViewportFar = 1.0f;
    pData->mScissorX = 0;
    pData->mScissorY = 0;
    pData->mScissorW = pSize->mWidth;
    pData->mScissorH = pSize->mHeight;
    pData->mScissorOffsetX = 0;
    pData->mScissorOffsetY = 0;
}

} /* extern "C" */

void nw4r::g3d::Camera::SetPosition(const math::VEC3& rPos) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic(lbl_8058E430, 180, lbl_8058E468);
    }
    if (fn_80067EE8(this)) {
        CameraData* pData = fn_800748E4(this);
        fn_80041E40(&pData->mPosX, &rPos);
        pData->mFlags &= ~8;
    }
}
extern "C" {


void fn_800749C8(nw4r::g3d::Camera* pSelf, nw4r::math::VEC3* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 195, lbl_8058E468);
    }
    if (pOut != NULL && fn_80067EE8(pSelf)) {
        CameraData* pData = fn_80074A54(pSelf);
        fn_80041E40(pOut, &pData->mPosX);
    }
}

void fn_80074AA8(nw4r::g3d::Camera* pSelf, f32* pX, f32* pY, f32* pZ) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 206, lbl_8058E468);
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = fn_80074A54(pSelf);
        if (pX != NULL) {
            *pX = pData->mPosX;
        }
        if (pY != NULL) {
            *pY = pData->mPosY;
        }
        if (pZ != NULL) {
            *pZ = pData->mPosZ;
        }
    }
}

} /* extern "C" */

void nw4r::g3d::Camera::SetPosture(const PostureInfo& rInfo) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic(lbl_8058E430, 226, lbl_8058E468);
    }
    if (!fn_80067EE8(this)) {
        return;
    }
    CameraData* pData = fn_800748E4(this);
    switch (rInfo.mType) {
    case 0: {
        if ((pData->mFlags & 1) == 0 || fn_80074D38(&rInfo.mPosX, &pData->mTargetX) != 0 ||
            fn_80074D38(&rInfo.mTargetX, &pData->mUpX) != 0) {
            pData->mFlags &= ~7;
            pData->mFlags |= 1;
            fn_80041E40(&pData->mTargetX, &rInfo.mPosX);
            fn_80041E40(&pData->mUpX, &rInfo.mTargetX);
            pData->mFlags &= ~8;
        }
        break;
    }
    case 1: {
        if ((pData->mFlags & 2) == 0 || fn_80074D38(&rInfo.mUpX, &pData->mUnk98) != 0) {
            pData->mFlags &= ~7;
            pData->mFlags |= 2;
            fn_80041E40(&pData->mUnk98, &rInfo.mUpX);
            pData->mFlags &= ~8;
        }
        break;
    }
    case 2: {
        if ((pData->mFlags & 4) == 0 || fn_80074D38(&rInfo.mTargetX, &pData->mUpX) != 0 ||
            rInfo.mUnk28 != pData->mUnkA4) {
            pData->mFlags &= ~7;
            pData->mFlags |= 4;
            fn_80041E40(&pData->mUpX, &rInfo.mTargetX);
            pData->mUnkA4 = rInfo.mUnk28;
            pData->mFlags &= ~8;
        }
        break;
    }
    }
}
extern "C" {


/* True when the two three-float vectors differ. */
s32 fn_80074D38(const f32* pA, const f32* pB) {
    s32 result = 0;
    if (pA[0] != pB[0] || pA[1] != pB[1] || pA[2] != pB[2]) {
        result = 1;
    }
    return result;
}

void fn_80074D78(nw4r::g3d::Camera* pSelf, nw4r::g3d::Camera::PostureInfo* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 282, lbl_8058E468);
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    u32 flags = pData->mFlags;
    if ((flags & 1) != 0) {
        pOut->mType = 0;
        fn_80041E40(&pOut->mPosX, &pData->mTargetX);
        fn_80041E40(&pOut->mTargetX, &pData->mUpX);
        return;
    }
    if ((flags & 2) != 0) {
        pOut->mType = 1;
        fn_80041E40(&pOut->mUpX, &pData->mUnk98);
        return;
    }
    pOut->mType = 2;
    fn_80041E40(&pOut->mTargetX, &pData->mUpX);
    pOut->mUnk28 = pData->mUnkA4;
}

} /* extern "C" */

void nw4r::g3d::Camera::SetPerspective(f32 fovy, f32 aspect, f32 near, f32 far) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic(lbl_8058E430, 329, lbl_8058E468);
    }
    if (fn_80067EE8(this)) {
        CameraData* pData = fn_800748E4(this);
        pData->mProjType = 0;
        pData->mProjA = fovy;
        pData->mProjB = aspect;
        pData->mProjC = near;
        pData->mProjD = far;
        pData->mFlags &= ~0xF0;
        pData->mFlags |= 0x20;
    }
}
extern "C" {


void fn_80074F24(nw4r::g3d::Camera* pSelf, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 371, lbl_8058E468);
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = fn_800748E4(pSelf);
        pData->mProjType = 1;
        pData->mUnkBC = a;
        pData->mUnkC0 = b;
        pData->mUnkC4 = c;
        pData->mUnkC8 = d;
        pData->mProjC = e;
        pData->mProjD = f;
        pData->mFlags &= ~0xF0;
        pData->mFlags |= 0x40;
    }
}

void fn_80074FEC(nw4r::g3d::Camera* pSelf, u32 x, u32 y, u32 w, u32 h) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 430, lbl_8058E468);
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = fn_800748E4(pSelf);
        pData->mScissorX = x;
        pData->mScissorY = y;
        pData->mScissorW = w;
        pData->mScissorH = h;
    }
}

void fn_8007507C(nw4r::g3d::Camera* pSelf, f32 a, f32 b, f32 c, f32 d) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 463, lbl_8058E468);
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = fn_800748E4(pSelf);
        pData->mViewportX = a;
        pData->mViewportY = b;
        pData->mViewportW = c;
        pData->mViewportH = d;
        fn_80074FEC(pSelf, (u32)d, (u32)c, (u32)b, (u32)a);
    }
}

void fn_80075170(nw4r::g3d::Camera* pSelf, f32* p1, f32* p2, f32* p3, f32* p4, f32* p5, f32* p6) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 521, lbl_8058E468);
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = fn_80074A54(pSelf);
        if (p1 != NULL) {
            *p1 = pData->mViewportX;
        }
        if (p2 != NULL) {
            *p2 = pData->mViewportY;
        }
        if (p3 != NULL) {
            *p3 = pData->mViewportW;
        }
        if (p4 != NULL) {
            *p4 = pData->mViewportH;
        }
        if (p5 != NULL) {
            *p5 = pData->mViewportNear;
        }
        if (p6 != NULL) {
            *p6 = pData->mViewportFar;
        }
    }
}

void fn_80075258(nw4r::g3d::Camera* pSelf, u8* pOut, const nw4r::math::VEC3* pVec) {
    if (pOut == NULL) {
        return;
    }
    f32 viewMtx[12];
    f32 projMtx[16];
    ProjParams params;
    f32 frustum[6];
    fn_8005050C(viewMtx);
    fn_80075390(projMtx);
    fn_80075394(pSelf, viewMtx);
    fn_80075440(pSelf, projMtx);
    CameraData* pData = fn_80074A54(pSelf);
    f32 projType = (f32)pData->mProjType;
    params.mProjType = projType;
    params.mUnk04 = projMtx[0];
    params.mUnk0C = projMtx[5];
    params.mUnk14 = projMtx[10];
    params.mUnk18 = projMtx[11];
    if (1.0f == projType) {
        params.mUnk08 = projMtx[3];
        params.mUnk10 = projMtx[7];
    } else {
        params.mUnk08 = projMtx[2];
        params.mUnk10 = projMtx[6];
    }
    fn_80075170(pSelf, &frustum[0], &frustum[1], &frustum[2], &frustum[3], &frustum[4], &frustum[5]);
    fn_80050508(viewMtx);
    fn_804BA230(viewMtx, &params.mProjType, frustum, pOut, pOut + 4, pOut + 8, pVec->x, pVec->y,
                pVec->z);
}

void fn_80075390(void* pOut) {
}

void fn_80075394(nw4r::g3d::Camera* pSelf, void* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 629, lbl_8058E468);
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    if ((pData->mFlags & 8) == 0) {
        fn_80075940(pSelf);
    }
    fn_8007100C(pOut, pData);
}

void fn_80075440(nw4r::g3d::Camera* pSelf, void* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 647, lbl_8058E468);
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    if ((pData->mFlags & 0x80) == 0) {
        fn_80075DD8(pSelf);
    }
    fn_8050168C(pOut, &pData->mProjMtx);
}

void fn_800754EC(nw4r::g3d::Camera* pSelf, void* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 665, lbl_8058E468);
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    u32 flags = pData->mFlags;
    if ((flags & 0x40) != 0) {
        fn_80050508(pOut);
        fn_804C6810(pData->mUnkBC, pData->mUnkC0, pData->mUnkC4, pData->mUnkC8, pData->mUnkCC,
                    -pData->mUnkD0, pData->mUnkD4, pData->mUnkD8);
        return;
    }
    if ((flags & 0x10) != 0) {
        fn_80050508(pOut);
        fn_804C6660(pData->mUnkBC, pData->mUnkC0, pData->mUnkC4, pData->mUnkC8, pData->mProjC,
                    pData->mUnkCC, -pData->mUnkD0, pData->mUnkD4, pData->mUnkD8);
        return;
    }
    fn_80050508(pOut);
    fn_804C6710(pData->mProjA, pData->mProjB, pData->mUnkCC, -pData->mUnkD0, pData->mUnkD4,
                pData->mUnkD8);
}

void fn_80075620(nw4r::g3d::Camera* pSelf, void* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 713, lbl_8058E468);
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    fn_800504D4(pOut);
    f32* pMtx = (f32*)pOut;
    pMtx[0] = pData->mUnkCC;
    pMtx[3] = pData->mUnkD4;
    pMtx[5] = -pData->mUnkD0;
    pMtx[7] = pData->mUnkD8;
    pMtx[10] = 0.0f;
    pMtx[11] = 1.0f;
}

void fn_800756DC(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 736, lbl_8058E468);
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    if (fn_80088584()->mUnk18 != 0) {
        fn_804BA7A0((pData->mFlags & 0x100) != 0, pData->mViewportX, pData->mViewportY,
                    pData->mViewportW, pData->mViewportH, pData->mViewportNear, pData->mViewportFar);
        return;
    }
    GXSetViewport(pData->mViewportX, pData->mViewportY, pData->mViewportW, pData->mViewportH,
                  pData->mViewportNear, pData->mViewportFar);
}

void fn_800757A8(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 773, lbl_8058E468);
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    if ((pData->mFlags & 0x80) == 0) {
        fn_80075DD8(pSelf);
    }
    fn_80075844(&pData->mProjMtx);
    GXSetProjection(&pData->mProjMtx[0][0], pData->mProjType);
}

void fn_80075844(void* pMtx) {
}

void fn_80075848(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 789, lbl_8058E468);
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    GXSetScissor(pData->mScissorX, pData->mScissorY, pData->mScissorW, pData->mScissorH);
}

void fn_800758C8(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 806, lbl_8058E468);
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    GXSetScissorBoxOffset(pData->mScissorOffsetX, pData->mScissorOffsetY);
}

#pragma peephole off

/* Rebuild the camera's view matrix from the posture the flags word selects: the +1 branch uses the
 * three-node path, +4 the position/target/up path and +2 the three-angle (Euler) path. */
void fn_80075940(nw4r::g3d::Camera* pSelf) {
    CameraData* pData = fn_80074A54(pSelf);
    u32 flags = pData->mFlags;
    nw4r::math::VEC3 diff;
    nw4r::math::VEC3 axisA;
    nw4r::math::VEC3 axisB;
    nw4r::math::VEC3 rowA;
    nw4r::math::VEC3 rowB;
    nw4r::math::VEC3 row0;
    nw4r::math::VEC3 row1;
    nw4r::math::VEC3 row2;
    f32 rotSin, rotCos;
    f32 sinX, sinY, sinZ, cosX, cosY, cosZ;
    if ((flags & 1) != 0) {
        if (fn_80074D38(&pData->mPosX, &pData->mUpX) == 0) {
            nw4r::db::Panic(lbl_8058E430, 832, lbl_8058E488, pData->mPosX, pData->mPosY,
                            pData->mPosZ);
        }
        s32 up = fn_800508A8(&pData->mUpX);
        s32 target = fn_800508A8(&pData->mTargetX);
        s32 pos = fn_800508A8(&pData->mPosX);
        fn_80050508(pData);
        fn_804C64E0(pos, target, up);
    } else if ((flags & 4) != 0) {
        fn_80041E8C(&diff, pData->mPosX - pData->mUpX, pData->mPosY - pData->mUpY,
                    pData->mPosZ - pData->mUpZ);
        if (0.0f == diff.x && 0.0f == diff.z) {
            pData->mViewMtx[0][0] = 1.0f;
            pData->mViewMtx[0][1] = 0.0f;
            pData->mViewMtx[0][2] = 0.0f;
            pData->mViewMtx[0][3] = -pData->mPosX;
            pData->mViewMtx[1][0] = 0.0f;
            pData->mViewMtx[1][1] = 0.0f;
            pData->mViewMtx[2][0] = 0.0f;
            pData->mViewMtx[2][2] = 0.0f;
            if (diff.y <= 0.0f) {
                pData->mViewMtx[1][2] = 1.0f;
                pData->mViewMtx[1][3] = -pData->mPosZ;
                pData->mViewMtx[2][1] = -1.0f;
                pData->mViewMtx[2][3] = pData->mPosY;
            } else {
                pData->mViewMtx[1][2] = -1.0f;
                pData->mViewMtx[1][3] = pData->mPosZ;
                pData->mViewMtx[2][1] = 1.0f;
                pData->mViewMtx[2][3] = -pData->mPosY;
            }
        } else {
            fn_80041E8C(&axisA, diff.z, 0.0f, -diff.x);
            fn_80043EA8(&axisB);
            fn_80050850(&diff, &diff);
            fn_80050850(&axisA, &axisA);
            fn_80051820(&axisB, &diff, &axisA);
            fn_80075DCC(&rotSin, &rotCos, pData->mUnkA4);
            fn_80043EA8(&rowA);
            fn_80043EA8(&rowB);
            rowA.x = rotSin * axisB.x + rotCos * axisA.x;
            rowA.y = rotSin * axisB.y;
            rowA.z = rotSin * axisB.z + rotCos * axisA.z;
            rowB.x = rotCos * axisB.x - rotSin * axisA.x;
            rowB.y = rotCos * axisB.y;
            rowB.z = rotCos * axisB.z - rotSin * axisA.z;
            pData->mViewMtx[0][0] = rowA.x;
            pData->mViewMtx[0][1] = rowA.y;
            pData->mViewMtx[0][2] = rowA.z;
            pData->mViewMtx[0][3] = -fn_80052214(&pData->mPosX, &rowA);
            pData->mViewMtx[1][0] = rowB.x;
            pData->mViewMtx[1][1] = rowB.y;
            pData->mViewMtx[1][2] = rowB.z;
            pData->mViewMtx[1][3] = -fn_80052214(&pData->mPosX, &rowB);
            pData->mViewMtx[2][0] = diff.x;
            pData->mViewMtx[2][1] = diff.y;
            pData->mViewMtx[2][2] = diff.z;
            pData->mViewMtx[2][3] = -fn_80052214(&pData->mPosX, &diff);
        }
    } else {
        if ((flags & 2) == 0) {
            nw4r::db::Panic(lbl_8058E430, 926, lbl_8058E4E0);
        }
        fn_80075DCC(&sinX, &cosX, pData->mUnk98);
        fn_80075DCC(&sinY, &cosY, pData->mUnk9C);
        fn_80075DCC(&sinZ, &cosZ, pData->mUnkA0);
        fn_80043EA8(&row0);
        fn_80043EA8(&row1);
        fn_80043EA8(&row2);
        row0.x = sinZ * (sinX * sinY) + cosX * cosZ;
        row0.y = cosY * sinZ;
        row0.z = sinZ * (sinX * cosX) - sinY * cosZ;
        row1.x = cosZ * (sinX * sinY) - cosX * sinZ;
        row1.y = cosY * cosZ;
        row1.z = cosZ * (sinX * cosX) + sinY * sinZ;
        row2.x = cosY * sinY;
        row2.y = -sinX;
        row2.z = cosY * cosX;
        pData->mViewMtx[0][0] = row0.x;
        pData->mViewMtx[0][1] = row0.y;
        pData->mViewMtx[0][2] = row0.z;
        pData->mViewMtx[0][3] = -fn_80052214(&pData->mPosX, &row0);
        pData->mViewMtx[1][0] = row1.x;
        pData->mViewMtx[1][1] = row1.y;
        pData->mViewMtx[1][2] = row1.z;
        pData->mViewMtx[1][3] = -fn_80052214(&pData->mPosX, &row1);
        pData->mViewMtx[2][0] = row2.x;
        pData->mViewMtx[2][1] = row2.y;
        pData->mViewMtx[2][2] = row2.z;
        pData->mViewMtx[2][3] = -fn_80052214(&pData->mPosX, &row2);
    }
    pData->mFlags |= 8;
}

#pragma peephole on

} /* extern "C" */
