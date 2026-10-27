/* The stage work block `stage_w` (`stage/stg_w.cpp`, .bss 0x806B87C0) as a class, and the records it holds.
 *
 * One type for every reader of the block: `stage/stg_w.cpp` defines the object (MWCC's `__sinit` constructs it and
 * the member constructors below) and `pad_connect.cpp` clears its two record-group pointers.  The types sit in
 * their own header because the owner header `stage/stg_w.h` is included beside `pl.h`, whose `MHchar` view
 * differs from `sound/mhchar.h`'s.
 */
#ifndef MHTRI_STAGE_STAGE_WORK_H
#define MHTRI_STAGE_STAGE_WORK_H

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "g3d/g3d_obj.h"              /* nw4r::g3d::AnmObj */
#include "sound/mhchar.h"             /* MHchar */
#include "mh3_pad/vec3.h"             /* VEC3_ctor, setVec3 (rule 2) */
#include "ef/effect.h"                /* eft_vec_ctor (rule 2) */
#include "ef/eft_capsule_ctor.h"      /* eft_capsule_ctor (rule 2) */
#include "lobby/mhchar_construct.h"   /* mhchar_construct (rule 2) */

struct _BODY_DATA;

/* A `VEC3` the stage data constructs: the default and the three-float constructors are the out-of-line
 * `VEC3_ctor` / `setVec3` calls retail's constructors make. */
struct StageVec3 : VEC3 {
    StageVec3() { VEC3_ctor(this); }
    StageVec3(f32 x, f32 y, f32 z) { setVec3(this, x, y, z); }
}; /* size: 0xC */

/* One 48-byte row of an area record: three light colours and the three light positions. */
struct StageAreaRow {
    /* +0x00 */ u32 words[3];
    /* +0x0C */ StageVec3 vecs[3];

    StageAreaRow() __attribute__((never_inline)) {}
}; /* size: 0x30 */

/* The light parameter set `stage_light_set_copy` copies member by member: a kind byte, an intensity
 * and twelve colour words. */
typedef struct StageLightSet {
    /* +0x00 */ u8 kind;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ f32 intensity;
    /* +0x08 */ u32 colours[12];
} StageLightSet; /* size: 0x38 */

/* The two ambient colour triples `stage_ambient_copy` copies byte by byte. */
typedef struct StageAmbient {
    /* +0x00 */ u8 rgb[3];
    /* +0x03 */ u8 rgb2[3];
} StageAmbient; /* size: 0x6 */

/* One light record of an area: the ambient triples, the light set and the light direction. */
typedef struct StageLight {
    /* +0x00 */ StageAmbient ambient;
    /* +0x06 */ u8 pad_0x06[2];
    /* +0x08 */ StageLightSet set;
    /* +0x40 */ VEC3 dir;
} StageLight; /* size: 0x4C */

/* One fog record `stage_fog_apply` hands to the scene fog: the fog type, its range and colour. */
typedef struct StageFogRec {
    /* +0x00 */ u32 type;
    /* +0x04 */ f32 start;
    /* +0x08 */ f32 end;
    /* +0x0C */ u32 colour;     /* packed RGBA */
} StageFogRec; /* size: 0x10 */

/* One 476-byte per-area record from stage_w+0xC20. */
struct StageArea {
    /* +0x000 */ GXColor color;
    /* +0x004 */ StageAreaRow rows[6];
    /* +0x124 */ StageLight light;        /* the above-water light */
    /* +0x170 */ StageLight light_water;  /* the under-water light */
    /* +0x1BC */ StageFogRec fog;
    /* +0x1CC */ StageFogRec fog_b;

    StageArea() __attribute__((never_inline)) {}
}; /* size: 0x1DC */

/* A stage model: the `MHchar` with the tail the stage reads (the model's animation count); its constructor is
 * the out-of-line `mhchar_construct`. */
