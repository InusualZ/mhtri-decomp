/* ef/eft_res.h - the declarations of `ef/eft_res.cpp`'s symbols its consumers call, in the owner's own `extern "C"`
 * signatures (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_EF_EFT_RES_H
#define MHTRI_EF_EFT_RES_H

#include "types.h"
#include "ef.h"

/* The effect resource slot `eft_res_slot_get` pools; its layout stays the owner's business. */
struct EftResSlot;

#ifdef __cplusplus
extern "C" {
#endif

/* Retires one pooled effect's runtime record. */
void eft_res_slot_release(void* self);
/* The name consumers outside `ef/` use for it. */
#define eft_record_retire(self) eft_res_slot_release(self)
/* Places `count` pooled models in the `mode` layout and files the result through `arg`. */
void eft_res_models_spawn(_EFT* self, void** models, s32 mode, s32 count, void* arg);
/* Pools the slots `size` bytes' worth of effect records need and returns the first record. */
EftResSlot* eft_res_slot_get(u32 size);
/* Releases `count` handles from `list` back to the heap. */
void eft_res_models_release(void* list, s32 count);
/* 0x800F92F4 - the per-mode alive check the effect state bodies make (`mode` 0 is the "may the record keep
 * stepping" test; `ef/fn_8030681C.cpp`'s kind-1 bodies branch on it).  The second parameter is `u32` because
 * `ef/fn_801173AC.cpp` includes this header beside its own `u32` declaration of the address (a second width is
 * `(10197) illegal function overloading`); the owner's own `void*` `self` would be a third view there. */
s32 eft_res_spawn_gate_ck(_EFT* self, u32 mode);
/* Takes one pooled model record out of the effect-model pool and returns it (the `EftModel`
 * `res_eft_*_model_create` then binds a character to); `ef/eft035.cpp` seeds its model slots with it. */
u8* eft_res_model_get(void);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* The two model-create entry points are C++ free functions (`res_eft_model_create__FP6MHcharUsUl`,
 * `res_eft_UV_model_create__FP6MHcharUsUllPP9_g3d_worklUc`), declared at C++ scope (rule 9); `ef/eft035.cpp`
 * calls both, and `res_eft_UV_model_create` is still unwritten in its owner. */
struct MHchar;
struct _g3d_work;
/* 0x800F91B4 - creates the pooled effect `id` of resource group `group`; `stage/shell.cpp`'s
 * `res_eft_create_shell` forwards to it with flags 0. */
namespace nw4r { namespace ef { struct Effect; } }
nw4r::ef::Effect* res_eft_create(u16 group, u16 id, u32 flags);
void* res_eft_model_create(MHchar* model, u16 id, u32 arg);
void* res_eft_UV_model_create(MHchar* model, u16 id, u32 arg, long mode, struct _g3d_work** list,
                              long count, u8 flag);

/* The UV-animated model `res_eft_UV_model_create` returns, reached through its table: slot 0x24 steps the animation,
 * slot 0x28 sets its scalar.  Declared only (its owner emits the table).  size: 0x04 - lower bound, an approximation
 * (the vtable word) */
class EftUvModelObject {
public:
    virtual void unused_0x08();
    virtual void unused_0x0C();
    virtual void unused_0x10();
    virtual void unused_0x14();
    virtual void unused_0x18();
    virtual void set_frame(f32 frame);
    virtual void unused_0x20();
    virtual void update();
    virtual void set_scalar(f32 value);
};
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x800F65B4 / 0x800F6710 - set the effect control's three pools up, and load the mode's common effect models
 * (GUESS names). */
void eft_control_init(void);
void eft_common_load(void);
/* 0x800F6B6C - files entry `index` of the group/kind proID table under a free slot; the slot ordinal or 0xFF
 * (GUESS name). */
u8 eft_pro_slot_assign(u8 group, u8 kind, u8 set, u8 sub, u8 index);
/* 0x800F6688 - releases every live effect slot (GUESS name). */
void eft_res_release_all(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x800F6520 - resets the effect resource pool (the system core's resets). */
void eft_res_pool_reset(void);
/* 0x80794A88 / 0x80794A8C - the per-map common eft and efm resource name tables `stage_map_obj_load` reads. */
extern char*** map_com_eft_name_ptr;
extern char*** eft_map_common_mdl_name_ptr;
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_RES_H */
