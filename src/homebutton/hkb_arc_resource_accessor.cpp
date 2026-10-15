/* homebutton/hkb_arc_resource_accessor.cpp - the keyboard layout engine's archive resource accessor
 *   (nw4r::lyt::MultiArcResourceAccessor): the recursive name lookup over an ARC directory tree, the per-archive
 *   resource fetch, the font and archive links, and the accessor's constructor, destructor, Attach, GetResource,
 *   RegistFont and GetFont.
 * RANGE. .text 0x80534DC0-0x805352DC (11 functions, in this order: FindNameResource, GetResourceSub, the
 *   FontRefLink constructor and Set, ArcResourceLink::Set, the accessor's constructor, destructor, Attach,
 *   GetResource, RegistFont, GetFont); .data 0x8064A938-0x8064A950 (the class table); .sdata 0x807944B8-0x807944C0
 *   ("." and "..").
 *   Edges: 0x80534DC0 is called only by 0x80534E8C, which is called only by the table slot 0x805351C0; the class
 *   table .data 0x8064A938 holds 0x805350FC, 0x805351C0 and 0x8053526C and is stored by the constructor 0x8053508C,
 *   which calls the base constructor 0x80534DA8.  The next `.data` object (0x8064A950) is a pointer table read from
 *   0x80536F08 on (a table followed by data, the V->S seam), so the archive class ends the TU; the cut sits after
 *   its last method 0x8053526C, and nothing in 0x805352DC..0x80535454 reads data that places it more exactly.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: nw4r::lyt::MultiArcResourceAccessor, ArcResourceLink and FontRefLink for the classes (the layout
 *   library's multi-archive accessor: an archive list at +0x04 searched by GetResource, a font list at +0x10
 *   searched by GetFont with strcmp, 0x8C font links and 0xA4 archive links); FindNameResource and GetResourceSub
 *   for the two file-local helpers; every row takes the compiler's mangling.
 * RESIDUALS. MultiArcResourceAccessor constructor 0x8053508C: retail keeps the two LinkListNode NULL stores of each
 *   list (+0x08/+0x0C, +0x14/+0x18) before the list init; here they fold away, as in homebutton/hkb_material.cpp's
 *   AnimationLinkList constructor (a LinkListImpl constructor calling an inline Initialize_ measured the same; a
 *   LinkListNode deriving from ut::NonCopyable makes the row 100, request hkb-lyt#1).
 *   The object also emits weak copies of the two LinkList destructors (0x58 B each) that no retail object carries:
 *   nothing references them, so the link dead-strips them.
 * SHAPES. the type string is filled with plain `resType >> n` stores (a (u8) cast adds an rlwinm mask retail lacks).
 */
#include "homebutton/hkb_arc_resource_accessor.h"

#include "MSL_C/extras.h"
#include "MSL_C/string.h"

