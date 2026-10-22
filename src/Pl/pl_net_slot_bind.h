/* Pl/pl_net_slot_bind.h - the declarations of `pl_net_slot_bind`, `pl_equip_models_set`, `pl_model_release`, `pl_equip_model_set`, `pl_parts_color_set`, which `Pl/pl_act.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em020_prog.cpp` calls them through. */
#ifndef MHTRI_PL_PL_NET_SLOT_BIND_H
#define MHTRI_PL_PL_NET_SLOT_BIND_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif
void pl_net_slot_bind(struct _PLW* plw, u8 slot);
void pl_equip_models_set(struct _PLW* plw, u8* equip_a, u8* equip_b, u8* equip_c);
void pl_model_release(struct _PLW* plw);
void pl_equip_model_set(struct _PLW* plw, u8* equip);
void pl_parts_color_set(struct _PLW* plw, u32 part, u8 color);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_NET_SLOT_BIND_H */