struct StageChar {
    /* +0x000 */ MHchar chr;
    /* +0x140 */ u8 pad_0x140[0x1C];
    /* +0x15C */ u32 anim_count;
    /* +0x160 */ u8 pad_0x160[4];

    StageChar() { mhchar_construct(&chr); }
}; /* size: 0x164 */

/* One of the two area load slots: the four models of a loaded area and their resources. */
struct StageSlot {
    /* +0x000 */ u8 index;
    /* +0x001 */ u8 state;              /* 0 free, 2 loaded */
    /* +0x002 */ u8 pad_0x002[2];
    /* +0x004 */ StageChar models[4];
    /* +0x594 */ u8 mapno;
    /* +0x595 */ u8 areano;
    /* +0x596 */ u8 pad_0x596[2];
    /* +0x598 */ u32 field_0x598;
    /* +0x59C */ s32 res_file[4];       /* the model's resource file, 0 when the model is unused */
    /* +0x5AC */ s32 res_id[4];         /* the resource id `nwDelResource` releases, -1 when none */
    /* +0x5BC */ s32 mem_index[4];      /* the resource memory block, -1 when none */
    /* +0x5CC */ u8 model_count;
    /* +0x5CD */ u8 pad_0x5CD[3];

    StageSlot() __attribute__((never_inline)) {}
}; /* size: 0x5D0 */

/* One of the stage's sixteen hit bodies (the breakable parts), a `_BODY_W` with its hit-id list. */
typedef struct StageBody {
    /* +0x00 */ u8 pad_0x00[5];
    /* +0x05 */ u8 active;
    /* +0x06 */ u8 hit;
    /* +0x07 */ u8 pad_0x07;
    /* +0x08 */ u8 area;
    /* +0x09 */ u8 pad_0x09[3];
    /* +0x0C */ u32 data;
    /* +0x10 */ u8 pad_0x10[4];
    /* +0x14 */ s16 damage;
    /* +0x16 */ u16 hit_ids[20];
    /* +0x3E */ u8 hit_count;
    /* +0x3F */ u8 pad_0x3F;
} StageBody; /* size: 0x40 */

/* One of the four breakable-part state records: the break state, the scatter of its three joints and the
 * damage seen last frame. */
struct StagePart {
    /* +0x00 */ u8 state;
    /* +0x01 */ u8 joint_done[3];
    /* +0x04 */ StageVec3 joint_base[3];   /* the joints' rest positions */
    /* +0x28 */ u32 joint_angle[3][3];
    /* +0x4C */ StageVec3 joint_pos[3];
    /* +0x70 */ u32 joint_rot[3][3];
    /* +0x94 */ s16 joint_time[3];
    /* +0x9A */ u8 field_0x9A;
    /* +0x9B */ bool visible;
    /* +0x9C */ s16 damage;
    /* +0x9E */ u8 pad_0x9E[2];

    StagePart() __attribute__((never_inline)) {}
}; /* size: 0xA0 */

/* One breakable part of an area: its body, joints, hidden joints, body data, kind, hit points and break effect. */
typedef struct StagePartDef {
    /* +0x00 */ u8 body;
    /* +0x01 */ u8 joints[3];
    /* +0x04 */ u8 vis_ids[4];
    /* +0x08 */ struct _BODY_DATA* body_data;
    /* +0x0C */ u8 kind;
    /* +0x0D */ u8 pad_0x0D;
    /* +0x0E */ u16 hp;
    /* +0x10 */ u8 eft_kind;
    /* +0x11 */ u8 pad_0x11[3];
    /* +0x14 */ VEC3 eft_pos;
    /* +0x20 */ f32 eft_scale;
    /* +0x24 */ s16 eft_param;
    /* +0x26 */ u8 pad_0x26[2];
} StagePartDef; /* size: 0x28 */

/* The sphere of an area effect emitter; its constructor is the out-of-line `eft_vec_ctor`. */
struct StageEftSphere {
    /* +0x00 */ nw4r::math::VEC3 center;
    /* +0x0C */ f32 radius;

