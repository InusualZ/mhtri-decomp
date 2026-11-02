/*
 * g3d/g3d_restev.cpp - nw4r g3d `ResTev` resource: the checked handle, the stage count, the display-list call,
 *   the 0x200-byte copy and the data-cache store (GUESS: the file name is the `g3d_restev_ac.h` assert's
 *   stem and nw4r's own g3d_restev.cpp; the dump answers `zz_` placeholders for every row).
 * RANGE. .text 0x8009A174-0x8009A360 (6 functions); extab 0x80009968-0x80009990, extabindex
 *   0x800229F8-0x80022A34, .rodata 0x8056F730-0x8056F770, .data 0x80591800-0x80591830, .sdata
 *   0x807912C8-0x807912D0.  Cut from `g3d/g3d_resshp.cpp` (seam request flip-resshp#1): the extabindex
 *   record at 0x800229F8 names 0x8009A174 (extab 0x80009968) and the record at 0x80022A34 names
 *   0x8009A360 (extab 0x80009990); the `.data` strings start 8-aligned at 0x80591800 after resshp's
 *   last string (0x805917FF) and the `.sdata` "ref" static sits in its own 8-aligned fragment at
 *   0x807912C8; the `.rodata` size table is referenced only by `ResTev::CallDisplayList`.
 * NAMES. res_tev_ref is a GUESS for the non-const `ResTev::ref` instance (request flip-resshp#2: the shared
 *   header declares only the const one, and adding the overload re-binds `g3d/g3d_state`'s call sites, so
 *   the row keeps a C name).  res_tev_ptr is a GUESS (the raw `ptr()` word res_tev_ref returns);
 *   res_tev_get_num_stages is a GUESS (the +0x0C byte, 1..16, that indexes the 16-entry display-list size
 *   table: nw4r's ResTev::GetNumStages).  res_tev_copy_to and res_tev_dc_store are GUESSES (nw4r's
 *   ResTev::CopyTo/DCStore, by the asserts and the ScnMdl buffer refill).  The `ResTevData` fields are
 *   GUESSes (the offsets are the facts).
 * RESIDUALS. The one-call stub res_tev_ref_place is 4 B of `.text` the linker dead-strips.
 * SHAPES. File-scope `#pragma peephole off` and `#pragma pool_data off` as in `g3d/g3d_resshp.cpp` (the
 *   TU's own strings are literals).  `ResTev::ref` is a `__declspec(noinline) inline` weak copy placed
 *   after the stub that references it (MWCC emits the copy after the first referencing function and
 *   creates its string literals at the definition).  ResTev::CallDisplayList indexes the size table
 *   with the stage count read inline in the call's argument.  res_tev_copy_to's rebased size is a cast
 *   sum, `size = (u32)(size - delta)`, so the result lands in r0.
 */
#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/res_common.h"
#include "g3d/g3d_cpu.h"
#include "g3d/g3d_resvtx.h"      /* nw4r::g3d::DC (rule 2) */
#include "g3d/g3d_resshp.h"
#include "g3d/g3d_restev.h"
#include "RVLGX/GXPixel.h"        /* GXCallDisplayList (rule 2) */
#include "OS/PPCArch.h"           /* PPCSync (rule 2) */

#pragma pool_data off

/* The target's `-O3` schedule is retail only with the peephole pass off. */
#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic(const char*, int, const char*, ...) - the owner's real C++ declaration, called
 * through its signature, never the mangled spelling (docs/plan.md 6.5 rule 9). */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The `ResTev` resource block the unit caches (`g3d_restev_ac.h`, the file name of its assert).  Only the
 * words the bodies reach are named; the names are the best the offsets support (the offsets are the facts). */
struct ResTevData {
    /* +0x00 */ u32 mSize;     /* the block's size, the DC range res_tev_dc_store stores/flushes */
    /* +0x04 */ u32 mDlSize;   /* the display-list size res_tev_copy_to rebases by the copy     */
    /* +0x08 */ u8 pad_0x08[0x4];
    /* +0x0C */ u8 mNumStages; /* the tev stage count `ResTev::CallDisplayList` indexes the size table by */
    /* +0x0D */ u8 pad_0x0D[0x13];
    /* +0x20 */ u8 mDl[];      /* the embedded display list `ResTev::CallDisplayList` calls at +0x20 */
}; /* size: 0x20+ (a lower bound: only the words above are reached) */

