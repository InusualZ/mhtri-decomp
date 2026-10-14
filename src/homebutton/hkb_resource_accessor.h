/*
 * homebutton/hkb_resource_accessor.h - the keyboard layout engine's resource accessor (nw4r::lyt::ResourceAccessor):
 *   the abstract source of named resources and fonts a layout and its materials load from.
 *   The class table is read from .data 0x8064A920 (destructor, a pure GetResource slot, GetFont).
 */
#ifndef HOMEBUTTON_HKB_RESOURCE_ACCESSOR_H
#define HOMEBUTTON_HKB_RESOURCE_ACCESSOR_H

#include "types.h"

namespace nw4r {
namespace ut {
class Font;
}  // namespace ut

namespace lyt {

/* A source of named resources (the layout's archive accessor). size: 0x4 */
class ResourceAccessor {
public:
    ResourceAccessor();
    virtual ~ResourceAccessor();
    /* untyped: the resource bytes, typed by the caller */
    virtual void* GetResource(u32 resType, const char* name, u32* pSize) = 0;
    virtual ut::Font* GetFont(const char* name);
};

}  // namespace lyt
}  // namespace nw4r

#endif /* HOMEBUTTON_HKB_RESOURCE_ACCESSOR_H */
