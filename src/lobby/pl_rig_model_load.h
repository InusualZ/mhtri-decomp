/* lobby/pl_rig_model_load.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_equip_page.cpp`'s model-rig part
 * loaders and resets, for `Pl/player_control.cpp`. */
#ifndef MHTRI_LOBBY_PL_RIG_MODEL_LOAD_H
#define MHTRI_LOBBY_PL_RIG_MODEL_LOAD_H

#include "types.h"
#include "Pl/pl_se_rig.h"   /* PlSeRig */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802230F8 (0xB4): clears the rig's armour model `slot`. */
void pl_rig_model_reset(PlSeRig* rig, u32 slot);
/* 0x802231AC (0xAC): clears the rig's weapon sub-model `slot`. */
void pl_rig_sub_reset(PlSeRig* rig, u32 slot);
/* 0x80223ADC (0x58): shows the hair model's parts the helm leaves visible. */
void pl_rig_hair_parts_apply(PlSeRig* rig);
/* 0x80223BCC (0x88): shows the face model's parts the helm leaves visible. */
void pl_rig_face_parts_apply(PlSeRig* rig);
/* 0x80223CEC (0x168): builds the rig's armour model `slot` from its bound file. */
void pl_rig_model_load(PlSeRig* rig, u8 slot);
/* 0x80223EF0 (0x3C8): builds the rig's weapon sub-model `slot` from part `file_id`'s bound file. */
void pl_rig_weapon_load(PlSeRig* rig, u8 slot, u8 file_id);
/* 0x80223E54 (0x9C): sets the rig's weapon kind from its player's weapon type. */
void pl_rig_weapon_kind_set(PlSeRig* rig);
/* 0x802242B8 (0x1E0): rebinds the rig's model callbacks. */
void pl_rig_callbacks_rebind(PlSeRig* rig);
/* 0x80224AC4 (0x364): moves the rig's models with its player. */
void pl_rig_model_move(PlSeRig* rig);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_PL_RIG_MODEL_LOAD_H */
