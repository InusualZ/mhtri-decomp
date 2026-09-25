/*
 * nw4r g3d: `g3d_resnode.cpp` - the `ResNode` animation-result translation unit, `.text`
 * 0x80098D5C-0x80099400 (7 functions).
 *
 * Naming (brief section 2, evidence class 1 - a `__FILE__` string).  Every `nw4r::db::Panic` in the unit
 * passes `.data` 0x805915C0 = "g3d_resnode.cpp" as its file argument (read out of
 * orig/RMHE08/sys/main.dol's `.data`): fn_80098D5C (lines 0x20/0x21), fn_80098F6C (0x7E/0x7F),
 * fn_80099178 (0xEF) and fn_80099278 (0x103).  The preceding registered unit `g3d/g3d_resmat.cpp`
 * (0x800947A4-0x80098D5C) already names that string as its right seam, so the module is `g3d`, the file
 * takes its evidenced TU name, and `langcheck.py` agrees (the name is a `.cpp` and the `Panic`
 * relocation is a C++ mangling).  The whole `g3d_resnode.cpp` `.data` fragment is exactly these three
 * strings - "g3d_resnode.cpp" (0x10) + "NW4R:Pointer must not be NULL (pResult)" (0x28) +
 * "NW4R:Failed assertion IsValid()" (0x20) = 0x58 bytes, ending at 0x80591618 - where the next TU's
 * fragment starts (`lbl_80591618` = "g3d_resshp.cpp").
 *
 * The right seam 0x80099400 is `g3d_resshp.cpp`'s first body, and it is proven from the classes, not
 * from the queue: fn_80099378/fn_800993B4 call `fn_8005D218` (`ResNode::ref` - its failing assert names
 * `.sdata` 0x80791148 = "ResNode" and `.data` 0x8058BE98 = "g3d_resnode_ac.h") on their own `this`,
 * while the next body, fn_80099400, calls `fn_80077674` (`ResShp::ref` - its assert names `.sdata`
 * 0x807911F0 = "ResShp" and `.data` 0x8058E720 = "g3d_resshp_ac.h") on its own `this`.  Two different
 * classes cannot be methods of one source file, so the boundary sits between them.
 *
 * **The brief's cap 0x80098D5C-0x800997E0 spans two translation units.**  proposal/800997E0 claims
 * 0x800997E0-0x8009A748 as `g3d_resshp.cpp` and both caps are contiguous, so this unit registers only
 * its own non-overlapping part, 0x80098D5C-0x80099400 (the sibling's left seam must move to 0x80099400;
 * see the unit report and the outbox config_requests).
 *
 * Sections: `.text` 0x80098D5C-0x80099400, `extab` 0x80009850-0x80009880 (6 records), `extabindex`
 * 0x80022854-0x8002289C (6 records) - contiguous with `g3d/g3d_resmat.cpp`'s end (0x80009850) and with
 * `g3d_resshp.cpp`'s start (0x80009880 / 0x8002289C).
 *
 * rule 7 deferred: the symbol map has only `fn_XXXXXXXX` for this range (checked with `grep` over
 * config/RMHE08/symbols.txt: every 0x80098D5C..0x80099400 row is a bare `fn_XXXXXXXX`).
 *
 * What the bodies are: fn_80098F6C fills an `AnmResult` from the node's own animation state and
 * fn_80098D5C applies an `AnmResult` back to the node, so the pair is nw4r's `GetAnmResult`/
 * `SetAnmResult`; fn_80099178/fn_80099278 store the translate/rotate triple and keep `ResNodeData`'s
 * "all three channels animated" bit coherent through fn_80099134.
 *
 * Measurement.  `python tools/units/recompile.py g3d/g3d_resnode.cpp --measure <symbol>` against MAIN's
 * retired per-symbol `auto_*_text.o` targets (MAIN has no split object for this unit until the
 * registration lands).  6 of 7 bodies are byte-identical (100.0 %); fn_80098D5C is 96.10 % (528 B both;
 * instruction-identical modulo register naming - the target keeps the flag word in r31 and the
 * `flags & ~0x10` temp in r29 where MWCC here picks r29/r28).  The object's sections equal the claim
 * exactly: `.text` 0x6A4, `extab` 0x30 (6 records), `extabindex` 0x48 (6 entries).
 *
 * The one codegen lever is the file-scoped `#pragma peephole off` below: with it every `rlwinm` flag
 * extract keeps the target's explicit compare (`rlwinm r0,rA,0,mb,me` + `cmpwi r0,0` + `beq`) instead of
 * the folded record form (`rlwinm.` + `beq`), which takes fn_80099134 from 81.47 % to 100.0 %.
 * `#pragma fp_contract off` rides along with `g3d/g3d_resanmcamera.cpp`'s precedent (these bodies use no
 * fused ops either way).
 *
 * What the bodies are: fn_80098F6C fills an `AnmResult` from the node's own animation state and
 * fn_80098D5C applies an `AnmResult` back to the node, so the pair is nw4r's `GetAnmResult`/
 * `SetAnmResult`; fn_80099178/fn_80099278 store the translate/rotate triple and keep `ResNodeData`'s
 * "all three channels animated" bit coherent through fn_80099134.
 *
 * Residual (best-scoring variant kept, per the matching policy): fn_80098D5C's register assignment
 * (see above); everything else is byte-identical.  The one naming caveat: the `fn_80099378`/
 * `fn_800993B4` pair is a `ResNode` sub-resource lookup whose target class is only evidenced by
 * fn_8008A220's header (`g3d_resuser_ac.h`), so the field is named `mSubResOfs` best-effort - the offset
 * is the fact.
 */
