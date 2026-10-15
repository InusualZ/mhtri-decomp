/*
 * nw4r/ut_LinkList.cpp - nw4r::ut::detail::LinkListImpl, the sentinel-node intrusive list.
 * RANGE. .text 0x80501CE8-0x80501E24 (4 functions); no data.
 * RANGE. The edges with the neighbouring nw4r files (math_types .. ut_LockedCache, 0x805012C4-0x80502828) are the
 *   namespace/class runs in link order; each file pools its own `.sdata2` constants (0.0f once per file).
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. File from NW4R's ut_LinkList.cpp; the map carries the member manglings.
 */

#include "nw4r/ut_LinkList.h"

namespace nw4r {
namespace ut {
namespace detail {

/* Unlinks `node` and returns the node after it. */
inline LinkListNode* LinkListImpl::Erase(LinkListNode* node) {
    LinkListNode* next = node->mNext;
    LinkListNode* prev = node->mPrev;

    next->mPrev = prev;
    prev->mNext = next;
    mSize--;
    node->mNext = NULL;
    node->mPrev = NULL;
    return next;
}

/* Unlinks every node from `first` up to `last`, returning `last`. */
inline LinkListImpl::Iterator LinkListImpl::Erase(Iterator first, Iterator last) {
    LinkListNode* node = first.mPointer;
    LinkListNode* end = last.mPointer;
    LinkListNode* next;

    while (node != end) {
        next = node->mNext;
        Erase(node);
        node = next;
    }
    return last;
}

/* 0x80501CE8 (0x84): unlinks every node, then frees the list when asked to. */
LinkListImpl::~LinkListImpl() {
    Clear();
}

/* 0x80501D6C (0x48): unlinks the node at `it` and returns the position after it. */
LinkListImpl::Iterator LinkListImpl::Erase(Iterator it) {
    Iterator next = Iterator(it.mPointer->mNext);

    return Erase(it, next);
}

/* 0x80501DB4 (0x44): unlinks every node. */
void LinkListImpl::Clear() {
    Erase(GetBeginIter(), GetEndIter());
}

/* 0x80501DF8 (0x2C): links `node` in front of `it` and returns its position. */
LinkListImpl::Iterator LinkListImpl::Insert(Iterator it, LinkListNode* node) {
    LinkListNode* next = it.mPointer;
    LinkListNode* prev = next->mPrev;

    node->mNext = next;
    node->mPrev = prev;
    next->mPrev = node;
    prev->mNext = node;
    mSize++;
    return Iterator(node);
}

}  // namespace detail

}  // namespace ut
}  // namespace nw4r
