/* g3d/g3d_anmclr.h - the cross-unit declarations of `g3d/g3d_anmclr.cpp` (C linkage, plain map stems): the material
 *   handle copies, the colour-result applier, the type-name store copies and the checked resolver.  It declares no
 *   global placement `operator delete`, so a consumer's placement `new` gets no landing pad retail does not have. */
#ifndef MHTRI_G3D_G3D_ANMCLR_H
#define MHTRI_G3D_G3D_ANMCLR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The material handle's copy constructor and its base-handle copy (one word each). */
struct ResHandle* res_mat_copy_ctor(struct ResHandle* pDst, const struct ResHandle* pSrc);
void res_mat_common_copy_ctor(struct ResHandle* pDst, const struct ResHandle* pSrc);

/* The three type-name store copies the texture-SRT classes' GetTypeObj members call: each stores v through out and
 * returns out. */
const u8 **type_obj_set_name_texsrt_res(const u8 **out, const u8 *v);
const u8 **type_obj_set_name_texsrt_node(const u8 **out, const u8 *v);
const u8 **type_obj_set_name_texsrt_override(const u8 **out, const u8 *v);

/* 0x80063FD0 - the checked `ResAnmScn` resource resolver `g3d/g3d_resfile.cpp`'s revision checks read. */
u32 fn_80063FD0(void *p);

/* 0x80064754 - returns the colour word itself (the identity the word readers pass their pointer through). */
u32 *color_to_u32_ref(u32 *self);
/* 0x80064834 - constructs a colour as opaque white (stores 0xFFFFFFFF through the word setter), returns `self`. */
u32 *color_ctor_white(u32 *self);

#ifdef __cplusplus
}

namespace nw4r { namespace g3d { struct ClrAnmResult; } }

/* 0x80064128 (0x5C0): applies a colour result to a material's channel and tev-colour blocks. */
extern "C" void apply_clr_anm_result(struct ResHandle chan, struct ResHandle tevColor,
                                     const nw4r::g3d::ClrAnmResult* pResult);
#endif

#endif /* MHTRI_G3D_G3D_ANMCLR_H */
