/*
 * nw4r/ut_LinkList.h - nw4r::ut::detail's sentinel-node intrusive list, whose members `nw4r/ut_LinkList.cpp` defines.
 */
#ifndef NW4R_UT_LINKLIST_H
#define NW4R_UT_LINKLIST_H

#include "types.h"

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

}  // namespace ut
}  // namespace nw4r
#endif

#endif