#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/res_common.h"
#include "g3d/g3d_resnode.h"

/* The target's `-O3` schedule is retail only with the peephole pass off: every flag extract keeps an
 * explicit `cmpwi` after the `rlwinm` instead of the folded record form `rlwinm.`. */
#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic(const char*, int, const char*, ...) - the owner's real C++ declaration, called
 * through its signature, never the mangled spelling (docs/plan.md 6.5 rule 9). */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The pooled constants this unit reads.  They live in the original `.data`/`.sdata2`, which the unit
 * does not claim, so they are declarations; each is still unsplit in the map (its address band
 * interleaves modules), so the declaration has no registered owner to move to. */
extern "C" {
extern const char lbl_805915C0[]; /* "g3d_resnode.cpp"                            .data */
extern const char lbl_805915D0[]; /* "NW4R:Pointer must not be NULL (pResult)"    .data */
extern const char lbl_805915F8[]; /* "NW4R:Failed assertion IsValid()"            .data */
extern const f32 lbl_80795F48;          /* 1.0f                                         .sdata2 */
extern const f32 lbl_80795F4C;          /* 0.0f                                         .sdata2 */
}

/* The neighbours this unit calls; the registered owner is named beside each.  The prototypes sit in
 * this `extern "C"` block (the shape `g3d/fn_8005AA28.cpp` uses) so the rule-2 conformance pass can
 * lift them into the owners' headers.  These are `fn_XXXXXXXX` stems, not manglings, so rule 9 does
 * not reach them. */
extern "C" {
void fn_80041E40(VEC3* pDst, const VEC3* pSrc);              /* owner: src/mh3_pad.cpp */
void fn_80041E8C(VEC3* pOut, f32 x, f32 y, f32 z);           /* owner: src/mh3_pad.cpp */
void fn_800504D4(MTX34* pMtx);                               /* owner: src/fn_8004CAD8.cpp */
s32 fn_8005AAEC(const ResHandle* pSelf);                     /* owner: src/g3d/fn_8005AA28.cpp */
void* fn_8005AAE4(const ResHandle* pSelf);                   /* owner: src/g3d/fn_8005AA28.cpp */
ResNodeData* fn_8005D0C4(const ResHandle* pSelf);            /* owner: src/g3d/g3d_anmchr.cpp */
ResNodeData* fn_8005D218(const ResHandle* pSelf);            /* owner: src/g3d/g3d_anmchr.cpp */
VEC3* fn_80067E54(VEC3* pOut, const VEC3* pIn);              /* owner: src/g3d/fn_80063888.cpp */
void fn_8008C484(f32* pOut, f32 x, f32 y, f32 z);            /* owner: src/g3d/g3d_resanmchr.cpp */
void* fn_8008A220(void* pOut, u32 value);                    /* owner: src/g3d/g3d_resanmcamera.cpp */
}

/* The unit's own functions, in address order (the forward declarations keep the source order
 * free).  They are C linkage: the map spells every one `fn_XXXXXXXX`. */
