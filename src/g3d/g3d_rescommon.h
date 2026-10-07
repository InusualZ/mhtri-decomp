/* g3d/g3d_rescommon.h - nw4r g3d's `ResCommon<T>` handle base and the accessor set every resource class carries.
 *   The library calls these accessors out of line (a copy per TU, the linker keeps the first), so they are
 *   declared here and each one is defined, non-inline, in the unit whose copy the linked image keeps. */
#ifndef MHTRI_G3D_G3D_RESCOMMON_H
#define MHTRI_G3D_G3D_RESCOMMON_H

#include "types.h"
#include "nw4r/db_assert.h"

namespace nw4r {
namespace g3d {

/* The one-word handle every resource class derives from: the address of its block, or NULL.  The constructor
 * is declared only; the unit that keeps an instantiation's copy defines it with NW4R_G3D_RESCOMMON_CTOR.
 * size: 0x4 */
template <typename T>
struct ResCommon {
    /* +0x0 */ T* mpData;

    /* untyped: opaque handle */
    explicit ResCommon(void* pData);

    /* Resolves a block-relative offset to a sub-resource handle, NULL when the offset is 0. */
    template <typename U>
    U ofs_to_obj(s32 ofs) const;
    /* Resolves a block-relative offset to an address, NULL when the offset is 0. */
    template <typename U>
    U* ofs_to_ptr(s32 ofs);

    /* Copies the handle word (the library's assignment, emitted out of line). */
    ResCommon& operator=(const ResCommon& rhs);

    /* Leaves the handle unset, for the copy constructors that copy it through an out-of-line copy. */
    ResCommon() {}
};

template <typename T>
ResCommon<T>& ResCommon<T>::operator=(const ResCommon<T>& rhs) {
    mpData = rhs.mpData;
    return *this;
}

template <typename T>
template <typename U>
U ResCommon<T>::ofs_to_obj(s32 ofs) const {
    if (ofs != 0) {
        return U(reinterpret_cast<u8*>(mpData) + ofs);
    }
    return U(NULL);
}

template <typename T>
template <typename U>
U* ResCommon<T>::ofs_to_ptr(s32 ofs) {
    u8* p = reinterpret_cast<u8*>(mpData);
    if (ofs != 0) {
        return reinterpret_cast<U*>(p + ofs);
    }
    return NULL;
}

/* A resource name: its length word, then the characters.  size: 0x4 (a lower bound) */
struct ResNameData {
    /* +0x0 */ u32 len;
    /* +0x4 */ char str[4];
};

/* A one-word handle on a resource name.  size: 0x4 */
class ResName : public ResCommon<ResNameData> {
public:
    /* untyped: opaque handle */
    explicit ResName(void* pData);
    bool IsValid() const;
};

/* One node of a resource dictionary's patricia tree.  size: 0x10 */
struct ResDicEntry {
    /* +0x0 */ u16 ref;
    /* +0x2 */ u16 flag;
    /* +0x4 */ u16 idxLeft;
    /* +0x6 */ u16 idxRight;
    /* +0x8 */ s32 ofsString;
    /* +0xC */ s32 ofsData;
};

/* A resource dictionary (the patricia tree of named sub-resources): entry 0 is the root, entries 1..numData the
 * named sub-resources.  size: 0x18 (a lower bound: the entry array runs on) */
struct ResDicData {
    /* +0x0 */ u32 size;
    /* +0x4 */ u32 numData;
    /* +0x8 */ ResDicEntry entry[1];
};

/* A one-word handle on a resource dictionary.  size: 0x4 */
class ResDic : public ResCommon<ResDicData> {
public:
    /* untyped: opaque handle */
    explicit ResDic(void* pData);
    void* operator[](const char* pName) const; /* untyped: opaque handle */
    void* operator[](const ResName name) const; /* untyped: opaque handle */
    void* operator[](int idx) const; /* untyped: opaque handle */
    u32 GetNumData() const;
    int GetIndex(const ResName name) const;
    const ResDicData& ref() const;
};

}  // namespace g3d
}  // namespace nw4r

/* Defines `ResCommon<D>`'s constructor (a single store) for the block type `D`. */
#define NW4R_G3D_RESCOMMON_CTOR(D) \
    template <> nw4r::g3d::ResCommon<D>::ResCommon(void* pData) : mpData(static_cast<D*>(pData)) {}

/* Declares the accessors of resource class `T`, whose block type is `T##Data`. */
#define NW4R_G3D_RESOURCE_FUNC_DECL(T)        \
    explicit T(void* pData);                  \
    static const char* GetClassName();        \
    bool IsValid() const;                     \
    T##Data* ptr();                           \
    const T##Data* ptr() const;               \
    T##Data& ref();                           \
    const T##Data& ref() const;

/* The bodies, one macro per accessor, for the unit that keeps the copy.  `file` and `line` are the original
 * accessor header's, which the asserts pass to `Panic`. */
#define NW4R_G3D_RESOURCE_CTOR(T) \
    T::T(void* pData) : nw4r::g3d::ResCommon<T##Data>(pData) {}
#define NW4R_G3D_RESOURCE_CTOR_ALIGNED(T, file, line)                                               \
    T::T(void* pData) : nw4r::g3d::ResCommon<T##Data>(pData) {                                      \
        if ((u32)pData & 0x3) nw4r::db::Panic(file, line, "NW4R:Failed assertion !((u32)p & 0x3)"); \
    }
#define NW4R_G3D_RESOURCE_CLASS_NAME(T) \
    const char* T::GetClassName() { return #T; }
#define NW4R_G3D_RESOURCE_IS_VALID(T) \
    bool T::IsValid() const { return mpData != NULL; }
#define NW4R_G3D_RESOURCE_PTR(T) \
    T##Data* T::ptr() { return mpData; }
#define NW4R_G3D_RESOURCE_PTR_CONST(T) \
    const T##Data* T::ptr() const { return mpData; }
#define NW4R_G3D_RESOURCE_REF(T, file, line)                                                         \
    T##Data& T::ref() {                                                                              \
        if (!IsValid()) nw4r::db::Panic(file, line, "%s::%s: Object not valid.", GetClassName(), "ref"); \
        return *ptr();                                                                               \
    }
#define NW4R_G3D_RESOURCE_REF_CONST(T, file, line)                                                   \
    const T##Data& T::ref() const {                                                                  \
        if (!IsValid()) nw4r::db::Panic(file, line, "%s::%s: Object not valid.", GetClassName(), "ref"); \
        return *ptr();                                                                               \
    }

#endif /* MHTRI_G3D_G3D_RESCOMMON_H */
