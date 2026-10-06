/* g3d/g3d_obj.h - nw4r g3d's object base classes: `G3dObj` (the parent/heap pair every scene and animation object
 *   carries, with the run-time type name the asserts print) and `AnmObj` (the animation object interface).
 *   Evidence: the vtables of `g3d/g3d_anmchr.cpp` (.data 0x8058BCA8..0x8058BE40): every class there opens with
 *   IsDerivedFrom, a pure G3dProc, the destructor, GetTypeObj and GetTypeName, and the AnmObj vtable at 0x8058BE08
 *   adds seven pure slots (SetFrame, GetFrame, UpdateFrame, SetUpdateRate, GetUpdateRate, Bind, Release); the
 *   destructor chain ends in 0x8007B2D4 (`g3d/fn_80075DCC.cpp`'s range) and an empty operator delete
 *   (0x8005D3E0).  The member names follow nw4r's; the classes are GUESSES until their units define them.
 *   Every member is declared out of line: retail calls them (the G3dObj constructor 0x8005D428 stores the vtable at
 *   0x8058ED18, then the parent at +0x04 and the heap at +0x08, and asserts the heap; the AnmObj constructor
 *   0x8005D3E4 calls it, stores its vtable and clears mFlags; TypeObj's constructor and GetTypeName are the
 *   out-of-line 0x8005DC60/0x8005DCD0 and 0x8005DC24).
 */
#ifndef MHTRI_G3D_G3D_OBJ_H
#define MHTRI_G3D_G3D_OBJ_H

#include "types.h"
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl */

struct MEMAllocator;

namespace nw4r {
namespace g3d {

/* size: 0xC */
class G3dObj {
public:
    /* The run-time type of an object: the address of its length-prefixed name.  size: 0x4 */
    class TypeObj {
    public:
        /* The length-prefixed type name a TypeObj points at; the name runs past str[4]. */
        struct TypeName {
            /* +0x0 */ u32 len;
            /* +0x4 */ char str[4];
        }; /* size: 0x8 (approximation) */

        explicit TypeObj(const TypeName* pName);
        u32 GetTypeID() const;
        const char* GetTypeName() const;
        bool operator==(const TypeObj& rhs) const;

        /* +0x0 */ const TypeName* mName;
    }; /* size: 0x4 */

    /* The notifications G3dProc dispatches. */
    enum G3dProcTask {
        G3DPROC_NONE = 0,
        G3DPROC_CALC_WORLD = 1,
        G3DPROC_CALC_MAT = 2,
        G3DPROC_CALC_VTX = 3,
        G3DPROC_CALC_VIEW = 4,
        G3DPROC_GATHER_SCNOBJ = 5,
        G3DPROC_DRAW_OPA = 6,
        G3DPROC_DRAW_XLU = 7,
        G3DPROC_UPDATEFRAME = 8,
        G3DPROC_CHILD_DETACHED = 0x10001,
        G3DPROC_ATTACH_PARENT = 0x10002,
        G3DPROC_DETACH_PARENT = 0x10003,
        G3DPROC_ZSORT = 0x10004
    };

    G3dObj(MEMAllocator* pHeap, G3dObj* pParent);

    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo) = 0; /* untyped: caller-owned payload */
    virtual ~G3dObj();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    void Destroy();
    G3dObj* GetParent() const;
    void SetParent(G3dObj* pParent);
    static const TypeObj GetTypeObjStatic();
    static void operator delete(void* pBlock); /* untyped: byte range */
    /* untyped: byte range - the placement address the Construct functions build the object in */
    static void* operator new(unsigned long size, void* pBlock);
    static void Dealloc(MEMAllocator* pHeap, void* pBlock); /* untyped: byte range */
    static void* Alloc(MEMAllocator* pHeap, u32 size); /* untyped: byte range */

    /* +0x00 vtable */
    /* +0x04 */ G3dObj* mpParent;
    /* +0x08 */ MEMAllocator* mpHeap;
};

/* size: 0x10 */
class AnmObj : public G3dObj {
public:
    /* The flag bits of mFlags. */
    enum AnmFlag {
        ANMFLAG_ISBOUND = (1 << 2)
    };

    AnmObj(MEMAllocator* pHeap, G3dObj* pParent);

    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo) = 0; /* untyped: caller-owned payload */
    virtual ~AnmObj();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    virtual void SetFrame(f32 frame) = 0;
    virtual f32 GetFrame() const = 0;
    virtual void UpdateFrame() = 0;
    virtual void SetUpdateRate(f32 rate) = 0;
    virtual f32 GetUpdateRate() const = 0;
    virtual bool Bind(ResMdl mdl) = 0;
    virtual void Release() = 0;

    void SetAnmFlag(AnmFlag flag, bool on);
    bool TestAnmFlag(AnmFlag flag) const;
    bool IsBound() const;
    static const TypeObj GetTypeObjStatic();

    /* +0x0C */ u32 mFlags;
};

/* The scene animation interface (cameras, lights, fog); only its constructor and the head of its vtable are
 * reconstructed.  size: 0x0C (approximation: the base's size, the derived scene animations add their members) */
class AnmScn : public G3dObj {
public:
    explicit AnmScn(MEMAllocator* pHeap);

    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo) = 0; /* untyped: caller-owned payload */
    virtual ~AnmScn();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;
};

/* The visibility, material-colour and texture-pattern animation interfaces; only their run-time type is reconstructed
 * (ScnMdlSimple::SetAnmObj casts to them).  size: 0x10 each (approximation: AnmObj's size, the derived classes add
 * their members) */
class AnmObjVis : public AnmObj {
public:
    static const TypeObj GetTypeObjStatic();
};

class AnmObjMatClr : public AnmObj {
public:
    static const TypeObj GetTypeObjStatic();
};

class AnmObjTexPat : public AnmObj {
public:
    static const TypeObj GetTypeObjStatic();
};

/* The checked cast of the run-time type: `pObj` as a `TTo`, or NULL when it is not one. */
template <typename TTo, typename TFrom>
TTo* DynamicCast(TFrom* pObj)
{
    bool derived = false;
    if (pObj != NULL) {
        if (pObj->IsDerivedFrom(TTo::GetTypeObjStatic())) {
            derived = true;
        }
    }
    if (derived) {
        return static_cast<TTo*>(pObj);
    }
    return NULL;
}

/* How a frame past the end is folded back (PlayPolicy_Onetime, PlayPolicy_Loop): (start, end, frame) -> frame. */
typedef f32 (*PlayPolicyFunc)(f32 startFrame, f32 endFrame, f32 frame);

/* An animation's frame counter: the current frame, the rate it advances by, the frame range and the play
 * policy.  size: 0x14 */
class FrameCtrl {
public:
    FrameCtrl(f32 startFrame, f32 endFrame, PlayPolicyFunc pPolicy);

    /* +0x00 */ f32 mFrame;
    /* +0x04 */ f32 mUpdateRate;
    /* +0x08 */ f32 mStartFrame;
    /* +0x0C */ f32 mEndFrame;
    /* +0x10 */ PlayPolicyFunc mpPlayPolicy;
};

}  // namespace g3d
}  // namespace nw4r

#endif /* MHTRI_G3D_G3D_OBJ_H */