extern "C" {

void fn_80098D5C(ResHandle* pSelf, AnmResult* pResult);
void fn_80098F6C(ResHandle* pSelf, AnmResult* pResult);
void fn_80099134(ResNodeData* pSelf);
void fn_80099178(ResHandle* pSelf, f32 x, f32 y, f32 z);
void fn_80099278(ResHandle* pSelf, f32 x, f32 y, f32 z);
u32 fn_80099378(ResHandle* pSelf);
u32 fn_800993B4(ResHandle* pSelf, u32 ofs);

/* -------------------------------------------------------------------------------------------------- */

/* Apply one `AnmResult` to the node: the target reads the record's channel bits and rewrites the
 * scale/rotation-matrix/translation parts it selects from the node's own state. */
void fn_80098D5C(ResHandle* pSelf, AnmResult* pResult) {
    if (pResult == NULL) {
        nw4r::db::Panic(lbl_805915C0, 0x20, lbl_805915D0);
    }
    if (fn_8005AAEC(pSelf) == 0) {
        nw4r::db::Panic(lbl_805915C0, 0x21, lbl_805915F8);
    }
    if (fn_8005AAEC(pSelf) != 0) {
        u32 flags = pResult->flags;
        ResNodeData* pData = fn_8005D0C4(pSelf);
        if ((flags & 0x80) != 0) {
            u32 dataFlags = pData->mFlags;
            if ((dataFlags & 0x8) != 0) {
                pResult->scale[2] = lbl_80795F48;
                pResult->scale[1] = lbl_80795F48;
                pResult->scale[0] = lbl_80795F48;
                flags |= 0x18;
            } else {
                u32 newFlags = flags & ~0x10;
                if ((dataFlags & 0x10) != 0) {
                    newFlags = flags | 0x10;
                }
                fn_80041E40((VEC3*)pResult->scale, (const VEC3*)pData->mScale);
                flags = newFlags & ~0xA;
            }
        }
        if ((flags & 0x100) != 0) {
            VEC3 translate;
            fn_80041E8C(&translate, pResult->mtx[3], pResult->mtx[7], pResult->mtx[11]);
            if ((pData->mFlags & 0x4) != 0) {
                fn_800504D4((MTX34*)pResult->mtx);
                flags |= 0x20;
            } else {
                fn_8008C484(pResult->mtx, pData->mRotate[0], pData->mRotate[1], pData->mRotate[2]);
                flags &= ~0x26;
            }
            pResult->mtx[3] = translate.x;
            pResult->mtx[7] = translate.y;
            pResult->mtx[11] = translate.z;
            flags |= 0x80000000;
        }
        if ((flags & 0x200) != 0) {
            if ((pData->mFlags & 0x2) != 0) {
                pResult->mtx[11] = lbl_80795F4C;
                pResult->mtx[7] = lbl_80795F4C;
                pResult->mtx[3] = lbl_80795F4C;
                flags |= 0x40;
            } else {
                pResult->mtx[3] = pData->mTranslate[0];
                pResult->mtx[7] = pData->mTranslate[1];
                pResult->mtx[11] = pData->mTranslate[2];
                flags &= ~0x46;
            }
        }
        if ((flags & 0x20) != 0 && (flags & 0x40) != 0) {
            flags |= 0x4;
            if ((flags & 0x8) != 0) {
                flags |= 0x2;
            }
        }
        pResult->flags = flags & ~0x380;
    }
}

/* Fill one `AnmResult` from the node's own animation state (the pair's other direction). */
void fn_80098F6C(ResHandle* pSelf, AnmResult* pResult) {
    if (pResult == NULL) {
        nw4r::db::Panic(lbl_805915C0, 0x7E, lbl_805915D0);
    }
    if (fn_8005AAEC(pSelf) == 0) {
        nw4r::db::Panic(lbl_805915C0, 0x7F, lbl_805915F8);
    }
    if (fn_8005AAEC(pSelf) != 0) {
        ResNodeData* pData = fn_8005D0C4(pSelf);
        u32 flags = 0;
        u32 dataFlags = pData->mFlags;
        if ((dataFlags & 0x8) != 0) {
            flags |= 0x18;
            pResult->scale[2] = lbl_80795F48;
            pResult->scale[1] = lbl_80795F48;
            pResult->scale[0] = lbl_80795F48;
        } else {
            if ((dataFlags & 0x10) != 0) {
                flags |= 0x10;
            }
            fn_80041E40((VEC3*)pResult->scale, (const VEC3*)pData->mScale);
        }
        if ((pData->mFlags & 0x4) != 0) {
            fn_800504D4((MTX34*)pResult->mtx);
            flags |= 0x20;
        } else {
            VEC3 rotate;
            fn_80041E40((VEC3*)pResult->rotate, fn_80067E54(&rotate, (const VEC3*)pData->mRotate));
            fn_8008C484(pResult->mtx, pData->mRotate[0], pData->mRotate[1], pData->mRotate[2]);
        }
        if ((pData->mFlags & 0x2) != 0) {
            flags |= 0x40;
        } else {
            pResult->mtx[3] = pData->mTranslate[0];
            pResult->mtx[7] = pData->mTranslate[1];
            pResult->mtx[11] = pData->mTranslate[2];
        }
        if ((flags & 0x20) != 0 && (flags & 0x40) != 0) {
            flags |= 0x4;
            if ((flags & 0x8) != 0) {
                flags |= 0x2;
            }
        }
        flags |= 0x80000000;
        flags |= 0x1;
        if ((pData->mFlags & 0x20) != 0) {
            flags |= 0x400;
        }
        if ((pData->mFlags & 0x40) != 0) {
            flags |= 0x800;
        }
        pResult->flags = flags;
    }
}

/* Keep `ResNodeData`'s "all three channels are animated" bit coherent with the rotate/translate/scale
 * bits beside it. */
void fn_80099134(ResNodeData* pSelf) {
    u32 flags = pSelf->mFlags;
    if ((flags & 0x2) != 0 && (flags & 0x4) != 0 && (flags & 0x8) != 0) {
        pSelf->mFlags = flags | 0x1;
    } else {
        pSelf->mFlags &= ~0x1;
    }
}

/* Store the translate triple, setting the channel bit when the triple is zeroed. */
void fn_80099178(ResHandle* pSelf, f32 x, f32 y, f32 z) {
    if (fn_8005AAEC(pSelf) == 0) {
        nw4r::db::Panic(lbl_805915C0, 0xEF, lbl_805915F8);
    }
    if (fn_8005AAEC(pSelf) != 0) {
        ResNodeData* pData = (ResNodeData*)fn_8005AAE4(pSelf);
        if (lbl_80795F4C == x && lbl_80795F4C == y && lbl_80795F4C == z) {
            pData->mFlags |= 0x2;
        } else {
            pData->mFlags &= ~0x2;
        }
        fn_80099134(pData);
        pData->mTranslate[0] = x;
        pData->mTranslate[1] = y;
        pData->mTranslate[2] = z;
    }
}

/* Store the rotate triple, setting the channel bit when the triple is zeroed. */
void fn_80099278(ResHandle* pSelf, f32 x, f32 y, f32 z) {
    if (fn_8005AAEC(pSelf) == 0) {
        nw4r::db::Panic(lbl_805915C0, 0x103, lbl_805915F8);
    }
    if (fn_8005AAEC(pSelf) != 0) {
        ResNodeData* pData = (ResNodeData*)fn_8005AAE4(pSelf);
        if (lbl_80795F4C == x && lbl_80795F4C == y && lbl_80795F4C == z) {
            pData->mFlags |= 0x4;
        } else {
            pData->mFlags &= ~0x4;
        }
        fn_80099134(pData);
        pData->mRotate[0] = x;
        pData->mRotate[1] = y;
        pData->mRotate[2] = z;
    }
}

/* Resolve the node's sub-resource: the offset at +0x6C, from the node's own base. */
u32 fn_80099378(ResHandle* pSelf) {
    ResNodeData* pData = fn_8005D218(pSelf);
    return fn_800993B4(pSelf, pData->mSubResOfs);
}

/* Build the one-word sub-resource handle at `base + ofs` (0 for a null offset) and return its word. */
u32 fn_800993B4(ResHandle* pSelf, u32 ofs) {
    u32 base = (u32)pSelf->mpData;
    if (ofs != 0) {
        u32 present;
        return *(u32*)fn_8008A220(&present, base + ofs);
    }
    u32 absent;
    return *(u32*)fn_8008A220(&absent, 0);
}

}  /* extern "C" */
