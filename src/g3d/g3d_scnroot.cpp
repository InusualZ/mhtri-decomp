/*
 * g3d/g3d_scnroot.cpp - nw4r g3d `ScnRoot` scene-graph root (camera, fog, light, the draw buffers and the scnMdl
 *   list).
 * RANGE. .text 0x800827E4-0x80084630 (49 functions); extab 0x800088F4-0x80008AFC, extabindex 0x800212C4-0x800214BC,
 *   .rodata 0x8056F6D0-0x8056F6E0 (the "ScnRoot" name record), .data 0x8058F530-0x8058F750 (opens on
 *   "g3d_scnroot.cpp"), .sdata 0x80791228-0x80791238, .sdata2 0x80795E68-0x80795E70.  The right edge is where
 *   `g3d/g3d_state.cpp` opens: fn_80084630 is the first function citing "g3d_state.cpp"; the four functions
 *   0x8008452C-0x80084630 are the class's run-time type members (the vtable in this unit's `.data` names three of
 *   them).
 * NAMES. Map stems, plus `nw4r::g3d::ScnRoot::GetCamera(int)` (0x80082B70) and `ScnRoot::SetCurrentCamera(int)`
 *   (0x80082C08).  The pool's asserts name `bufOpa`/`bufXlu` and `0 <= camID && camID < NUM_CAMERA`.
 *   g3d_scn_root_get_type_obj is a GUESS, g3d_scn_root_get_type_name is a GUESS, g3d_scn_root_is_derived_from is a
 *   GUESS, g3d_scn_root_get_type_obj_static is a GUESS (ScnRoot's GetTypeObj, GetTypeName, IsDerivedFrom and
 *   GetTypeObjStatic, by their vtable slots and bodies; free functions until the class is declared with its
 *   virtuals), G3dVtObject is a GUESS, scn_typename_ScnRoot is a GUESS (the record the type lookups read).
 * RESIDUALS. 45 functions unwritten (objdiff scores them zero) in one run, 0x800827E4-0x8008452C.
 *   g3d_scn_root_get_type_name: one relocation argument differs.
 *   flipcheck: `.text` short of the claim; `.rodata`, `.data`, `.sdata` and `.sdata2` are claimed and not emitted.
 * SHAPES. File-scope `#pragma peephole off` and `#pragma pool_data off` (retail keeps every `clrlwi` + `cmpwi`).
 */

#include "types.h"
#include "g3d/g3d_scnroot.h"
#include "g3d/g3d_anmchr.h"  /* TypeObj::GetTypeName and operator==, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/fn_80075DCC.h" /* type_obj_set_name_scnleaf (rule 2) */
#include "g3d/g3d_scnobj.h"  /* nw4r::g3d::ScnGroup, owned by g3d/g3d_scnobj.cpp (rule 2) */

#pragma peephole off
#pragma pool_data off

/* The object `g3d_scn_root_get_type_name` dispatches on: its first word is a vtable and the call is slot +0x14. */
class G3dVtObject {
public:
    virtual void m00(); /* +0x00 */
    virtual void m04(); /* +0x04 */
    virtual void m08(); /* +0x08 */
    virtual void m0C(); /* +0x0C */
    virtual void m10(); /* +0x10 */
    virtual u32 m14();  /* +0x14 - the slot g3d_scn_root_get_type_name calls */
}; /* size: 0x4 */

extern "C" {

const u8* g3d_scn_root_get_type_obj(void);
u32 g3d_scn_root_get_type_name(G3dVtObject* pSelf);
u32 g3d_scn_root_is_derived_from(nw4r::g3d::ScnGroup* pSelf, u32* pArg);
const u8* g3d_scn_root_get_type_obj_static(void);

/* The `ScnRoot` singleton lookup: `type_obj_set_name_scnleaf` stores the found object through its out-parameter
 * and returns that parameter, so the result is the word it stored. */
const u8* g3d_scn_root_get_type_obj(void) {
    const u8* pScnRoot;
    return *type_obj_set_name_scnleaf(&pScnRoot, (const u8*)scn_typename_ScnRoot);
}

/* A virtual dispatch on slot +0x14 whose result is handed to the `TypeObj::GetTypeName` unwrapper. */
u32 g3d_scn_root_get_type_name(G3dVtObject* pSelf) {
    u32 value = pSelf->m14();
    return (u32)reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&value)->GetTypeName();
}

/* Returns 1 when `pArg` is the `ScnRoot` type object, otherwise defers to `ScnGroup::IsDerivedFrom`. */
u32 g3d_scn_root_is_derived_from(nw4r::g3d::ScnGroup* pSelf, u32* pArg) {
    const u8* pScnRoot = g3d_scn_root_get_type_obj_static();
    if ((*reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(pArg) == *reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&pScnRoot))) {
        return 1;
    }
    u32 key = *pArg;
    return pSelf->nw4r::g3d::ScnGroup::IsDerivedFrom(
        *reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&key));
}

/* The same lookup, emitted a second time as the class's static accessor. */
const u8* g3d_scn_root_get_type_obj_static(void) {
    const u8* pScnRoot;
    return *type_obj_set_name_scnleaf(&pScnRoot, (const u8*)scn_typename_ScnRoot);
}

}
