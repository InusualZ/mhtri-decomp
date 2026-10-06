/*
 * nw4r/fn_805012C4.h - declarations of the symbols owned by `nw4r/fn_805012C4.cpp` that other units call or read:
 *   nw4r::math's matrix helpers, ut's intrusive lists, the binary-file header check, the character-stream reader,
 *   the tag processor and the locked-cache wrappers.
 */
#ifndef NW4R_FN_805012C4_H
#define NW4R_FN_805012C4_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object, typed by the callers' views */
void  MEMInitList(void* list, u16 offset);   /* ut::List initialiser */

/* --------------------------------------------------------------------------------------------- */
/* Callees whose band holds no registered unit                                                    */
/* --------------------------------------------------------------------------------------------- */
/* untyped: opaque band object, typed by the callers' views */
void fn_80501A64(void* list, void* node);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80501BF4(void* list, void* node);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80501C60(void* list, void* node);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80501C9C(void* list, u16 n);            /* List_GetNth */

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `unsplit/unknown.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

void mtx34_rotate_vec3(nw4r::math::VEC3* out, const nw4r::math::MTX34* mtx, const nw4r::math::VEC3* v);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace nw4r {
namespace math {

/* A plane n.p + d = 0. size: 0x10 */
struct PLANE {
    f32 Test(const VEC3& p) const;

    /* +0x0 */ VEC3 N;
    /* +0xC */ f32 d;
};

/* An axis-aligned box. size: 0x18 */
struct AABB {
    void Set(const AABB* box, const MTX34* mtx);

    /* +0x00 */ VEC3 min;
    /* +0x0C */ VEC3 max;
};

/* A view frustum: its camera matrix, side planes, depth range, bounding box and the six planes the box tests
 * read. size: 0xF0 */
class Frustum {
public:
    int IntersectAABB_Ex(const AABB* box) const;

    /* +0x00 */ MTX34 cam;
    /* +0x30 */ u8 pad_0x30[0x48];
    /* +0x78 */ AABB box;
    /* +0x90 */ PLANE planes[6];
};

MTX33* MTX33Identity(MTX33* pOut);
MTX33* MTX34ToMTX33(MTX33* pOut, const MTX34* pM);
MTX34* MTX34Zero(MTX34* pOut);
MTX34* MTX34Scale(MTX34* pOut, const MTX34* pM, const VEC3* pS);
MTX34* MTX34Trans(MTX34* pOut, const MTX34* pM, const VEC3* pT);
MTX34* MTX34RotAxisFIdx(MTX34* pOut, const VEC3* pAxis, f32 fIdx);
MTX34* MTX34RotXYZFIdx(MTX34* pOut, f32 fx, f32 fy, f32 fz);
MTX34* MTX33ToMTX34(MTX34* pOut, const MTX33* pM);
MTX44* MTX44Identity(MTX44* pOut);
MTX44* MTX44Copy(MTX44* pOut, const MTX44* pM);

}  // namespace math

namespace ut {

/* The link record an element of an offset-based List carries. size: 0x8 */
struct Link {
    /* untyped: the neighbouring elements, whatever type the list holds */
    /* +0x0 */ void* prevObject;
    /* +0x4 */ void* nextObject;
};

/* An intrusive doubly linked list of elements whose Link sits `offset` bytes in. size: 0xC */
struct List {
    /* untyped: the end elements, whatever type the list holds */
    /* +0x0 */ void* headObject;
    /* +0x4 */ void* tailObject;
    /* +0x8 */ u16 numObjects;
    /* +0xA */ u16 offset;
};

/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...) */
void List_Init(List* list, u16 offset);
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void List_Append(List* list, void* object);
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void List_Insert(List* list, void* target, void* object);
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void List_Remove(List* list, void* object);
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void* List_GetNext(const List* list, const void* object);
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void* List_GetPrev(const List* list, const void* object);
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void* List_GetNth(const List* list, u16 index);

}  // namespace ut
}  // namespace nw4r
#endif

#ifdef __cplusplus
namespace nw4r {
namespace ut {

namespace detail {

/* One link of an intrusive doubly linked list. size: 0x8 */
class LinkListNode {
public:
    LinkListNode() : mNext(NULL), mPrev(NULL) {}

    /* +0x0 */ LinkListNode* mNext;
    /* +0x4 */ LinkListNode* mPrev;
};

/* A circular doubly linked list of LinkListNodes with a sentinel node. size: 0xC */
class LinkListImpl {
public:
    /* A position in the list: the node it points at. */
    class Iterator {
    public: /* size: 0x4 */
        Iterator(LinkListNode* node) : mPointer(node) {}

        /* +0x0 */ LinkListNode* mPointer;
    };

    ~LinkListImpl();

    Iterator GetBeginIter() { return Iterator(mNode.mNext); }
    Iterator GetEndIter() { return Iterator(&mNode); }

    LinkListNode* Erase(LinkListNode* node);
    Iterator Erase(Iterator it);
    Iterator Erase(Iterator first, Iterator last);
    void Clear();
    Iterator Insert(Iterator it, LinkListNode* node);

    /* +0x0 */ u32 mSize;
    /* +0x4 */ LinkListNode mNode;
};

}  // namespace detail

/* The common header of an nw4r binary resource file. size: 0x10 */
struct BinaryFileHeader {
    /* +0x0 */ u32 signature;
    /* +0x4 */ u16 byteOrder;
    /* +0x6 */ u16 version;
    /* +0x8 */ u32 fileSize;
    /* +0xC */ u16 headerSize;
    /* +0xE */ u16 dataBlocks;
};

/* The header of one data block inside a binary resource file. size: 0x8 */
struct BinaryBlockHeader {
    /* +0x0 */ u32 kind;
    /* +0x4 */ u32 size;
};

bool IsValidBinaryFile(const BinaryFileHeader* header, u32 signature, u16 version, u16 minBlocks);

/* Reads characters out of an encoded string one code point at a time. size: 0x8 */
class CharStrmReader {
public:
    typedef u16 (CharStrmReader::*ReadFunc)();

    CharStrmReader(ReadFunc func) : mCharStrm(NULL), mReadFunc(func) {}

    /* untyped: byte range - the encoded string, read as u8 or u16 by the encoding */
    void Set(const void* stream) {
        mCharStrm = stream;
    }
    /* untyped: byte range - the encoded string, read as u8 or u16 by the encoding */
    const void* GetCurrentPos() const {
        return mCharStrm;
    }
    u16 Next() {
        return (this->*mReadFunc)();
    }

    u16 ReadNextCharUTF8();
    u16 ReadNextCharUTF16();
    u16 ReadNextCharCP1252();
    u16 ReadNextCharSJIS();

    template <typename T> T GetChar(int offset) const {
        return reinterpret_cast<const T* const&>(mCharStrm)[offset];
    }

    template <typename T> void StepStrm(int count) {
        reinterpret_cast<const T*&>(mCharStrm) += count;
    }

    /* untyped: byte range - the encoded string, read as u8 or u16 by the encoding */
    /* +0x0 */ const void* mCharStrm;
    /* +0x4 */ ReadFunc mReadFunc;
};

/* An axis-aligned rectangle. size: 0x10 */
struct Rect {
    Rect() : left(0.0f), top(0.0f), right(0.0f), bottom(0.0f) {}
    Rect(f32 l, f32 t, f32 r, f32 b) : left(l), top(t), right(r), bottom(b) {}

    f32 GetWidth() const {
        return right - left;
    }

    /* Orders the edges so left <= right and top <= bottom. */
    void Normalize() {
        f32 l = left;
        f32 t = top;
        f32 r = right;
        f32 b = bottom;

        left = FSelect(r - l, l, r);
        right = FSelect(r - l, r, l);
        top = FSelect(b - t, t, b);
        bottom = FSelect(b - t, b, t);
    }

    /* `ifPos` when `cond` >= 0, else `ifNeg`, through `fsel`. */
    static f32 FSelect(register f32 cond, register f32 ifPos, register f32 ifNeg) {
        register f32 ret;
        asm { fsel ret, cond, ifPos, ifNeg }
        return ret;
    }

    /* +0x0 */ f32 left;
    /* +0x4 */ f32 top;
    /* +0x8 */ f32 right;
    /* +0xC */ f32 bottom;
};

template <typename T> struct PrintContext;
template <typename T> class TextWriterBase;

/* What a tag processor asks the writer to do after a control character. */
enum Operation {
    OPERATION_DEFAULT = 0,
    OPERATION_NO_CHAR_SPACE = 1,
    OPERATION_CHAR_SPACE = 2,
    OPERATION_NEXT_LINE = 3,
    OPERATION_END_DRAW = 4
};

/* Handles a text writer's control characters (line feed and tab). size: 0x4 */
template <typename T> class TagProcessorBase {
public:
    TagProcessorBase();
    virtual ~TagProcessorBase();
    virtual Operation Process(u16 code, PrintContext<T>* context);
    virtual Operation CalcRect(Rect* rect, u16 code, PrintContext<T>* context);

    /* Moves the cursor to the start of the next line. */
    void ProcessLinefeed(PrintContext<T>* context) {
        TextWriterBase<T>& writer = *context->writer;
        f32 x = context->xOrigin;
        f32 y = writer.GetCursorY() + writer.GetLineHeight();

        writer.SetCursorX(x);
        writer.SetCursorY(y);
    }

    /* Moves the cursor to the next tab stop (tab width in character widths). */
    void ProcessTab(PrintContext<T>* context) {
        TextWriterBase<T>& writer = *context->writer;
        int tabWidth = writer.GetTabWidth();

        if (tabWidth > 0) {
            f32 charWidth = writer.IsWidthFixed() ? writer.GetFixedWidth() : writer.GetFontWidth();
            f32 dx = writer.GetCursorX() - context->xOrigin;
            f32 tabPixel = tabWidth * charWidth;
            int numTab = static_cast<int>(dx / tabPixel) + 1;
            f32 x = context->xOrigin + tabPixel * numTab;

            writer.SetCursorX(x);
        }
    }

    /* +0x0 */ /* the vtable */
};

/* The locked (scratch) data cache, guarded by one mutex. */
namespace LC {

void Enable();
void Disable();
bool Lock();
void Unlock();
void LoadBlocks(void* dst, void* src, u32 blocks); /* untyped: byte range */
void LoadData(void* dst, void* src, u32 size);     /* untyped: byte range */
void StoreBlocks(void* dst, void* src, u32 blocks); /* untyped: byte range */
void StoreData(void* dst, void* src, u32 size);     /* untyped: byte range */

}  // namespace LC

}  // namespace ut
}  // namespace nw4r
#endif

#endif
