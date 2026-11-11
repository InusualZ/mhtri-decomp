/* homebutton/hkb_group.cpp - the software keyboard layout engine's groups (nw4r::lyt::Group, GroupContainer): a named
 *   set of panes the layout binds animations to.
 * RANGE. .text 0x8052B004-0x8052B344 (5 functions); no data of its own.
 *   Edges: the left edge follows homebutton/hkb_pane.cpp's static initialiser 0x8052AFF8; the right edge is
 *   homebutton/hkb_layout.cpp's first function (the SetTagProcessorImpl helper) at 0x8052B344. The three units
 *   homebutton/hkb_group.cpp, hkb_layout.cpp and hkb_picture.cpp were one registered range; the seams are proven by
 *   the class tables (.data rises Group 0x8064A5E8, Layout 0x8064A5F8, Picture 0x8064A638, where one object emits its
 *   tables in descending order), the `.sdata2` pool (0.0 at 0x8079D570 for the layout, again at 0x8079D578 for the
 *   picture), the `.sbss` run-time type records, and the static initialiser (0x8052C874, which sets
 *   Picture::typeInfo).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: the file name and nw4r::lyt::Group / GroupContainer members, from the runtime dump
 *   (nw4hbm::lyt::GroupContainer::AppendGroup / FindGroupByName at their addresses: the HOME Button copy of
 *   NintendoWare's layout library, spelt nw4r::lyt in this band after homebutton/hkb_material.cpp).
 * SHAPES. the list destructors copy the iterator with `it++` (a copy then `++it` keeps a second register); the
 *   GroupContainer destructor hands DeleteObj LinkList::GetPointerFromNode's pointer (see homebutton/hkb_pane.cpp).
 * SHAPES. the Group constructor keeps LinkListNode's two zero stores before the list initialisation because
 *   LinkListNode derives from ut::NonCopyable (src/nw4r/ut_LinkList.h).
 * RESIDUALS. our object emits weak LinkList<...>, LinkListNode and ut::NonCopyable destructors the target lacks
 *   (unreferenced, dead-stripped by the link).
 */
#include "homebutton/hkb_layout.h"

#include "MSL_C/string.h"

namespace nw4r {
namespace lyt {

typedef ut::LinkList<PaneLink, 0> PaneLinkList;
typedef ut::LinkList<Group, 4> GroupList;

/* 0x8052B004 (0x11C): Builds a group from its `grp1` block: its name, then a link to each named pane found under
 * the root pane. */
Group::Group(const res::Group* pResGroup, Pane* pRootPane) {
    mbUserAllocated = false;

    strncpy(mName, pResGroup->name, sizeof(mName) - 1);
    mName[sizeof(mName) - 1] = '\0';

    const char* paneNameBase = detail::ConvertOffsToPtr<char>(pResGroup, sizeof(res::Group));
    for (int i = 0; i < pResGroup->paneNum; ++i) {
        Pane* pFindPane = pRootPane->FindPaneByName(paneNameBase + i * 16, true);
        if (pFindPane) {
            AppendPane(pFindPane);
        }
    }
}

/* Links a pane into the group. */
inline void Group::AppendPane(Pane* pPane) {
    PaneLink* pPaneLink = Layout::NewObj<PaneLink>();
    if (pPaneLink) {
        pPaneLink->mTarget = pPane;
        mPaneLinkList.PushBack(pPaneLink);
    }
}

/* 0x8052B120 (0xAC): Unlinks and frees the group's pane links. */
Group::~Group() {
    for (PaneLinkList::Iterator it = mPaneLinkList.GetBeginIter(); it != mPaneLinkList.GetEndIter();) {
        PaneLinkList::Iterator currIt = it++;
        mPaneLinkList.Erase(currIt.mIterator);
        Layout::DeleteObj(currIt.operator->());
    }
}

/* 0x8052B1CC (0xC8): Unlinks the groups and destroys the ones the container allocated. */
GroupContainer::~GroupContainer() {
    for (GroupList::Iterator it = mGroupList.GetBeginIter(); it != mGroupList.GetEndIter();) {
        GroupList::Iterator currIt = it++;
        mGroupList.Erase(currIt.mIterator);
        if (!currIt->IsUserAllocated()) {
            Layout::DeleteObj(GroupList::GetPointerFromNode(currIt.mIterator.mPointer));
        }
    }
}

/* 0x8052B294 (0x34): Adds a group at the end of the container. */
void GroupContainer::AppendGroup(Group* pGroup) {
    mGroupList.PushBack(pGroup);
}

/* 0x8052B2C8 (0x7C): Returns the group named `findName`, or NULL. */
Group* GroupContainer::FindGroupByName(const char* findName) {
    for (GroupList::Iterator it = mGroupList.GetBeginIter(); it != mGroupList.GetEndIter(); ++it) {
        if (detail::EqualsResName(it->GetName(), findName)) {
            return it.operator->();
        }
    }
    return NULL;
}

}  // namespace lyt
}  // namespace nw4r