    StageEftSphere() { eft_vec_ctor(&center); }
}; /* size: 0x10 */

/* The capsule of an area effect emitter; its constructor is the out-of-line `eft_capsule_ctor`. */
struct StageEftCapsule {
    /* +0x00 */ nw4r::math::CAPSULE capsule;

    StageEftCapsule() { eft_capsule_ctor(&capsule); }
}; /* size: 0x1C */

/* One of the four area effect emitters `eft_scnbox_load` fills from the area's scenario box file (the
 * effect unit's `EftEmitterRecord`): the shape mode and its sphere and capsule parameters. */
struct StageEftSeat {
    /* +0x00 */ s8 mode;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ StageEftSphere sphere;
    /* +0x14 */ StageEftCapsule capsule;
    /* +0x30 */ u8 pad_0x30[4];

    StageEftSeat() __attribute__((never_inline)) {}
}; /* size: 0x34 */

/* stage_w: the offsets this range touches. */
struct StageWork {
    /* +0x0000 */ u8 init_count;
    /* +0x0001 */ u8 field_0x0001;
    /* +0x0002 */ u8 field_0x0002;
    /* +0x0003 */ u8 field_0x0003;
    /* +0x0004 */ StageChar* chars[4];       /* the current area's models (slot models) */
    /* +0x0014 */ nw4r::g3d::AnmObj* anims[4]; /* the models' animation objects */
    /* +0x0024 */ StageSlot slots[2];
    /* +0x0BC4 */ s8 cur_slot;
    /* +0x0BC5 */ u8 mapno;
    /* +0x0BC6 */ u8 areano;
    /* +0x0BC7 */ StageAmbient ambient;
    /* +0x0BCD */ u8 pad_0x0BCD[3];
    /* +0x0BD0 */ StageLightSet light;
    /* +0x0C08 */ StageVec3 light_dir;
    /* +0x0C14 */ u8 flags;
    /* +0x0C15 */ u8 joint_state[2][4];     /* the joint visibility state of the two models (map, area) */
    /* +0x0C1D */ u8 pad_0x0C1D[3];
    /* +0x0C20 */ StageArea areas[15];
    /* +0x2804 */ f32 field_0x2804;
    /* +0x2808 */ StageVec3 field_0x2808;
    /* +0x2814 */ f32 anim_rate;
    /* +0x2818 */ f32 anim_rate_target;
    /* +0x281C */ u8 bitmask[15];
    /* +0x282B */ u8 pad_0x282B;
    /* +0x282C */ StageBody bodies[16];
    /* +0x2C2C */ StagePart parts[4];
    /* +0x2EAC */ u8 event_pending;
    /* +0x2EAD */ u8 vis_ids[8];            /* the joint ids the broken parts keep visible, 0xFF-terminated */
    /* +0x2EB5 */ u8 pad_0x2EB5[0x2EC0 - 0x2EB5];
    /* +0x2EC0 */ StageEftSeat eft_seats[4];
    /* +0x2F90 */ u8 alt_model;
    /* +0x2F91 */ u8 oneshot_flags;         /* bit 0: the one-shot clear; bit 7: armed for the next frame */
    /* +0x2F92 */ u16 area_mask;            /* one bit per area whose timer is armed */
    /* +0x2F94 */ u32 area_timers[16];      /* one 4-second timer per area */
    /* +0x2FD4 */ void* map_group_direct;   /* the record group `stage_map_group_get` returns for a kept id */
    /* +0x2FD8 */ void* map_group_remapped; /* the record group it returns for a remapped id */
    /* +0x2FDC */ s16 demo_countdown;       /* the frames until the demo markers are placed again */
    /* +0x2FDE */ u8 pad_0x2FDE[2];

    StageWork() __attribute__((never_inline)) {}
}; /* size: 0x2FE0 */

#endif /* MHTRI_STAGE_STAGE_WORK_H */
