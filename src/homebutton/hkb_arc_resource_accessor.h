/*
 * homebutton/hkb_arc_resource_accessor.h - the keyboard layout engine's archive resource accessor
 *   (nw4r::lyt::MultiArcResourceAccessor): a list of attached ARC archives searched in order for a resource, and a
 *   list of registered fonts searched by name.  The layouts are read from homebutton/hkb_arc_resource_accessor.cpp.
 */
#ifndef HOMEBUTTON_HKB_ARC_RESOURCE_ACCESSOR_H
#define HOMEBUTTON_HKB_ARC_RESOURCE_ACCESSOR_H

#include "types.h"
#include "ARC/arc.h"
#include "homebutton/hkb_material.h"
#include "homebutton/hkb_resource_accessor.h"

namespace nw4r {
namespace lyt {

/* A font registered under a name. size: 0x8C */
class FontRefLink {
public:
    FontRefLink();
    void Set(const char* name, ut::Font* pFont);

    const char* GetFontName() const { return mFontName; }
    ut::Font* GetFont() const { return mpFont; }

    /* +0x00 */ ut::detail::LinkListNode mLink;
    /* +0x08 */ char mFontName[128];
    /* +0x88 */ ut::Font* mpFont;
};

/* An ARC archive attached under a resource root directory. size: 0xA4 */
class ArcResourceLink {
public:
    /* untyped: byte range - the archive image */
    bool Set(void* archiveStart, const char* resRootDirectory);

    ARCHandle* GetArcHandle() { return &mArcHandle; }
    const char* GetResRootDir() const { return mResRootDir; }

    /* +0x00 */ ut::detail::LinkListNode mLink;
    /* +0x08 */ ARCHandle mArcHandle;
    /* +0x24 */ char mResRootDir[128];
};

/* A resource accessor over several archives and registered fonts. size: 0x1C */
class MultiArcResourceAccessor : public ResourceAccessor {
public:
    MultiArcResourceAccessor();
    virtual ~MultiArcResourceAccessor();
    /* untyped: the resource bytes, typed by the caller */
    virtual void* GetResource(u32 resType, const char* name, u32* pSize);
    virtual ut::Font* GetFont(const char* name);

    void Attach(ArcResourceLink* pLink);
    void DetachAll() { mArcList.Clear(); }
    void RegistFont(FontRefLink* pLink);

    /* +0x04 */ ut::LinkList<ArcResourceLink, 0> mArcList;
    /* +0x10 */ ut::LinkList<FontRefLink, 0> mFontList;
};

}  // namespace lyt
}  // namespace nw4r

#endif /* HOMEBUTTON_HKB_ARC_RESOURCE_ACCESSOR_H */