namespace nw4r {
namespace lyt {

/* 0x80534DC0 (0xCC): Returns the entry number of the file `resName` anywhere below the current directory, or -1. */
static s32 FindNameResource(ARCHandle* pArcHandle, const char* resName) {
    s32 entryNum = -1;
    ARCDir dir;
    ARCDirEntry dirEntry;

    ARCOpenDir(pArcHandle, ".", &dir);
    while (ARCReadDir(&dir, &dirEntry)) {
        if (dirEntry.isDir) {
            ARCChangeDir(pArcHandle, dirEntry.name);
            entryNum = FindNameResource(pArcHandle, resName);
            ARCChangeDir(pArcHandle, "..");
            if (entryNum != -1) {
                break;
            }
        } else if (stricmp(resName, dirEntry.name) == 0) {
            entryNum = dirEntry.entryNum;
            break;
        }
    }
    ARCCloseDir(&dir);

    return entryNum;
}

/* 0x80534E8C (0x140): Returns the start of resource `name` of type `resType` under `resRootDir`, or NULL;
 * stores its length through `pSize`. */
/* untyped: the resource bytes, typed by the caller */
static void* GetResourceSub(ARCHandle* pArcHandle, const char* resRootDir, u32 resType, const char* name,
                            u32* pSize) {
    s32 entryNum = -1;

    if (ARCConvertPathToEntrynum(pArcHandle, resRootDir) != -1 && ARCChangeDir(pArcHandle, resRootDir)) {
        if (resType == 0) {
            entryNum = FindNameResource(pArcHandle, name);
        } else {
            char resTypeStr[5];
            resTypeStr[0] = resType >> 24;
            resTypeStr[1] = resType >> 16;
            resTypeStr[2] = resType >> 8;
            resTypeStr[3] = resType >> 0;
            resTypeStr[4] = '\0';

            if (ARCConvertPathToEntrynum(pArcHandle, resTypeStr) != -1 && ARCChangeDir(pArcHandle, resTypeStr)) {
                entryNum = ARCConvertPathToEntrynum(pArcHandle, name);
                ARCChangeDir(pArcHandle, "..");
            }
        }
        ARCChangeDir(pArcHandle, "..");
    }

    if (entryNum != -1) {
        ARCFileInfo arcFileInfo;
        ARCFastOpen(pArcHandle, entryNum, &arcFileInfo);
        void* resPtr = ARCGetStartAddrInMem(&arcFileInfo);
        if (pSize) {
            *pSize = ARCGetLength(&arcFileInfo);
        }
        ARCClose(&arcFileInfo);
        return resPtr;
    }

    return NULL;
}

/* 0x80534FCC (0x14): Constructs an unlinked font link with no font. */
FontRefLink::FontRefLink() : mpFont(NULL) {}

/* 0x80534FE0 (0x40): Names the link and stores its font. */
void FontRefLink::Set(const char* name, ut::Font* pFont) {
    strcpy(mFontName, name);
    mpFont = pFont;
}

/* 0x80535020 (0x6C): Opens the archive image and records its resource root directory; false if it is no archive. */
/* untyped: byte range - the archive image */
bool ArcResourceLink::Set(void* archiveStart, const char* resRootDirectory) {
    BOOL bSuccess = ARCInitHandle(archiveStart, &mArcHandle);
    if (!bSuccess) {
        return false;
    }

    strncpy(mResRootDir, resRootDirectory, sizeof(mResRootDir) - 1);
    mResRootDir[sizeof(mResRootDir) - 1] = '\0';
    return true;
}

/* 0x8053508C (0x70): Constructs an accessor with no archives and no fonts. */
MultiArcResourceAccessor::MultiArcResourceAccessor() {}

/* 0x805350FC (0x90): Detaches every archive and destroys the accessor. */
MultiArcResourceAccessor::~MultiArcResourceAccessor() {
    DetachAll();
}

/* 0x8053518C (0x34): Appends an archive to the search list. */
void MultiArcResourceAccessor::Attach(ArcResourceLink* pLink) {
    mArcList.PushBack(pLink);
}

/* 0x805351C0 (0x78): Returns the first attached archive's resource `name` of type `resType`, or NULL. */
/* untyped: the resource bytes, typed by the caller */
void* MultiArcResourceAccessor::GetResource(u32 resType, const char* name, u32* pSize) {
    ut::LinkList<ArcResourceLink, 0>::Iterator it = mArcList.GetBeginIter();
    for (; it != mArcList.GetEndIter(); ++it) {
        if (void* resPtr = GetResourceSub(it->GetArcHandle(), it->GetResRootDir(), resType, name, pSize)) {
            return resPtr;
        }
    }

    return NULL;
}

/* 0x80535238 (0x34): Appends a font link to the font list. */
void MultiArcResourceAccessor::RegistFont(FontRefLink* pLink) {
    mFontList.PushBack(pLink);
}

/* 0x8053526C (0x70): Returns the registered font named `name`, or NULL. */
ut::Font* MultiArcResourceAccessor::GetFont(const char* name) {
    ut::LinkList<FontRefLink, 0>::Iterator it = mFontList.GetBeginIter();
    for (; it != mFontList.GetEndIter(); ++it) {
        if (strcmp(name, it->GetFontName()) == 0) {
            return it->GetFont();
        }
    }

    return NULL;
}

}  // namespace lyt
}  // namespace nw4r