/* The tev display-list sizes `ResTev::CallDisplayList` indexes by the block's stage count less one
 * (`.rodata`, 16 words). */
static const u32 tev_display_list_size[16] = {
    0xA0, 0xA0, 0xC0, 0xC0, 0x100, 0x100, 0x120, 0x120, 0x160, 0x160, 0x180, 0x180, 0x1C0, 0x1C0, 0x1E0, 0x1E0,
};

/* The unit's own functions, in address order (the forward declarations keep the source order free).
 * They are C linkage: the map spells every one `fn_XXXXXXXX`. */
extern "C" {
u32 res_tev_ref(ResHandle* pSelf);
u32 res_tev_ptr(ResHandle* pSelf);
u8 res_tev_get_num_stages(ResHandle* pSelf);
}

/* 0x8009A174 (0x64): `ResTev::ref` with the `g3d_restev_ac.h` validity assert (the non-const instance; the
 * shared header declares only the const one, so it is a C-named free function). */
extern "C" __declspec(noinline) inline u32 res_tev_ref(ResHandle* pSelf) {
    static char sMember[] = "ref";

    if (!reinterpret_cast<const nw4r::g3d::ResTev*>(pSelf)->IsValid()) {
        nw4r::db::Panic("g3d_restev_ac.h", 0x25, "%s::%s: Object not valid.", nw4r::g3d::ResTev::GetClassName(), sMember);
    }
    return res_tev_ptr(pSelf);
}

/* The tev block's checked handle; dead-stripped, it places the weak copy of res_tev_ref here. */
extern "C" u32 res_tev_ref_place(ResHandle* pSelf) {
    return res_tev_ref(pSelf);
}

/* 0x8009A1D8 - the raw `ref()` word. */
extern "C" u32 res_tev_ptr(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x8009A1E0 - call the block's display list with the size its stage count names: `PPCSync` first
 * when the caller asks for it, then `tev_display_list_size[stages - 1]` bytes at the block's +0x20. */
void nw4r::g3d::ResTev::CallDisplayList(bool flag) const {
    if (flag) {
        PPCSync();
    }

    GXCallDisplayList((void*)((u32)&ref() + 0x20), tev_display_list_size[(u8)res_tev_get_num_stages((ResHandle*)this) - 1]);
}

/* 0x8009A254 - the block's tev stage count (its +0x0C byte, 1..16). */
extern "C" u8 res_tev_get_num_stages(ResHandle* pSelf) {
    return *(u8*)((u32)&reinterpret_cast<const nw4r::g3d::ResTev*>(pSelf)->ref() + 0xC);
}

/* 0x8009A278 - copy the block to a 0x20-aligned destination, rebase its recorded size by the amount
 * the copy moved it by, and store the copy back to the cache. */
extern "C" u32 res_tev_copy_to(ResHandle* pSelf, void* pDst) {
    u32 src = res_tev_ref(pSelf);
    nw4r::g3d::detail::Copy32ByteBlocks(pDst, (const void*)src, 0x200);

    nw4r::g3d::ResTev copy(pDst);

    ResTevData* pData = (ResTevData*)res_tev_ref((ResHandle*)&copy);
    u32 size = pData->mDlSize;
    size = (u32)(size - ((u32)pDst - src));
    pData->mDlSize = size;

    res_tev_dc_store((ResHandle*)&copy, 0);
    return (u32)copy.mpData;
}

/* 0x8009A2F4 - store or invalidate the block's own `mSize` bytes. */
extern "C" void res_tev_dc_store(ResHandle* pSelf, s32 flag) {
    u32 base = res_tev_ref(pSelf);
    u32 size = *(u32*)res_tev_ref(pSelf);

    if (flag != 0) {
        nw4r::g3d::DC::StoreRange((void*)base, size);
    } else {
        nw4r::g3d::DC::StoreRangeNoSync((void*)base, size);
    }
}
