/*
 * homebutton/hkb_bounding.h - the keyboard layout engine's bounding pane (nw4r::lyt::Bounding), which
 *   homebutton/hkb_bounding.cpp defines: a pane that is never drawn and only marks an area.
 */
#ifndef HOMEBUTTON_HKB_BOUNDING_H
#define HOMEBUTTON_HKB_BOUNDING_H

#include "homebutton/hkb_layout.h"

namespace nw4r {
namespace lyt {

/* A pane that marks an area and draws nothing. size: 0xD8 */
class Bounding : public Pane {
public:
    Bounding(const res::Bounding* pBlock, const ResBlockSet& resBlockSet);
    virtual ~Bounding();
    virtual const ut::detail::RuntimeTypeInfo* GetRuntimeTypeInfo() const;
    virtual void DrawSelf(const DrawInfo& drawInfo);

    /* The class's run-time type record (0x80795A40); its parent is Pane's. */
    static const ut::detail::RuntimeTypeInfo typeInfo;
};

}  // namespace lyt
}  // namespace nw4r

#endif
