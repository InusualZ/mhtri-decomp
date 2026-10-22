/*
 * camera/camera_main.cpp - the game camera: the per-player camera work (`CamWork`, two of them at `camera_work`),
 * its six cameras (follow, aim, shoulder, event, area, lobby), the quake slots and the g3d camera accessors.
 *
 * RANGE.  `.text` 0x802B5640..0x802BF278 (157 functions, every one written); extab 0x80013E04..0x8001420C,
 *   extabindex 0x80031B84..0x80032190, `.ctors` 0x8056F380..0x8056F384, `.data` 0x805CFBE8..0x805D39A0, `.bss`
 *   0x806BB7E0..0x806BC300, `.sdata` 0x807924B0..0x807924E8, `.sdata2` 0x8079A4D8..0x8079A600 (the block in
 *   config/RMHE08/splits.txt).  The `.sdata2` pool is emitted from the literals and is byte-identical.
 * FLAGS.  `cflags_main`; the file builds with `#pragma peephole off` (retail keeps the unfused `rlwinm` + `!= 0`,
 *   the `clrlwi`-before-`add` index masks and the `li r0,N` + `psq_lx` epilogue pair) and `#pragma fp_contract off`
 *   (retail keeps `fmuls` + `fadds` in camera_blend_mix and camera_follow_update).
 * NAMES.  The record layout is traced from the constructors (camera_work_ctor builds the six cameras in order:
 *   camera_follow_ctor the follow camera's fifteen vectors, camera_aim_ctor the aim camera, camera_shoulder_ctor the
 *   shoulder camera, camera_event_ctor the event camera, camera_area_ctor the area camera, camera_lobby_ctor the lobby
 *   camera and its goal, camera_quake_ctor the three quake slots) and from the camera index each camera hands
 *   camera_view_apply (0..5).  GUESS (from each body and its callers): every `camera_*` function and table name of the
 *   unit, the field names of `CamWork` and its members, and the foreign callee names proposed in
 *   config/RMHE08/name_registry.jsonl (lane camera-main) with their leaf headers.
 * RESIDUALS.
 *   - `light/light.cpp` views the same 0x4F8 bytes as `LightWork` (light/light_work.h) and declares the accessor
 *     `camera_work_get` with that return type, so this unit casts its result to `CamWork*`; the constructors
 *     (camera_work_ctor..camera_follow_ctor) and camera_quake_update keep the `LightWork` member types.
 *   - The unit's `.data`, `.bss`, `.sdata` and `.ctors` are not emitted: the tables are declared `extern` under their
 *     map names (the camera ranges, shake, aim-eye, kill-cut key, area, lobby, script and demo-camera tables) and
 *     the two camera works are not a defined static object (camera_work_construct is the `.ctors` entry).
 *   - camera_follow_collide: the goto-free shape (a `turned` flag around the push pass) measures 95.70; the
 *     original's two jumps into the ground clamp measure 97.96 (rule 8 keeps the flag).
 *   - Switch lowering: camera_follow_yaw_input, camera_area_blend_update, camera_follow_kill_cut and
 *     camera_lobby_update test the two-value range first in retail (`subi`/`cmplwi`/`ble`); MWCC lowers our switches
 *     to a compare chain (case orders, `if` chains and `s32` selectors tried).
 *   - Register allocation and scheduling: camera_follow_update (the closest-height pick), camera_follow_shake_update
 *     (one slot pointer), camera_follow_lock_update, camera_follow_dive_ck, camera_aim_update, camera_shoulder_update,
 *     camera_event_script_step, camera_event_dcm_run, camera_quake_apply, camera_water_clamp, camera_lobby_viewport_fit,
 *     camera_work_init, camera_work_update (`beq`/`b` and two `clrlwi`s around the camera kind).
 *   - camera_quake_update: retail materialises the zero once at the top; ours rematerialises it per arm.
 *   - camera_event_ctor: the first run walk has its pointer/bound register pair swapped.
 * SHAPES.  The vector helpers that write through r3 (`subVec3`, `addVec3`, `vec3_scale`, `vec3_div`,
 *   `get_camera_target_pos`) are called through views returning the vector, which reproduces the original's
 *   temporaries and their stack slots; the C++ accessors return `CamVec3`/`CamMtx34`, whose copy constructor makes
 *   MWCC build the result in the caller's return slot (the manglings carry no return type).  A float parameter's
 *   position decides where its load is scheduled (coll_line_check takes the radius third).  Literals are written in
 *   the order the pool holds them (`0.7f + 0.3f * t`).
 */

#include "types.h"
#include "nw4r/math.h"
#include "MTX/mtx.h"
#include "light/light_work.h"
#include "Pl/plw.h"
#include "enemy/ENEMY_WORK.h"

#include "scene_current_camera_get.h"
#include "g3d/g3d_camera.h"
#include "g3d/g3d_scnroot.h"
#include "g3d/g3d_calcworld.h"
#include "menu/get_pop_dat_ptr.h"
#include "mh3_pad.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"

#include "camera/camera_main.h"


/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

struct CamScriptOp;
struct CamDcmKey;
struct CamAreaRec;
struct _LB_NPC;


/* The view every camera of the work starts with: where it sits and looks, the previous frame's pair, its roll
 * and field of view, and the step/timer pair each camera's state machine runs on.  `camera_view_apply` hands the
 * position, the target, `fovy` and `roll` to the g3d camera the index selects. */
struct CamView {
    /* +0x00 */ nw4r::math::VEC3 pos;
    /* +0x0C */ nw4r::math::VEC3 target;
    /* +0x18 */ nw4r::math::VEC3 prev_pos;
    /* +0x24 */ nw4r::math::VEC3 prev_target;
    /* +0x30 */ f32 roll;
    /* +0x34 */ f32 fovy;
    /* +0x38 */ u8 unused_0x38[0x8];
    /* +0x40 */ u8 active;                  /* the camera drives the view this frame */
    /* +0x41 */ u8 flags;
    /* +0x42 */ u8 step;
    /* +0x43 */ u8 sub_step;
    /* +0x44 */ s16 timer;
    /* +0x46 */ s16 timer_max;
    /* +0x48 */ f32 rate;                   /* 1 / the blend's frame count */
}; /* size: 0x4C */

/* One 0x1C-byte follow-distance record (`camera_range_land`, `camera_range_water` and the single records after them). */
struct CamRange {
    /* +0x00 */ Vec offset;                 /* the camera's offset from the player: y the height, z the distance */
    /* +0x0C */ Vec target_offset;          /* the look-at offset from the player */
    /* +0x18 */ f32 clearance;              /* the least height over the ground under the camera */
}; /* size: 0x1C */

/* The follow camera: the player-tracking view the game uses outside events (camera index 0). */
struct CamFollow : CamView {
    /* +0x04C */ nw4r::math::VEC3 goal_pos;
    /* +0x058 */ nw4r::math::VEC3 goal_target;
    /* +0x064 */ u32 pitch_goal;
    /* +0x068 */ u32 yaw_goal;
    /* +0x06C */ u32 pitch;
    /* +0x070 */ u32 yaw;
    /* +0x074 */ u32 tilt_goal;
    /* +0x078 */ u32 tilt;
    /* +0x07C */ u8 range;                  /* the follow-distance step, 0..4 */
    /* +0x07D */ u8 pad_0x07D[0x3];
    /* +0x080 */ CamRange* range_rec;
    /* +0x084 */ u8 dive_step;
    /* +0x085 */ u8 pad_0x085;
    /* +0x086 */ s16 dive_timer;
    /* +0x088 */ u8 hit_kind;               /* 1 when a wall, 2 when the water surface clipped the camera */
    /* +0x089 */ u8 pad_0x089[0x8];
    /* +0x091 */ u8 wall_side;              /* which way the camera turns off a wall: 1 or 2 */
    /* +0x092 */ u8 reset;                  /* the next update snaps the camera behind the player */
    /* +0x093 */ u8 pad_0x093[0x9];
    /* +0x09C */ s8 lock_state;             /* the lock-on state camera_follow_lock_update derives */
    /* +0x09D */ u8 lock_delay;
    /* +0x09E */ u8 lock_follow;
    /* +0x09F */ u8 lock_turn;
    /* +0x0A0 */ u8 pitch_hold;
    /* +0x0A1 */ u8 yaw_hold;
    /* +0x0A2 */ u8 shake_count;
    /* +0x0A3 */ u8 turn_latch;
    /* +0x0A4 */ u8 jump_latch;
    /* +0x0A5 */ u8 push_count;
    /* +0x0A6 */ u8 push_mode;
    /* +0x0A7 */ u8 pad_0x0A7;
    /* +0x0A8 */ s16 swim_timer;
    /* +0x0AA */ u8 pitch_manual;
    /* +0x0AB */ u8 pitch_manual_timer;
    /* +0x0AC */ u8 pushed;
    /* +0x0AD */ u8 yaw_snap;
    /* +0x0AE */ u16 yaw_snap_goal;
    /* +0x0B0 */ u8 pitch_snap;
    /* +0x0B1 */ u8 pad_0x0B1;
    /* +0x0B2 */ u16 pitch_snap_goal;
    /* +0x0B4 */ u8 push_hold;
    /* +0x0B5 */ u8 moved;
    /* +0x0B6 */ u8 tilt_state;             /* returned by camera_tilt_state_get */
    /* +0x0B7 */ u8 occluded;
    /* +0x0B8 */ nw4r::math::VEC3 shake_pos_peak;
    /* +0x0C4 */ nw4r::math::VEC3 shake_pos;
    /* +0x0D0 */ nw4r::math::VEC3 shake_target_peak;
    /* +0x0DC */ nw4r::math::VEC3 shake_target;
    /* +0x0E8 */ nw4r::math::VEC3 push_pos;
    /* +0x0F4 */ nw4r::math::VEC3 push_target;
    /* +0x100 */ u8 slot_state[2];          /* camera_shake_slot_free_ck tests, camera_shake_slot_arm arms */
    /* +0x102 */ u8 slot_timer[2];
    /* +0x104 */ f32 slot_rate[2];
    /* +0x10C */ u8 slot_armed[2];
    /* +0x10E */ u8 shake_kind;             /* the row of the shake table `camera_shake_table` */
    /* +0x10F */ u8 shake_latched;
    /* +0x110 */ u16 shake_angle;
    /* +0x112 */ u8 pad_0x112[0x2];
    /* +0x114 */ s32 cut_frame;
    /* +0x118 */ s8 cut_state;              /* camera_target_slot_clear sets -1, camera_cut_active_ck tests > 0 */
    /* +0x119 */ u8 pad_0x119;
    /* +0x11A */ u16 cut_yaw;
    /* +0x11C */ u16 cut_pitch;
    /* +0x11E */ u8 pad_0x11E[0x2];
    /* +0x120 */ nw4r::math::VEC3 cut_center;
    /* +0x12C */ nw4r::math::VEC3 cut_pos;
    /* +0x138 */ nw4r::math::VEC3 cut_offset;
    /* +0x144 */ u8 cut_step;
    /* +0x145 */ u8 cut_next;               /* camera_flag_0x145_set raises it */
    /* +0x146 */ u8 pad_0x146[0x2];
}; /* size: 0x148 */

/* The aim camera: the first-person view of the scopes (camera index 1). */
struct CamAim : CamView {
    /* +0x4C */ nw4r::math::MTX34 rot;
    /* +0x7C */ nw4r::math::VEC3 origin;
    /* +0x88 */ f32 fovy_min;
    /* +0x8C */ f32 fovy_max;
    /* +0x90 */ f32 fovy_scale;
    /* +0x94 */ f32 fovy_near;
    /* +0x98 */ f32 fovy_far;
    /* +0x9C */ u32 field_0x9C;
    /* +0xA0 */ u32 field_0xA0;
    /* +0xA4 */ s8 kind;                    /* camera_aim_scope_get's pick, -1 when no scope is up */
    /* +0xA5 */ u8 mode;
    /* +0xA6 */ u8 zoom;
    /* +0xA7 */ u8 pad_0xA7;
}; /* size: 0xA8 */

/* The shoulder camera: the over-the-shoulder view of the lock-on (camera index 2). */
struct CamShoulder : CamView {
    /* +0x4C */ s16 pitch;
    /* +0x4E */ s16 yaw_offset;
    /* +0x50 */ s16 yaw_dir;
    /* +0x52 */ u8 pad_0x52[0x2];
}; /* size: 0x54 */

/* The event camera: the scripted view of demos and kill cuts (camera index 3). */
struct CamEvent : CamView {
    /* +0x04C */ nw4r::math::VEC3 pos_start;
    /* +0x058 */ nw4r::math::VEC3 pos_end;
    /* +0x064 */ nw4r::math::VEC3 target_start;
    /* +0x070 */ nw4r::math::VEC3 target_end;
    /* +0x07C */ union {
        CamScriptOp* script;                /* the next script word */
        CamDcmKey* keys;                    /* the demo-camera key run (`stage_dcm_buffer_tbl`) */
    };
    /* +0x080 */ CamScriptOp* loop;
    /* +0x084 */ u8 interp;
    /* +0x085 */ u8 pos_space;
    /* +0x086 */ u8 target_space;
    /* +0x087 */ u8 pos_arg;
    /* +0x088 */ u8 target_arg;
    /* +0x089 */ u8 field_0x89;
    /* +0x08A */ u8 mode;                   /* the requested event mode */
    /* +0x08B */ s8 result;
    /* +0x08C */ struct _ENEMY_WORK* enemy; /* the monster the event frames */
    /* +0x090 */ f32 field_0x90;
    /* +0x094 */ f32 field_0x94;
    /* +0x098 */ s32 field_0x98;
    /* +0x09C */ s32 field_0x9C;
    /* +0x0A0 */ s32 field_0xA0;
    /* +0x0A4 */ s32 field_0xA4;
    /* +0x0A8 */ u32 roll_start;            /* 65536ths of a turn */
    /* +0x0AC */ u32 roll_end;
    /* +0x0B0 */ u32 fovy_start;
    /* +0x0B4 */ u32 fovy_end;
    /* +0x0B8 */ s32 field_0xB8;
    /* +0x0BC */ s32 field_0xBC;
    /* +0x0C0 */ s16 field_0xC0;
    /* +0x0C2 */ s16 field_0xC2;
    /* +0x0C4 */ s32 field_0xC4;
    /* +0x0C8 */ s32 field_0xC8;
    /* +0x0CC */ s16 field_0xCC;
    /* +0x0CE */ s16 field_0xCE;
    /* +0x0D0 */ s32 field_0xD0;
    /* +0x0D4 */ s32 field_0xD4;
    /* +0x0D8 */ s32 field_0xD8;
    /* +0x0DC */ s32 field_0xDC;
    /* +0x0E0 */ s32 field_0xE0;
    /* +0x0E4 */ s32 field_0xE4;
    /* +0x0E8 */ s32 field_0xE8;
    /* +0x0EC */ s32 field_0xEC;
    /* +0x0F0 */ u8 field_0xF0[4];
    /* +0x0F4 */ CamScriptOp* mark;
    /* +0x0F8 */ f32 field_0xF8;
    /* +0x0FC */ s8 field_0xFC;
    /* +0x0FD */ u8 slot;                   /* the `stage_dcm_buffer_tbl` slot the keys came from */
    /* +0x0FE */ s16 frames;
    /* +0x100 */ s16 length;                /* returned by camera_event_length_get */
    /* +0x102 */ u8 talk_step;
    /* +0x103 */ u8 pad_0x103;
}; /* size: 0x104 */

/* The area camera: the fixed views a map's area records place (camera index 4). */
struct CamArea : CamView {
    /* +0x4C */ u32 field_0x4C;
    /* +0x50 */ u32 field_0x50;
    /* +0x54 */ s8 area_id;
    /* +0x55 */ s8 prev_area_id;
    /* +0x56 */ u8 changed;
    /* +0x57 */ u8 pad_0x57;
    /* +0x58 */ CamAreaRec* rec;
    /* +0x5C */ CamAreaRec* prev_rec;
    /* +0x60 */ f32 distance;
    /* +0x64 */ f32 height;
    /* +0x68 */ f32 pitch;                  /* degrees */
    /* +0x6C */ f32 yaw;                    /* degrees */
    /* +0x70 */ f32 distance_from;
    /* +0x74 */ f32 height_from;
    /* +0x78 */ f32 pitch_from;
    /* +0x7C */ f32 yaw_from;
    /* +0x80 */ u8 reload;
    /* +0x81 */ u8 pad_0x81[0x3];
    /* +0x84 */ nw4r::math::VEC3 rec_pos;   /* the fixed position the record places the camera at */
    /* +0x90 */ nw4r::math::VEC3 field_0x90;
    /* +0x9C */ nw4r::math::VEC3 start_pos; /* where the blend to `rec_pos` starts */
    /* +0xA8 */ nw4r::math::VEC3 field_0xA8;
}; /* size: 0xB4 */

/* The floor quad of an area-camera record: four corners (in order around the quad) and the height of the
 * volume over the first one. */
struct CamAreaQuad {
    /* +0x00 */ nw4r::math::VEC3 corner[4];
    /* +0x30 */ f32 height;
}; /* size: 0x34 */

/* One 0x64-byte area-camera record: the area it belongs to, the camera kind, the floor quad the player must
 * stand in, the four camera parameters, the blend length, the camera position and the clamp box. */
struct CamAreaRec {
    /* +0x00 */ u8 id;
    /* +0x01 */ u8 kind;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ CamAreaQuad quad;
    /* +0x38 */ f32 param[4];               /* distance, height, pitch, yaw */
    /* +0x48 */ u8 blend_frames;
    /* +0x49 */ u8 pad_0x49[0x3];
    /* +0x4C */ Vec pos;
    /* +0x58 */ f32 box_x;
    /* +0x5C */ f32 box_y;
    /* +0x60 */ f32 box_z;
}; /* size: 0x64 */

/* The lobby camera's manual height and distance limits (`camera_lobby_range_default`, `camera_lobby_range_kind2`, `camera_lobby_range_kind1`). */
struct CamLobbyRange {
    /* +0x0 */ f32 height_min;
    /* +0x4 */ f32 height_max;
    /* +0x8 */ f32 distance_min;
    /* +0xC */ f32 distance_max;
}; /* size: 0x10 */

/* One 8-byte entry of a map's area-camera table (`camera_area_sets[mapno]`, indexed by area): the records of the
 * area and how many there are. */
struct CamAreaSet {
    /* +0x0 */ s8 id;                       /* -1 in the first entry when the map has no area cameras */
    /* +0x1 */ u8 count;
    /* +0x2 */ u8 pad_0x2[0x2];
    /* +0x4 */ CamAreaRec* recs;
}; /* size: 0x8 */

/* The lobby camera's goal: where it heads, what it looks at, and how it gets there. */
struct CamLobbyGoal {
    /* +0x00 */ nw4r::math::VEC3 pos;
    /* +0x0C */ nw4r::math::VEC3 target;
    /* +0x18 */ u8 step;                    /* returned by camera_mode_cur_get */
    /* +0x19 */ u8 field_0x19;
    /* +0x1A */ u8 pad_0x1A[0x2];
    /* +0x1C */ _LB_NPC* npc;
    /* +0x20 */ f32 lift;
    /* +0x24 */ f32 height;
    /* +0x28 */ f32 distance;
    /* +0x2C */ f32 fovy;
    /* +0x30 */ s16 blend;
    /* +0x32 */ u8 armed;                   /* camera_lobby_disarm clears it */
    /* +0x33 */ u8 kind;
    /* +0x34 */ s16 swing;
    /* +0x36 */ u8 talk_lock;               /* camera_talk_lock_set stores it */
    /* +0x37 */ u8 manual;                  /* camera_lobby_manual_set stores it */
    /* +0x38 */ f32 tilt;
    /* +0x3C */ nw4r::math::VEC3 look;
}; /* size: 0x48 */

/* The lobby camera (camera index 5). */
struct CamLobby : CamView {
    /* +0x4C */ CamLobbyGoal goal;
    /* +0x94 */ u8 pad_0x94[0x4];
}; /* size: 0x98 */

/* One quake slot: CamWork carries three of them (`set_quake_sub`, `camera_pl_quake2_req`, `camera_shake_req`). */
struct CamQuake {
    /* +0x00 */ nw4r::math::VEC3 origin;
    /* +0x0C */ u8 active;
    /* +0x0D */ u8 kind;                    /* low five bits index the duration and amplitude tables */
    /* +0x0E */ s16 timer;
    /* +0x10 */ u8 level;
    /* +0x11 */ u8 pad_0x11[0x3];
}; /* size: 0x14 */

/* The camera work: the six cameras, the player they follow and the quake slots (two of them at
 * `camera_work`, one per split-screen player; `camera_work_get` returns the current one). */
struct CamWork {
    /* +0x000 */ CamFollow follow;
    /* +0x148 */ CamAim aim;
    /* +0x1F0 */ CamShoulder shoulder;
    /* +0x244 */ CamEvent event;
    /* +0x348 */ CamArea area;
    /* +0x3FC */ CamLobby lobby;
    /* +0x494 */ _PLW* player;
    /* +0x498 */ u8 index;                  /* the split-screen player this work belongs to */
    /* +0x499 */ u8 kind;                   /* the default camera */
    /* +0x49A */ u8 field_0x49A;
    /* +0x49B */ u8 active_kind;            /* the camera driving the view, 0xFF while none */
    /* +0x49C */ u8 invert;                 /* bit 0 inverts the yaw input, bit 1 the pitch input */
    /* +0x49D */ u8 pad_0x49D;
    /* +0x49E */ u16 pad_state[5];          /* the controller words camera_pad_state_get copies from `Psw` */
    /* +0x4A8 */ CamQuake quake[3];
    /* +0x4E4 */ struct _ENEMY_WORK* mode_enemy;
    /* +0x4E8 */ u8 mode_req;               /* the event mode camera_mode_req posts */
    /* +0x4E9 */ u8 pad_0x4E9[0x3];
    /* +0x4EC */ u32 view_pitch;
    /* +0x4F0 */ u32 view_yaw;
    /* +0x4F4 */ u16 area_handle;
    /* +0x4F6 */ u8 lobby_kind;
    /* +0x4F7 */ u8 slot_bits;              /* the camera-slot bit flags (camera_slot_bit_set/70/1CC) */
}; /* size: 0x4F8 */

/* The vector and matrix the C++ camera accessors return: the math types with the copy constructor the
 * original's vector and matrix classes carry, which makes the compiler build the returned local in the
 * caller's return slot (the accessors' manglings carry no return type, so the callers' declarations in
 * `camera/camera.h` name the plain math types). */
struct CamVec3 : nw4r::math::VEC3 {
    CamVec3() {}
    CamVec3(const CamVec3& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
    }
}; /* size: 0xC */

struct CamMtx34 : nw4r::math::MTX34 {
    CamMtx34() {}
    CamMtx34(const CamMtx34& other)
    {
        *(nw4r::math::MTX34*)this = other;
    }
}; /* size: 0x30 */

/* The 0x24-byte demo-camera key camera_dcm_length_get walks; it reads the frame word of each. */
struct CamDcmKey {
    /* +0x00 */ s32 frame;                  /* negative ends the run */
    /* +0x04 */ f32 fovy;
    /* +0x08 */ f32 roll;
    /* +0x0C */ nw4r::math::VEC3 pos;
    /* +0x18 */ nw4r::math::VEC3 target;
}; /* size: 0x24 */

/* One 4-byte entry of the demo-camera table (`camera_dcm_table`): the event mode the keys play for and how they
 * play (bit 0 relative to the monster, bit 1 kept out of the walls, bit 3 holding the last key). */
struct CamDcmEntry {
    /* +0x0 */ u16 mode;                    /* 0xFFFF ends the table */
    /* +0x2 */ u16 flags;
}; /* size: 0x4 */

/* One event-camera script word: the opcode, the word count to the next one, an argument byte and a 16-bit
 * value (sharing the first two bytes), then the opcode's 32-bit data. */
struct CamScriptOp {
    /* +0x0 */ union {
        /* +0x0 */ s16 value;
        /* +0x0 */ struct {
            /* +0x0 */ u8 low;
            /* +0x1 */ u8 arg;
        } bytes; /* size: 0x2 */
    };
    /* +0x2 */ s8 size;
    /* +0x3 */ s8 opcode;
    /* +0x4 */ s32 data[3];
}; /* size: 0x10 (the data words an opcode carries; the shortest word is 4 bytes) */

/* One 8-byte kill-cut shot record (`.sdata` 0x807924B0..0x807924D8): when it starts, how long it runs and
 * its field of view. */
struct CamCutShot {
    /* +0x0 */ s16 start;
    /* +0x2 */ s16 length;
    /* +0x4 */ s16 fovy;
    /* +0x6 */ u8 pad_0x6[0x2];
}; /* size: 0x8 */

/* One 0x18-byte aim-camera eye record per scope (`camera_aim_eye_table`): the eye's offset in the player's frame and
 * the look point's. */
struct CamAimEye {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 height;
    /* +0x08 */ f32 z;
    /* +0x0C */ Vec look;
}; /* size: 0x18 */

/* One 0xC-byte row of the follow camera's shake table (`camera_shake_table`). */
struct CamShake {
    /* +0x0 */ u8 count;                    /* how many frames the shake builds up */
    /* +0x1 */ u8 frames;                   /* how many frames it decays over */
    /* +0x2 */ u8 pad_0x2[0x2];
    /* +0x4 */ f32 pos_scale;
    /* +0x8 */ f32 target_scale;
}; /* size: 0xC */

/* The camera's view of a pad record's control words from +0xD6 (`Psw`). */
struct CamPadControls {
    /* +0x00 */ u16 stick_x;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ u16 stick_y;
    /* +0x06 */ u8 pad_0x06[0x2];
    /* +0x08 */ u16 buttons;
    /* +0x0A */ u8 pad_0x0A[0x8];
    /* +0x12 */ u16 trigger_hold;
    /* +0x14 */ u8 pad_0x14[0x2];
    /* +0x16 */ u16 trigger_press;
}; /* size: 0x18 */

extern "C" {

/* Defined later in this file. */
void camera_scene_posture_init(u8 index, s32 camera);
void camera_water_cell_update(CamWork* work);
void camera_follow_reset(CamWork* work);
void camera_follow_update(CamWork* work);
void camera_follow_yaw_input(CamWork* work, CamFollow* cam, _PLW* player);
s32 camera_lock_drift(u32 angle, f32 low, f32 high, f32 scale);
void camera_follow_pitch_input(CamWork* work, CamFollow* cam, _PLW* player);
void camera_pad_state_clear(CamWork* work);
u32 camera_follow_wall_dist(CamFollow* cam, nw4r::math::VEC3* pos, u16 yaw, f32* dist);
u8 camera_follow_wall_ck(CamFollow* cam, nw4r::math::VEC3* pos, u16 yaw);
void camera_follow_collide(CamWork* work, CamFollow* cam, _PLW* player);
u32 camera_follow_dive_ck(CamFollow* cam);
u32 camera_player_surface_ck(_PLW* player);
u32 camera_player_climb_ck(_PLW* player, u8 kind);
u32 camera_follow_shake_ck(CamFollow* cam, _PLW* player, u8 kind);
u32 camera_follow_weapon_shake_ck(CamFollow* cam, _PLW* player, u8 kind);
bool camera_shake_slot_free_ck(CamFollow* cam, u8 index);
void camera_shake_slot_arm(CamFollow* cam, u8 index, u8 frames);
void camera_follow_shake_update(CamFollow* cam, _PLW* player);
void camera_follow_push_apply(CamFollow* cam);
void camera_follow_lock_update(CamFollow* cam, _PLW* player);
u8 camera_lock_state_get(void);
void camera_follow_snap(CamFollow* cam, _PLW* player);
void camera_follow_kill_cut(CamFollow* cam, _PLW* player);
f32 camera_blend_ease(s16 timer, f32 rate, f32 power);
f32 camera_blend_mix(f32 a, f32 b, f32 t);
s32 camera_angle_approach(u32 a, u32 b, u8 shift);
bool camera_cut_active_ck(void);
bool camera_area_active_ck(void);
u32 camera_underwater_ck(void);
u32 camera_input_ok_ck(_PLW* player);
u8 camera_water_clamp(CamView* cam, nw4r::math::VEC3* target);
void camera_pad_state_get(CamWork* work);
void camera_aim_reset(CamWork* work);
void camera_aim_update(CamWork* work);
void camera_shoulder_update(CamWork* work);
void camera_area_cam_reset(CamWork* work);
void camera_area_cam_update(CamWork* work);
void camera_lobby_reset(CamWork* work);
void camera_lobby_frame(CamWork* work);
void camera_lobby_goal_set(_LB_NPC* npc, nw4r::math::VEC3* look, nw4r::math::VEC3* pos, u8 snap);
void camera_event_reset(CamWork* work);
void camera_event_update(CamWork* work);
void camera_view_pos_get(nw4r::math::VEC3* out, u8 kind);
f32 camera_fovy_get(void);
f32 camera_roll_get(void);
void camera_area_rec_take(CamArea* cam, CamAreaRec* rec);
void camera_area_blend_update(CamWork* work, CamArea* cam);
CamAreaRec* camera_area_rec_find(CamArea* cam, CamAreaRec* rec, u8 count, nw4r::math::VEC3* pos);
void camera_area_param_take(CamArea* cam, const CamAreaRec* rec);
void camera_area_blend_start(CamArea* cam, s32 frames);
void camera_area_blend_ck(CamArea* cam);
void camera_area_orbit_blend(CamWork* work, CamArea* cam);
void camera_area_fixed_blend(CamWork* work, CamArea* cam);
void camera_lobby_enter(void);
void camera_lobby_leave(void);
void camera_talk_reset(void);
bool camera_work_ck(void);
s32 camera_event_script_step(CamWork* work, CamEvent* cam);
void camera_script_pos_read(nw4r::math::VEC3* out, CamEvent* cam, CamScriptOp* op);
void camera_script_target_read(nw4r::math::VEC3* out, CamEvent* cam, CamScriptOp* op);
void camera_event_channel_latch(CamEvent* cam, u32 mask);
void camera_event_path_step(CamWork* work, CamEvent* cam);
void camera_event_enemy_get(CamEvent* cam, u32* angle, nw4r::math::VEC3* pos);
void camera_event_enemy_angles_get(CamEvent* cam, u32* angle_a, u32* angle_b, nw4r::math::VEC3* pos);
bool camera_event_enemy_mode1_ck(CamEvent* cam);
bool camera_event_enemy_mode2_ck(CamEvent* cam);
s16 camera_dcm_length_get(CamEvent* cam);
s32 camera_dcm_step(CamWork* work, CamEvent* cam);
u8 camera_aim_zoom_get(u32 unused, u8* out_mode, u8* out_zoom, f32* out_scale);
u32 camera_mode1_ck(void);
void camera_position_set(nw4r::g3d::Camera* cam, const nw4r::math::VEC3* pos);
s32 camera_angle_y_get(void);
void camera_area_reset(void);
void camera_kill_cut_start(u8 id, struct _ENEMY_WORK* enemy);
void camera_quake_apply(CamWork* work, CamQuake* slot, u8 index);
}

/* The camera's C++ entry points (`camera/camera.h` declares them for the other units). */
CamVec3 get_camera_pos(void);
CamVec3 get_camera_direction(void);
CamMtx34 get_current_view_mtx(void);
void set_quake_sub(u8 kind, nw4r::math::VEC3* origin);

extern "C" {
void camera_mode_req(u8 kind, struct _ENEMY_WORK* enemy);
s32 camera_event_script_run(CamWork* work, CamEvent* cam);
s32 camera_event_dcm_run(CamWork* work, CamEvent* cam);
bool camera_slot_bit_ck(CamWork* work, u8 slot);
bool camera_slot_all_ck(CamWork* work);
void camera_dcm_buffers_clear(void);
void camera_view_apply(u8 index, nw4r::math::VEC3* pos, nw4r::math::VEC3* target, f32 fovy, f32 roll);
void camera_view_angles_update(CamWork* work);
void camera_quake_update(LightWork* self, u8 index);
}

/* Owner headers (rule 2): every symbol a registered unit defines is declared in that unit's header,
 * never here.  `unsplit/unknown.h` carries the module-ambiguous ones. */
#include "ef/fn_800CDB2C.h"
#include "fn_8004CAD8.h"
#include "g3d/g3d_anmscn.h"
#include "lobby/lb_npc.h"
#include "stage/stg_w.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "stage/get_now_mapno.h"
#include "gx.h"
#include "unsplit/ef.h"
#include "unsplit/g3d.h"
#include "Pl/pl_act_no_eft_ck.h"
#include "Pl/pl_act.h"
#include "mh3_pad/system_w.h"
#include "vec3_scale.h"
#include "ef/vec3_div.h"
#include "enemy/enemy_control.h"
#include "Pl/pl_coll.h"
#include "MSL/mathf.h"
#include "lobby/lb_npc.h"
#include "lobby/lb_cmd_held_ck.h"
#include "ef/system_lobby_flag_get.h"
#include "ef/sys_quest_move.h"
#include "lobby/event_demo_running_ck.h"
#include "main.h"
#include "Network/network_pat_control.h"
#include "lobby/lb_server_sel_trans.h"
#include "fn_80056F24.h"

#include "mh3_pad/Psw.h"
#include "ef/get_move_work_adrs.h"

/* The leaf headers of the callees the owners declare nowhere this unit can include (rule 2). */
#include "screen_aspect_get.h"
#include "word_read.h"
#include "scene_camera_get.h"
#include "scene_current_camera_get.h"
#include "scene_current_camera_set.h"
#include "vec3_lerp.h"
#include "g3d/camera_get_position.h"
#include "g3d/camera_posture_get.h"
#include "g3d/camera_set_viewport.h"
#include "ef/system_lobby_room_get.h"
#include "enemy/em_kill_cut_target_get.h"
#include "lobby/lb_screen_rect_get.h"
#include "Pl/pl_now_area_ck.h"
#include "Pl/pl_camera_reset_ck.h"
#include "Pl/pl_target_lock_ck.h"
#include "Pl/pl_camera_close_ck.h"
#include "Pl/pl_eye_mtx_get.h"
#include "Pl/stage_water_surface_get.h"
#include "Pl/coll_line_check.h"
#include "menu/coll_push_out.h"
#include "stage/stage_pos_z_low_ck.h"
#include "stage/stage_area_colour_update.h"
#include "light/light_dir_set.h"
#include "lobby/lb_server_leave_ck.h"
#include "lobby/lb_server_ready_ck.h"
#include "enemy/lobby_net_busy_ck.h"
#include "hud/hud_screen_width_get.h"
#include "lobby/lb_sub1a_send.h"
#include "enemy/em024_action11_state5_ck.h"
#include "quest/quest_move_sub_state_ck.h"
#include "assignVec3.h"
#include "enemy/em_area_ck.h"
#include "hud/cockpit.h"

/* Call views (the `_c1` convention): a callee whose declared type differs from the one the call needs, and the
 * vector helpers that write their result through r3, called as functions returning the vector (the shape of
 * the original's by-value vector operators). */
#define calcVecAngX_c1 ((u32 (*)(nw4r::math::VEC3*))calcVecAngX)
#define camera_target_pos_get ((nw4r::math::VEC3 (*)(void))get_camera_target_pos)
#define vec3_sub ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))subVec3)
#define vec3_add ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))addVec3)
#define cam_vec3_sub ((CamVec3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))subVec3)
#define pl_id_bit_ck_c1 ((u32 (*)(_PLW*, s32))pl_id_bit_ck)
#define pl_status_bit_ck_c1 ((s32 (*)(_PLW*, u32))pl_status_bit_ck)
#define vec3_scale_v ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, f32))vec3_scale)
#define pl_target_lock_ck_c1 ((u32 (*)(_PLW*))pl_target_lock_ck)
#define em_vec3_from ((nw4r::math::VEC3* (*)(nw4r::math::VEC3*, const Vec*))em_vec3_copy)
#define vec3_div_v ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, f32))vec3_div)
#define pl_act_true_ck_c1 ((u32 (*)(_PLW*))pl_act_true_ck)
#define camera_view_pos ((nw4r::math::VEC3 (*)(u8))camera_view_pos_get)
#define event_demo_running_ck_c1 ((u8 (*)(void))event_demo_running_ck)

/* The unit's own tables (`.data`, `.sdata`, `.bss`): declared until the source emits them (RESIDUALS). */
extern CamRange camera_range_land[6];
extern CamRange camera_range_dive[2];
extern CamRange camera_range_water[6];
extern CamRange camera_range_close[1];
extern CamShake camera_shake_table[];
extern CamCutShot camera_cut_shot_swing;
extern CamCutShot camera_cut_shot7_0;
extern CamCutShot camera_cut_shot7_1;
extern CamCutShot camera_cut_shot4_0;
extern CamCutShot camera_cut_shot4_1;
extern Vec camera_cut_swing_keys[2];
extern Vec camera_cut_offset_init;
extern f32 camera_cut7_pos_keys0[];
extern f32 camera_cut7_target_keys0[];
extern f32 camera_cut7_pos_keys1[];
extern f32 camera_cut7_target_keys1[];
extern f32 camera_cut4_pos_keys0[];
extern f32 camera_cut4_target_keys0[];
extern f32 camera_cut4_pos_keys1[];
extern f32 camera_cut4_target_keys1[];
extern CamAimEye camera_aim_eye_table[3];
extern Vec camera_shoulder_eye;
extern Vec camera_shoulder_look;
extern CamAreaSet* camera_area_sets[];
extern CamLobbyRange camera_lobby_range_default;
extern CamLobbyRange camera_lobby_range_kind2;
extern CamLobbyRange camera_lobby_range_kind1;
extern f32 camera_lobby_room_distance[];
extern Vec* camera_lobby_views[];
extern CamScriptOp* camera_event_scripts[];
extern u8 camera_kill_cut_mode[];
extern CamDcmKey* stage_dcm_buffer_tbl[50];
extern CamDcmEntry camera_dcm_table[];
extern const s16 camera_quake_frames[];
extern const s16 camera_quake_amplitude[];
extern f32 camera_quake_range[];

/* The pooled string and the entry points whose address owns no registered unit, so no header exists
 * for them. */
extern "C" const char stage_dcm_path_format[];
extern "C" int sprintf(char* buffer, const char* format, ...);

/* The rest of this unit's own functions, declared before their definitions. */
extern "C" LightWork* camera_work_get(void);
extern "C" LightWork* camera_work_ctor(LightWork* self);
extern "C" LightTriple* camera_lobby_goal_ctor(LightTriple* self);
extern "C" LightOctal* camera_area_ctor(LightOctal* self);
extern "C" LightParams* camera_event_ctor(LightParams* self);
extern "C" LightQuad* camera_shoulder_ctor(LightQuad* self);
extern "C" LightQuadMid* camera_aim_ctor(LightQuadMid* self);
extern "C" LightRoot* camera_follow_ctor(LightRoot* self);
extern "C" LightChannel* camera_quake_ctor(LightChannel* self);
extern "C" LightQuadTriple* camera_lobby_ctor(LightQuadTriple* self);

/* The two camera works (`.bss` 0x806BB7E0, 0x9F0 B = 2 x 0x4F8). */
extern LightWork camera_work[2];

#pragma peephole off
#pragma fp_contract off

/* Resets one camera work and runs its six cameras' initialisers. */
extern "C" void camera_work_setup(CamWork* work, u8 index)
{
    work->index = index;
    work->field_0x49A = 0;
    work->kind = 0;
    ((nw4r::g3d::ScnRoot*)pRoot)->SetCurrentCamera(0);
    camera_pad_state_clear(work);
    camera_follow_reset(work);
    camera_aim_reset(work);
    camera_event_reset(work);
    camera_area_cam_reset(work);
    camera_lobby_reset(work);
    work->player = NULL;
    work->quake[2].active = 0;
    work->quake[1].active = 0;
    work->quake[0].active = 0;
    work->view_yaw = 0;
    work->mode_enemy = NULL;
    work->mode_req = 0;
    work->area_handle = 0;
    work->slot_bits = 0;
}

/* Resets both camera works. */
extern "C" void camera_work_init(void)
{
    u8 index;

    index = 0;
    do {
        camera_work_setup((CamWork*)&camera_work[index], index);
        index++;
    } while (index < 2U);
    camera_dcm_buffers_clear();
}

/* Runs one camera work's per-frame update and picks the camera that drives the view. */
extern "C" void camera_work_update(CamWork* work)
{
    nw4r::math::VEC3 light;
    u32 move;
    u8 kind;

    VEC3_ctor(&light);
    move = (u32)get_move_work_adrs(2);
    work->player = (_PLW*)(move + (s8)my_player_no() * 0xB20);
    camera_pad_state_get(work);
    camera_follow_update(work);
    camera_aim_update(work);
    camera_shoulder_update(work);
    camera_event_update(work);
    camera_area_cam_update(work);
    camera_lobby_frame(work);
    work->active_kind = 0xFFU;
    if (work->event.active == 1) {
        work->active_kind = 3U;
    } else if (work->aim.active == 1) {
        work->active_kind = 1U;
    } else if (work->shoulder.active == 1) {
        work->active_kind = 2U;
    } else if (work->lobby.active == 1) {
        work->active_kind = 5U;
    } else if (work->area.active == 1) {
        work->active_kind = 4U;
    }
    kind = work->active_kind;
    if (kind == 0xFF) {
        kind = work->kind;
    }
    scene_current_camera_set(kind);
    stage_area_colour_update();
    camera_quake_update((LightWork*)work, kind);
    camera_view_angles_update(work);
    copyVec3(&light, &vec3_sub(&get_camera_pos(), &camera_target_pos_get()));
    if (screen_split_mode_ck() != 0) {
        setVector3(&light, 0.0f, 0.5f, -1.0f);
    }
    light_dir_set(&light);
    camera_water_cell_update(work);
}

/* Updates the first camera work and, in split screen, the second player's too. */
extern "C" void camera_update(void)
{
    camera_work_update((CamWork*)&camera_work[0]);
    if (screen_split_mode_ck() != 0) {
        my_player_no_set(1);
        camera_work_update((CamWork*)&camera_work[1]);
        my_player_no_set(0);
    }
}

/* Builds the posture record of the scene root's indexed camera. */
extern "C" void camera_scene_posture_init(u8 index, s32 camera)
{
    nw4r::g3d::Camera cam;
    s32 handle;
    nw4r::g3d::Camera::PostureInfo info;

    handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(index);
    word_copy_return_dst(&cam, &handle);
    camera_posture_info_ctor(&info);
    info.mType = camera;
}

/* Looks up the map cell under the camera and forgets the cached one when there is none. */
extern "C" void camera_water_cell_update(CamWork* work)
{
    nw4r::math::VEC3 poly;
    nw4r::math::VEC3 cam;
    nw4r::math::VEC3 out;

    VEC3_ctor(&poly);
    copyVec3(&poly, &::get_camera_pos());
    if (stage_water_surface_get(&poly, ::get_now_areano(), &work->area_handle, (f32*)&out, 0xFFFF) == 0) {
        work->area_handle = 0;
    }
}

/* Places the follow camera behind the player and clears every follow state. */
extern "C" void camera_follow_reset(CamWork* work)
{
    CamFollow* cam = &work->follow;
    nw4r::math::VEC3 poly;
    nw4r::math::VEC3 world;
    u8 mode;

    VEC3_ctor(&poly);
    setVector3(&cam->pos, 0.0f, 0.0f, 100.0f);
    setVector3(&cam->target, 0.0f, 0.0f, 0.0f);
    cam->fovy = 55.0f;
    cam->roll = 0.0f;
    camera_scene_posture_init(0, 2);
    camera_view_apply(0, &cam->pos, &cam->target, cam->fovy, cam->roll);
    subVec3(&world, &cam->pos, &cam->target);
    copyVec3(&poly, &world);
    calcVecAngXY(&poly, &cam->pitch_goal, &cam->yaw_goal);
    cam->pitch = cam->pitch_goal;
    cam->yaw = cam->yaw_goal;
    mode = get_cfg(work->index, 3);
    switch (mode) {
    case 0:
        cam->range = 0;
        break;
    case 1:
        cam->range = 1;
        break;
    case 3:
        cam->range = 3;
        break;
    case 4:
        cam->range = 4;
        break;
    default:
        cam->range = 2;
        break;
    }
    cam->reset = 1;
    cam->range_rec = NULL;
    cam->tilt_goal = 0;
    cam->tilt = 0;
    setVector3(&cam->shake_pos_peak, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->shake_pos, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->shake_target_peak, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->shake_target, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->push_pos, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->push_target, 0.0f, 0.0f, 0.0f);
    cam->pitch_hold = 0;
    cam->yaw_hold = 0;
    cam->shake_count = 0;
    cam->turn_latch = 0;
    cam->push_count = 0;
    cam->push_mode = 0;
    cam->jump_latch = 0;
    cam->swim_timer = 0;
    cam->pitch_manual = 0;
    cam->pitch_manual_timer = 0;
    cam->yaw_snap = 0;
    cam->yaw_snap_goal = 0;
    cam->pitch_snap = 0;
    cam->pitch_snap_goal = 0;
    cam->pushed = 0;
    cam->moved = 0;
    cam->push_hold = 0;
    cam->slot_state[1] = 0;
    cam->slot_state[0] = 0;
    cam->slot_timer[1] = 0;
    cam->slot_timer[0] = 0;
    cam->slot_armed[1] = 0;
    cam->slot_armed[0] = 0;
    cam->shake_latched = 0;
    cam->shake_kind = 0;
    cam->shake_angle = 0;
    cam->lock_follow = 0;
    cam->lock_turn = 0;
    cam->tilt_state = 0;
    cam->occluded = 0;
    cam->dive_step = 0;
    cam->cut_frame = 0;
    cam->cut_state = 0;
    cam->cut_yaw = 0;
    cam->cut_pitch = 0;
    cam->cut_step = 0;
    cam->cut_next = 0;
}

/* Runs the follow camera for one frame: the lock-on and input turns, the distance record, the swim and
 * shake offsets, the collision pass and the blend from the previous view. */
extern "C" void camera_follow_update(CamWork* work)
{
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 player_pos;
    nw4r::math::VEC3 unused;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 view_pos;
    nw4r::math::VEC3 view_target;
    nw4r::math::VEC3 diff;
    _PLW* player;
    f32 target_lag;
    f32 pos_lag;
    u8 shift;

    player = work->player;
    VEC3_ctor(&offset);
    VEC3_ctor(&player_pos);
    VEC3_ctor(&unused);
    VEC3_ctor(&dir);
    VEC3_ctor(&view_pos);
    VEC3_ctor(&view_target);
    if (player->slot_active != 0 && GameMode_ck() != 2) {
        camera_follow_lock_update(&work->follow, player);
        work->invert = 0;
        switch (get_cfg(player->chunk_ofs, 8)) {
        case 1:
            work->invert |= 2;
            break;
        case 2:
            work->invert |= 1;
            break;
        case 3:
            work->invert |= 2;
            work->invert |= 1;
            break;
        }
        if (work->active_kind != 0xFF && work->active_kind != 2 && work->active_kind != 3) {
            u32 yaw;

            copyVec3(&view_pos, &get_camera_pos());
            copyVec3(&view_target, &camera_target_pos_get());
            yaw = calcVecAng2(&view_target, &view_pos);
            work->follow.yaw = yaw;
            work->follow.yaw_goal = yaw;
        }
        if (work->follow.reset != 0)
            camera_follow_snap(&work->follow, player);
        copyVec3(&player_pos, &player->vec_0x03C);
        if (camera_follow_dive_ck(&work->follow) == 1) {
            work->follow.range_rec = camera_range_dive;
        } else {
            camera_follow_pitch_input(work, &work->follow, player);
            if (pl_camera_close_ck(player) == 1) {
                work->follow.range_rec = camera_range_close;
            } else if (player->kind_0x09 == 3) {
                work->follow.range_rec = &camera_range_water[work->follow.range];
            } else if (player->kind_0x09 == 1 && work->follow.range == 2) {
                work->follow.range_rec = &camera_range_land[5];
            } else {
                work->follow.range_rec = &camera_range_land[work->follow.range];
            }
        }
        camera_follow_yaw_input(work, &work->follow, player);
        if (work->active_kind == 2 && work->follow.lock_follow == 0) {
            s32 frames;
            CamRange* table;
            u8 pick;
            f32 y;
            f32 best;
            f32 d;

            frames = 20;
            work->follow.step = 1;
            work->follow.timer = frames - 1;
            work->follow.rate = 1.0f / (f32)frames;
            VEC3_ctor(&diff);
            copyVec3(&diff, &vec3_sub(&work->shoulder.pos, &player_pos));
            table = camera_range_land;
            best = 100000000.0f;
            pick = 0;
            y = diff.y;
            d = table[1].offset.y - y;
            d = d * d;
            if (best > d) {
                pick = 1;
                best = d;
            }
            d = table[2].offset.y - y;
            d = d * d;
            if (best > d) {
                pick = 2;
                best = d;
            }
            d = table[3].offset.y - y;
            d = d * d;
            if (best > d) {
                pick = 3;
                best = d;
            }
            d = table[4].offset.y - y;
            d = d * d;
            if (best > d) {
                pick = 4;
            }
            if (player->kind_0x09 == 3) {
                u32 pitch = calcVecAngX_c1(&diff);

                work->follow.pitch = pitch;
                work->follow.pitch_goal = pitch;
            } else if (get_cfg(player->chunk_ofs, 11) != 0) {
                work->follow.range = pick;
                work->follow.range_rec = &camera_range_land[pick];
            }
        }
        shift = 2;
        work->follow.tilt_state = 0;
        if (get_cfg(player->chunk_ofs, 10) == 1 && work->follow.pitch_snap == 1) {
            shift = 3;
        } else if (camera_player_surface_ck(player) != 0) {
            if (work->follow.pitch_manual == 0 && work->follow.pitch_manual_timer == 0) {
                s32 speed;

                work->follow.tilt_state = 1;
                speed = work->follow.occluded != 0 ? 144 : 48;
                if ((s16)work->follow.pitch_goal < -0x1000) {
                    work->follow.pitch_goal += speed;
                    if ((s16)work->follow.pitch_goal > -0x1000) {
                        work->follow.pitch_goal = 0xF000;
                        work->follow.tilt_state = 2;
                    }
                } else {
                    work->follow.pitch_goal -= speed;
                    if ((s16)work->follow.pitch_goal < -0x1000) {
                        work->follow.pitch_goal = 0xF000;
                        work->follow.tilt_state = 2;
                    }
                }
                shift = 2;
            }
            work->follow.tilt_goal = 0;
        } else {
            if (work->follow.pitch_manual == 0 && work->follow.pitch_manual_timer == 0) {
                s32 speed;

                speed = work->follow.occluded != 0 ? 30 : 10;
                if ((s16)work->follow.pitch_goal > 0 && (s16)work->follow.pitch_goal < 0xA00) {
                    work->follow.pitch_goal -= speed;
                    if ((s16)work->follow.pitch_goal <= 0)
                        work->follow.pitch_goal = 0;
                } else if ((s16)work->follow.pitch_goal < 0 && (s16)work->follow.pitch_goal > -0xA00) {
                    work->follow.pitch_goal += speed;
                    if ((s16)work->follow.pitch_goal >= 0)
                        work->follow.pitch_goal = 0;
                }
                shift = 2;
            }
            if (Pl_act_ck(player, 0, 27) == 0 && Pl_act_ck(player, 1, 18) == 0 && player->field_0x00A != 4)
                work->follow.tilt_goal = 0;
        }
        work->follow.pitch += (u16)camera_angle_approach(work->follow.pitch_goal, work->follow.pitch, shift);
        work->follow.yaw += camera_angle_approach(work->follow.yaw_goal, work->follow.yaw, 2);
        work->follow.tilt += camera_angle_approach(work->follow.tilt_goal, work->follow.tilt, 5);
        if (work->follow.yaw_snap != 0 && (u16)(work->follow.yaw_snap_goal - work->follow.yaw) - 0x100 > 0xFE00U) {
            work->follow.yaw_snap = 0;
            work->follow.yaw_snap_goal = 0;
        }
        if (work->follow.pitch_snap != 0 && (u16)(work->follow.pitch_snap_goal - work->follow.pitch) - 0x100 > 0xFE00U) {
            work->follow.pitch_snap = 0;
            work->follow.pitch_snap_goal = 0;
        }
        offset.x = 0.0f;
        offset.y = 0.0f;
        offset.z = 0.0f;
        work->follow.goal_target.x = player_pos.x;
        work->follow.goal_target.z = player_pos.z;
        if (Pl_act_ck(player, 0, 6) != 0 || Pl_act_ck(player, 0, 7) != 0) {
            offset.y = work->follow.range_rec->offset.y - 100.0f;
            offset.z = work->follow.range_rec->offset.z;
            setVector3(&dir, 0.0f, 0.0f, 60.0f);
            rotVecY(&dir, player->field_0x058);
            copyVec3(&dir, &vec3_sub(&player_pos, &dir));
            work->follow.goal_target.x = dir.x;
            work->follow.goal_target.y = 40.0f + player_pos.y;
            work->follow.goal_target.z = dir.z;
            target_lag = 10.0f;
        } else {
            offset.y = work->follow.range_rec->offset.y;
            if (player->kind_0x09 == 3) {
                if (Pl_act_ck(player, 1, 21) == 1)
                    offset.y = 170.0f;
                else
                    offset.y = 0.0f;
            }
            offset.z = work->follow.range_rec->offset.z;
            if (Pl_act_ck(player, 1, 21) == 1)
                work->follow.goal_target.y = 170.0f + player_pos.y;
            else if (pl_act_no_eft_ck(player, 2) == 1)
                work->follow.goal_target.y = 70.0f + player_pos.y;
            else
                work->follow.goal_target.y = player_pos.y + work->follow.range_rec->target_offset.y;
            target_lag = 6.0f;
        }
        pos_lag = 8.0f;
        if (camera_player_climb_ck(player, 0) == 1) {
            work->follow.yaw_goal = (u16)(player->field_0x058 + 0x8000);
            work->follow.range = 2;
        }
        if (camera_player_climb_ck(player, 1) == 1) {
            pl_get_joint_wpos(player, 1, &player_pos);
            setVector3(&dir, 0.0f, 150.0f, 60.0f);
            rotVecY(&dir, player->field_0x058);
            copyVec3(&work->follow.goal_target, &vec3_add(&player_pos, &dir));
        }
        if (player->kind_0x09 == 3) {
            if (work->follow.swim_timer == 0) {
                work->follow.swim_timer = 1;
                work->follow.pitch = 0xF000;
                if (Pl_act_ck(player, 0, 151) == 1)
                    work->follow.pitch_goal = 0;
                else
                    work->follow.pitch_goal = work->follow.pitch;
            }
            if (Pl_act_ck(player, 1, 20) != 0) {
                if (Pl_frame_check(player, 0, 38.0f, 0.0f) != 0 && (s8)work->follow.pitch_hold == 0)
                    work->follow.pitch_goal = 0;
                work->follow.swim_timer = 5;
            } else if (Pl_act_ck(player, 1, 21) != 0) {
                work->follow.pitch_goal = 0;
                work->follow.swim_timer = 10;
            }
            if (work->follow.swim_timer > 1) {
                work->follow.swim_timer--;
                pos_lag = 3.0f;
                target_lag = 5.0f;
            } else {
                pos_lag = 1.0f;
                target_lag = pos_lag;
            }
            rotVecX(&offset, work->follow.pitch + work->follow.tilt);
        } else if (work->follow.swim_timer > 0) {
            work->follow.swim_timer--;
            pos_lag = 10.0f;
            target_lag = pos_lag;
        }
        rotVecY(&offset, work->follow.yaw);
        copyVec3(&work->follow.goal_pos, &vec3_add(&offset, &player_pos));
        work->follow.push_mode = 0;
        camera_follow_shake_update(&work->follow, player);
        work->follow.target.x = work->follow.goal_target.x;
        if (work->follow.hit_kind == 2)
            work->follow.target.y = work->follow.goal_target.y;
        else
            work->follow.target.y += (1.0f / target_lag) * (work->follow.goal_target.y - work->follow.target.y);
        work->follow.target.z = work->follow.goal_target.z;
        work->follow.pos.x = work->follow.goal_pos.x;
        if (work->follow.hit_kind == 2)
            work->follow.pos.y = work->follow.goal_pos.y;
        else
            work->follow.pos.y += (1.0f / pos_lag) * (work->follow.goal_pos.y - work->follow.pos.y);
        work->follow.pos.z = work->follow.goal_pos.z;
        if (work->follow.push_mode != 0)
            camera_follow_push_apply(&work->follow);
        addVec3To(&work->follow.pos, &work->follow.shake_pos);
        addVec3To(&work->follow.target, &work->follow.shake_target);
        camera_follow_kill_cut(&work->follow, player);
        camera_follow_collide(work, &work->follow, player);
        if (camera_player_climb_ck(player, 3) == 1) {
            copyVec3(&work->follow.pos, &work->follow.prev_pos);
            copyVec3(&work->follow.target, &work->follow.prev_target);
        }
        switch (work->follow.step) {
        case 1:
            copyVec3(&work->follow.prev_pos, &get_camera_pos());
            copyVec3(&work->follow.prev_target, &camera_target_pos_get());
            work->follow.step++;
        case 2: {
            f32 t;

            t = camera_blend_ease(work->follow.timer, work->follow.rate, 2.0f);
            if (--work->follow.timer <= 0)
                work->follow.timer = 0;
            vec3_lerp(&work->follow.pos, &work->follow.prev_pos, &work->follow.pos, t);
            vec3_lerp(&work->follow.target, &work->follow.prev_target, &work->follow.target, t);
            break;
        }
        }
        copyVec3(&work->follow.prev_pos, &work->follow.pos);
        copyVec3(&work->follow.prev_target, &work->follow.target);
        camera_view_apply(0, &work->follow.pos, &work->follow.target, work->follow.fovy, work->follow.roll);
    }
}

/* Turns the follow camera from the player's input and the lock-on: the manual yaw turn, the snap behind the
 * player and the lock-on drift towards the player's facing. */
extern "C" void camera_follow_yaw_input(CamWork* work, CamFollow* cam, _PLW* player)
{
    u32 speed;
    u32 yaw;
    u8 follow;
    u8 turned;

    follow = 0;
    turned = 0;
    if (system_w.field_0x2a != 0)
        return;
    speed = 1035;
    if (cam->hit_kind != 0)
        speed = 843;
    else if (cam->moved == 1)
        speed = 747;
    yaw = cam->yaw_goal;
    if (pl_id_bit_ck_c1(player, 9) == 1 && (pl_id_bit_ck_c1(player, 6) == 1 || pl_id_bit_ck_c1(player, 8) == 1) &&
        camera_input_ok_ck(player) == 1) {
        u32 max;
        u16 step;

        max = player->kind_0x09 == 3 ? 10 : 5;
        if ((s8)cam->yaw_hold < (s32)max)
            cam->yaw_hold++;
        step = (u32)((f32)speed * ((f32)(s8)cam->yaw_hold / (f32)max));
        if ((work->invert & 1) != 0)
            step = -step;
        if (pl_id_bit_ck_c1(player, 6) == 1)
            yaw += step;
        else if (pl_id_bit_ck_c1(player, 8) == 1)
            yaw -= step;
        cam->yaw_goal = yaw;
        cam->wall_side = 0;
    } else {
        if (player->kind_0x09 == 3)
            cam->yaw_hold = 0;
        else if ((s8)cam->yaw_hold > 0)
            cam->yaw_hold--;
        if (camera_follow_dive_ck(cam) == 0 && (s8)cam->lock_state >= 0 && get_cfg(player->chunk_ofs, 11) != 1) {
            u16 diff;
            s32 state;

            follow = 1;
            if (cam->lock_follow == 0 && (s8)cam->lock_state == 1) {
                u16 behind = player->field_0x058 + 0x8000;

                cam->yaw_goal = behind;
                cam->yaw_hold = 10;
                cam->yaw_snap = 1;
                cam->yaw_snap_goal = behind;
            }
            diff = (u16)(((u16)player->field_0x058 ^ 0x8000) - cam->yaw_goal);
            state = (s8)cam->lock_state;
            switch (state) {
            case 0:
            case 2:
            case 3:
                cam->yaw_goal += camera_lock_drift(diff, 4096.0f, 26624.0f, 0.00021701389f);
                if ((u8)(cam->lock_state - 2) <= 1)
                    follow = 0;
                break;
            case 1:
                cam->yaw_goal += camera_lock_drift(diff, 0.0f, 26624.0f, 0.00021701389f);
                break;
            }
        } else if (pl_camera_reset_ck(player, 0) == 1) {
            if (cam->lock_turn == 0) {
                u16 behind = player->field_0x058 + 0x8000;

                cam->yaw_goal = behind;
                cam->yaw_hold = 10;
                cam->yaw_snap = 1;
                cam->yaw_snap_goal = behind;
            }
            cam->yaw_goal += camera_lock_drift((u16)(((u16)player->field_0x058 ^ 0x8000) - cam->yaw_goal), 0.0f, 26624.0f,
                                         0.00021701389f);
        } else if (pl_id_bit_ck_c1(player, 0) == 1 && camera_input_ok_ck(player) == 1) {
            u16 behind = player->field_0x058 + 0x8000;

            cam->yaw_goal = behind;
            cam->yaw_hold = 10;
            cam->yaw_snap = 1;
            cam->yaw_snap_goal = behind;
            turned = 1;
        }
    }
    cam->lock_follow = follow;
    cam->lock_turn = turned;
}

/* The lock-on drift for an angle between `low` and `high` (or their mirror past half a turn): zero at the
 * edges, `scale` times the squared distances from both edges in between. */
extern "C" s32 camera_lock_drift(u32 angle, f32 low, f32 high, f32 scale)
{
    f32 a;
    f32 b;

    if ((f32)angle <= low)
        return 0;
    if (!((f32)angle < high)) {
        if ((f32)angle <= 65536.0f - high)
            return 0;
        if ((f32)angle < 65536.0f - low) {
            angle = 0x10000 - angle;
            scale = -scale;
        } else {
            return 0;
        }
    }
    a = (f32)(s32)angle - low;
    b = high - (f32)(s32)angle;
    a = 1.0f / 65536.0f * (a * a);
    b = 1.0f / 65536.0f * (b * b);
    return (s16)(scale * (a * b));
}

/* Tilts the follow camera from the player's input: the manual pitch while swimming, or the distance step on
 * land. */
extern "C" void camera_follow_pitch_input(CamWork* work, CamFollow* cam, _PLW* player)
{
    if (system_w.field_0x2a == 0 && pl_id_bit_ck_c1(player, 9) != 0 && camera_input_ok_ck(player) != 0) {
        if (player->kind_0x09 == 3) {
            u32 speed;
            u32 pitch;

            speed = 560;
            if (cam->hit_kind == 0 && cam->moved == 1)
                speed = 368;
            pitch = cam->pitch_goal;
            cam->pitch_manual = 0;
            if (pl_id_bit_ck_c1(player, 2) == 1 || pl_id_bit_ck_c1(player, 4) == 1) {
                s32 step;
                s32 up;
                s32 down;

                if ((s8)cam->pitch_hold < 20)
                    cam->pitch_hold++;
                step = (s32)((f32)speed * ((f32)(s8)cam->pitch_hold / 20.0f));
                if ((work->invert & 2) != 0) {
                    up = 4;
                    down = 2;
                } else {
                    up = 2;
                    down = 4;
                }
                if (pl_id_bit_ck_c1(player, up) == 1) {
                    pitch += (u16)step;
                    if ((s16)pitch > 0x3E94)
                        pitch = 0x3E94;
                } else if (pl_id_bit_ck_c1(player, down) == 1) {
                    pitch -= (u16)step;
                    if ((s16)pitch < -0x3E94)
                        pitch = -0x3E94;
                }
                cam->pitch_goal = pitch;
                cam->pitch_manual = 1;
                cam->pitch_manual_timer = 30;
            } else {
                if (pl_id_bit_ck_c1(player, 0) == 1 && pl_act_param_tier_ck(player, 0) == 0 &&
                    (camera_player_surface_ck(player) != 1 || camera_underwater_ck() != 0)) {
                    u16 level = -player->param_0x54;

                    cam->pitch_goal = level;
                    cam->pitch_hold = 10;
                    cam->pitch_snap = 1;
                    cam->pitch_snap_goal = level;
                }
                cam->pitch_hold = 0;
                if (cam->pitch_manual_timer != 0)
                    cam->pitch_manual_timer--;
            }
            cam->range = 2;
            return;
        }
        if ((work->invert & 2) != 0) {
            if (pl_id_bit_ck_c1(player, 3) == 1) {
                if (++cam->range > 4)
                    cam->range = 4;
            } else if (pl_id_bit_ck_c1(player, 1) == 1) {
                if (cam->range != 0)
                    cam->range--;
            }
        } else if (pl_id_bit_ck_c1(player, 1) == 1) {
            if (++cam->range > 4)
                cam->range = 4;
        } else if (pl_id_bit_ck_c1(player, 3) == 1) {
            if (cam->range != 0)
                cam->range--;
        }
        cam->pitch_hold = 0;
        cam->pitch_snap = 0;
    }
}

/* Clears the five controller words the camera carries over from the previous frame. */
extern "C" void camera_pad_state_clear(CamWork* work)
{
    work->pad_state[0] = 0;
    work->pad_state[1] = 0;
    work->pad_state[2] = 0;
    work->pad_state[3] = 0;
    work->pad_state[4] = 0;
}

/* Copies one 16-bit record. */
extern "C" void camera_pad_word_copy(u16* dst, const u16* src)
{
    *dst = *src;
}

/* Copies the player's five camera control words out of the pad record. */
extern "C" void camera_pad_state_get(CamWork* work)
{
    CamPadControls* controls = (CamPadControls*)&Psw[work->index].control_0x0D6;

    camera_pad_word_copy(&work->pad_state[0], &controls->stick_x);
    camera_pad_word_copy(&work->pad_state[1], &controls->stick_y);
    work->pad_state[2] = controls->buttons;
    work->pad_state[3] = controls->trigger_hold;
    work->pad_state[4] = controls->trigger_press;
}

/* Whether the line from `pos` to the point at the follow camera's distance on the `yaw` bearing hits a wall;
 * `dist` gets the distance to the hit, or zero. */
extern "C" u32 camera_follow_wall_dist(CamFollow* cam, nw4r::math::VEC3* pos, u16 yaw, f32* dist)
{
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 probe;
    nw4r::math::VEC3 hit;
    u8 area;

    VEC3_ctor(&offset);
    VEC3_ctor(&probe);
    VEC3_ctor(&hit);
    area = get_now_areano();
    vec_to_mh_vec3(&offset, &cam->range_rec->offset);
    rotVecY(&offset, yaw);
    copyVec3(&probe, &vec3_add(pos, &offset));
    if (coll_line_check(&probe, &cam->target, 28.0f, &hit, 1, 0xFFFF, area, 1) > 0) {
        *dist = vec3_distance(&probe, &hit);
        return 1;
    }
    *dist = 0.0f;
    return 0;
}

/* Whether the line from `pos` to the point at the follow camera's distance on the `yaw` bearing hits a wall. */
extern "C" u8 camera_follow_wall_ck(CamFollow* cam, nw4r::math::VEC3* pos, u16 yaw)
{
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 probe;
    u8 area;

    VEC3_ctor(&offset);
    VEC3_ctor(&probe);
    area = get_now_areano();
    vec_to_mh_vec3(&offset, &cam->range_rec->offset);
    rotVecY(&offset, yaw);
    copyVec3(&probe, &vec3_add(pos, &offset));
    return coll_line_check(&probe, &cam->target, 28.0f, NULL, 1, 0xFFFF, area, 1) > 0;
}

/* Keeps the follow camera out of the walls and the ground: turns it off a wall the player hugs, pushes it out of
 * the collision, clamps it over the ground and pulls the target back when the camera got closer. */
extern "C" void camera_follow_collide(CamWork* work, CamFollow* cam, _PLW* player)
{
    nw4r::math::VEC3 old_pos;
    nw4r::math::VEC3 old_target;
    nw4r::math::VEC3 hit;
    nw4r::math::VEC3 rel;
    nw4r::math::VEC3 before;
    f32 dist_a;
    f32 dist_b;
    f32 old_dist;
    f32 new_dist;
    u8 kind;
    u32 step;
    u32 yaw;
    u8 area;
    u32 pushed;
    u8 turned;

    VEC3_ctor(&old_pos);
    VEC3_ctor(&old_target);
    VEC3_ctor(&hit);
    VEC3_ctor(&rel);
    VEC3_ctor(&before);
    kind = 0;
    step = 0;
    yaw = 0;
    turned = 0;
    copyVec3(&old_pos, &cam->pos);
    copyVec3(&old_target, &cam->target);
    area = get_now_areano();
    if ((pl_status_bit_ck_c1(player, 0x1C0) != 0 || cam->wall_side != 0) && player->kind_0x09 != 3) {
        if (coll_line_check(&old_pos, &old_target, 28.0f, &hit, 1, 0xFFFF, area, 1) > 0 && cam->hit_kind == 0) {
            u32 turn;
            u8 blocked;

            yaw = (u16)cam->yaw;
            if (cam->wall_side == 0) {
                if (camera_follow_wall_dist(cam, &player->vec_0x03C, yaw + 0x80, &dist_a) == 0)
                    cam->wall_side = 1;
                else if (camera_follow_wall_dist(cam, &player->vec_0x03C, yaw - 0x80, &dist_b) == 0)
                    cam->wall_side = 2;
                else if (dist_a > dist_b)
                    cam->wall_side = 1;
                else
                    cam->wall_side = 2;
            }
            blocked = 1;
            turn = 0;
            while (blocked != 0) {
                step += 0x80;
                if (step >= 0x10000)
                    break;
                turn = (u16)-step;
                if (cam->wall_side == 1)
                    turn = (u16)step;
                blocked = camera_follow_wall_ck(cam, &player->vec_0x03C, yaw + turn);
            }
            if (blocked == 0) {
                copyVec3(&rel, &vec3_sub(&cam->pos, &cam->target));
                rotVecY(&rel, (u16)turn);
                copyVec3(&cam->pos, &vec3_add(&cam->target, &rel));
                cam->yaw_goal = (u16)(cam->yaw_goal + (u16)turn);
            }
            turned = 1;
        } else {
            cam->wall_side = 0;
        }
    }
    if (turned == 0) {
        copyVec3(&before, &cam->pos);
        coll_push_out(&cam->pos, 20.0f, 1, 1);
        pushed = 0;
        if (cam->yaw_snap == 1 || (s8)cam->yaw_hold > 0 || (s8)cam->pitch_hold > 0)
            pushed = 1;
        if (pushed == 0 && cam->pushed == 1 && cam->moved == 1)
            cam->push_hold = 1;
        if (vec3_dist_sq(&before, &cam->pos) > 0.0f) {
            cam->moved = 1;
            if (pushed == 0 && cam->push_hold == 0) {
                copyVec3(&rel, &vec3_sub(&cam->pos, &cam->target));
                if (player->kind_0x09 == 3) {
                    u32 pitch;
                    u32 clamped;

                    clamped = 0;
                    rel.y += cam->range_rec->target_offset.y;
                    pitch = calcVecAngX(&rel);
                    cam->pitch_goal = pitch;
                    if ((s16)pitch > 0x3E94) {
                        cam->pitch_goal = 0x3E94;
                        clamped = 1;
                    } else if ((s16)pitch < -0x3E94) {
                        cam->pitch_goal = -0x3E94;
                        clamped = 1;
                    }
                    if (clamped != 0)
                        rotVecX(&rel, (u16)(cam->pitch_goal - pitch));
                }
                rel.y = 0.0f;
                cam->yaw_goal = (u16)calcVecAng3(&rel);
            }
        } else {
            cam->moved = 0;
            cam->push_hold = 0;
        }
        cam->pushed = pushed;
        if (player->kind_0x09 == 3) {
            turned = 2;
            if (coll_line_check(&cam->target, &cam->pos, 25.0f, &hit, 3, -5, area, 1) > 0) {
                cam->pos.x = hit.x;
                cam->pos.y = hit.y;
                cam->pos.z = hit.z;
                kind = 2;
            }
        } else {
            if (coll_line_check(&cam->target, &cam->pos, 25.0f, &hit, 1, 0xFFFF, area, 1) > 0) {
                cam->pos.x = cam->goal_pos.x = hit.x;
                cam->pos.z = cam->goal_pos.z = hit.z;
                kind = 1;
            }
        }
    }
    if (turned != 2) {
        f32 ground_y;
        f32 floor;

        ground_y = GetGroundHit(&cam->pos, -5, area);
        floor = ground_y + cam->range_rec->clearance;
        if (floor > cam->pos.y && 1000.0f > ground_y - cam->pos.y)
            cam->pos.y = cam->goal_pos.y = floor;
    }
    old_dist = vec3_distance(&old_pos, &old_target);
    new_dist = vec3_distance(&cam->pos, &old_target);
    if (old_dist <= 0.00001f) {
        cam->target.x += 0.1f;
        cam->target.z += 0.1f;
    } else if (new_dist <= 0.00001f) {
        cam->target.x += 0.1f;
        cam->target.z += 0.1f;
    } else if (old_dist > new_dist) {
        nw4r::math::VEC3 back;

        old_dist -= new_dist;
        copyVec3(&rel, &vec3_sub(&old_pos, &cam->target));
        if (work->player->kind_0x09 != 3)
            rel.y = 0.0f;
        vec3_normalize_into(&rel, &rel);
        copyVec3(&cam->target, &vec3_sub(&cam->target, &vec3_scale_v(&rel, old_dist)));
    }
    cam->occluded = 0;
    if (camera_water_clamp(cam, &cam->target) != 0)
        cam->occluded = 1;
    cam->hit_kind = kind;
}

/* Tracks the player's dive: 1 when the camera should hold its height while the player sinks, 0 otherwise. */
extern "C" u32 camera_follow_dive_ck(CamFollow* cam)
{
    _PLW* player;
    u32 depth;

    player = (_PLW*)get_move_work_adrs(2);
    player = &player[(s8)my_player_no()];
    depth = 0;
    if (player->kind_0x09 == 2 && pl_act_no_eft_ck(player, 0) == 0) {
        depth = 1;
        if (player->motion_pos_0x40 < player->vec_0x048.y - 10.0f) {
            depth = 2;
            if (player->motion_pos_0x40 > 300.0f + player->ground_y_0x060)
                depth = 3;
        }
    } else if (player->kind_0x09 == 3) {
        cam->dive_step = 0;
        return 0;
    }
    switch (cam->dive_step) {
    case 0:
        if (depth < 3) {
            cam->dive_timer = 4;
            break;
        }
        if (--cam->dive_timer > 0)
            break;
        cam->dive_step++;
    case 1:
        if ((depth & 1) != 0) {
            cam->dive_timer = 15;
            return 1;
        }
        if (--cam->dive_timer > 0)
            return 1;
        cam->dive_step = 0;
        break;
    }
    return 0;
}

/* Whether the player swims near the surface with its swim flag up. */
extern "C" u32 camera_player_surface_ck(_PLW* player)
{
    if (player->kind_0x09 == 3 && player->motion_pos_0x40 > -50.0f && player->field_0x074 != 0)
        return 1;
    return 0;
}

/* Whether the player is in the given special action: 0..2 the three phases of the climb-up, 3 the ledge grab. */
extern "C" u32 camera_player_climb_ck(_PLW* player, u8 kind)
{
    switch (kind) {
    case 0:
        if (Pl_act_ck(player, 6, 62) == 1 && Pl_frame_check(player, 0, 2.0f, 0.0f) == 1)
            return 1;
        break;
    case 1:
        if (Pl_act_ck(player, 6, 62) == 1 && Pl_frame_check(player, 2, 116.0f, 0.0f) == 1)
            return 1;
        break;
    case 2:
        if (Pl_act_ck(player, 6, 62) == 1 && Pl_frame_check(player, 0, 116.0f, 0.0f) == 1)
            return 1;
        break;
    case 3:
        if (Pl_act_ck(player, 6, 54) == 1)
            return 1;
        break;
    }
    return 0;
}

/* Whether the player's current action shakes the follow camera (`kind` 0: the swing shakes, 1: the turn
 * shakes); picks the shake table row on the way. */
extern "C" u32 camera_follow_shake_ck(CamFollow* cam, _PLW* player, u8 kind)
{
    u16 turn;

    if (cam->swim_timer > 1)
        return 0;
    switch (kind) {
    case 0:
        if (cam->hit_kind != 0)
            return 0;
        if (Pl_act_ck(player, 0, 27) != 0) {
            if (player->field_0x018 == 1) {
                if (player->field_0x002 == 3)
                    return 0;
                cam->shake_kind = 2;
            } else {
                cam->shake_kind = 0;
            }
            return 1;
        }
        if (Pl_act_ck(player, 0, 23) != 0 || Pl_act_ck(player, 0, 81) != 0) {
            if (player->field_0x018 == 1)
                cam->shake_kind = 1;
            else
                cam->shake_kind = 0;
            if (Pl_act_ck(player, 0, 81) != 0) {
                cam->shake_count = 0;
                cam->shake_latched = 1;
            }
            if ((s8)cam->shake_count > 0)
                return 1;
            if ((s16)cam->pitch_goal > -0xC00)
                return 1;
            break;
        }
        if (Pl_act_ck(player, 0, 24) != 0 || Pl_act_ck(player, 0, 82) != 0) {
            if (player->field_0x018 == 1)
                cam->shake_kind = 1;
            else
                cam->shake_kind = 0;
            if (Pl_act_ck(player, 0, 82) != 0) {
                cam->shake_count = 0;
                cam->shake_latched = 1;
            }
            if ((s8)cam->shake_count > 0)
                return 1;
            if ((s16)cam->pitch_goal < 0xC00)
                return 1;
            break;
        }
        if (Pl_act_ck(player, 0, 100) != 0) {
            cam->shake_kind = 3;
            cam->push_mode = 2;
            if ((s8)cam->push_count > 0)
                return 1;
            if ((s16)cam->pitch_goal > -0x2000)
                return 1;
            break;
        }
        if (Pl_act_ck(player, 0, 101) != 0) {
            cam->shake_kind = 3;
            cam->push_mode = 2;
            if ((s8)cam->push_count > 0)
                return 1;
            if ((s16)cam->pitch_goal < 0x2000)
                return 1;
        }
        break;
    case 1:
        turn = cam->yaw - player->field_0x058;
        if (Pl_act_ck(player, 0, 27) != 0) {
            if (cam->hit_kind != 0)
                return 0;
            if (player->field_0x018 == 1) {
                if (player->field_0x002 == 3)
                    return 0;
                cam->shake_kind = 5;
            } else {
                cam->shake_kind = 4;
            }
            cam->shake_angle = turn;
            if (cam->turn_latch == 0) {
                if ((u16)(turn + 0xCFFF) <= 0x9FFE)
                    return 1;
                cam->turn_latch = 1;
            }
            break;
        }
        if (Pl_act_ck(player, 0, 25) != 0 || Pl_act_ck(player, 0, 15) != 0 || Pl_act_ck(player, 0, 140) != 0) {
            if (player->kind_0x09 == 3) {
                cam->push_mode = 1;
                cam->shake_kind = 6;
            } else {
                cam->push_mode = 3;
                cam->shake_kind = 7;
            }
            cam->shake_angle = turn + 0x8000;
            if (cam->turn_latch == 0) {
                if ((s8)cam->push_count > 0)
                    return 1;
                if ((u16)(turn + 0x4FFF) > 0x1FFE)
                    return 1;
                cam->turn_latch = 1;
            }
            break;
        }
        if (Pl_act_ck(player, 0, 26) != 0 || Pl_act_ck(player, 0, 16) != 0 || Pl_act_ck(player, 0, 141) != 0) {
            if (player->kind_0x09 == 3) {
                cam->push_mode = 1;
                cam->shake_kind = 6;
            } else {
                cam->push_mode = 3;
                cam->shake_kind = 7;
            }
            cam->shake_angle = turn - 0x8000;
            if (cam->turn_latch == 0) {
                if ((s8)cam->push_count > 0)
                    return 1;
                if ((s16)turn <= 0x3000 || turn >= 0x5000)
                    return 1;
                cam->turn_latch = 1;
            }
            break;
        }
        cam->turn_latch = 0;
        break;
    }
    return 0;
}

/* Whether the player's current weapon action shakes the follow camera (`kind` 0: the gunlance blast, 1: the
 * charge-blade burst). */
extern "C" u32 camera_follow_weapon_shake_ck(CamFollow* cam, _PLW* player, u8 kind)
{
    u16 turn;

    switch (kind) {
    case 0:
        switch (player->field_0x002) {
        case 1:
            if (Pl_act_ck(player, 4, 33) != 0) {
                cam->shake_kind = 8;
                if ((s8)cam->shake_count > 0)
                    return 1;
            } else if (Pl_act_ck(player, 4, 19) != 0) {
                cam->shake_kind = 8;
                if ((s8)cam->shake_count > 0)
                    return 1;
            }
            break;
        }
        break;
    case 1:
        turn = cam->yaw - player->field_0x058;
        switch (player->field_0x002) {
        case 7:
            if ((Pl_act_ck(player, 4, 47) != 0 && Pl_frame_check(player, 1, 20.0f, 0.0f) != 0) ||
                (Pl_act_ck(player, 4, 58) != 0 && Pl_frame_check(player, 1, 20.0f, 0.0f) != 0)) {
                cam->shake_kind = 9;
                if (cam->jump_latch == 0) {
                    if ((s8)cam->shake_count > 0)
                        return 1;
                    if ((u16)(turn + 0xAFFF) <= 0x5FFE)
                        return 1;
                    cam->jump_latch = 1;
                }
            } else {
                cam->jump_latch = 0;
            }
            break;
        }
        break;
    }
    return 0;
}

/* Whether the indexed follow slot is still free. */
extern "C" bool camera_shake_slot_free_ck(CamFollow* cam, u8 index)
{
    return cam->slot_state[index] == 0;
}

/* Arms the indexed follow slot with its frame count. */
extern "C" void camera_shake_slot_arm(CamFollow* cam, u8 index, u8 frames)
{
    cam->slot_state[index] = 1;
    cam->slot_timer[index] = frames;
    cam->slot_armed[index] = 0;
}

/* Builds the follow camera's swing and turn shakes from the shake table row the player's action picks, and
 * runs the two shake slots' decay. */
extern "C" void camera_follow_shake_update(CamFollow* cam, _PLW* player)
{
    nw4r::math::VEC3 pos_rel;
    nw4r::math::VEC3 target_rel;
    u32 turn;
    u32 swing;
    u8 i;

    VEC3_ctor(&pos_rel);
    VEC3_ctor(&target_rel);
    swing = 0;
    turn = 0;
    copyVec3(&pos_rel, &vec3_sub(&cam->pos, &cam->goal_pos));
    copyVec3(&target_rel, &vec3_sub(&cam->target, &cam->goal_target));
    if (camera_follow_shake_ck(cam, player, 0) == 1 || camera_follow_weapon_shake_ck(cam, player, 0) == 1) {
        f32 pos_scale = camera_shake_table[cam->shake_kind].pos_scale;
        f32 target_scale = camera_shake_table[cam->shake_kind].target_scale;

        if (cam->shake_latched == 0) {
            if (cam->push_mode != 0) {
                if ((s8)cam->push_count < camera_shake_table[cam->shake_kind].count) {
                    cam->push_count++;
                    cam->push_pos.y = pos_rel.y * pos_scale;
                    cam->push_target.y = target_rel.y * target_scale;
                }
            } else if ((s8)cam->shake_count < camera_shake_table[cam->shake_kind].count) {
                cam->shake_count++;
                cam->shake_pos.y = pos_rel.y * pos_scale;
                cam->shake_target.y = target_rel.y * target_scale;
                cam->slot_armed[0] = 1;
                if (camera_shake_slot_free_ck(cam, 0) == 0)
                    cam->slot_state[0] = 0;
            } else if (camera_shake_slot_free_ck(cam, 0) == 1) {
                cam->shake_pos_peak.y = pos_rel.y * pos_scale;
                cam->shake_target_peak.y = target_rel.y * target_scale;
                camera_shake_slot_arm(cam, 0, camera_shake_table[cam->shake_kind].frames);
                cam->shake_latched = 1;
            }
        }
        swing = 1;
    }
    if (camera_follow_shake_ck(cam, player, 1) == 1 || camera_follow_weapon_shake_ck(cam, player, 1) == 1) {
        f32 pos_scale;
        f32 target_scale;

        if (Pl_act_ck(player, 0, 27) != 0) {
            f32 t;

            if (cam->shake_angle < 0x8000)
                t = 1.0f - (f32)(s32)(0x8000 - cam->shake_angle) / 20480.0f;
            else
                t = 1.0f - (f32)(s32)(cam->shake_angle - 0x8000) / 20480.0f;
            pos_scale = 0.7f + 0.3f * t;
            target_scale = 0.5f + 0.3f * t;
        } else {
            pos_scale = camera_shake_table[cam->shake_kind].pos_scale;
            target_scale = camera_shake_table[cam->shake_kind].target_scale;
        }
        if (cam->push_mode != 0) {
            if ((s8)cam->push_count < camera_shake_table[cam->shake_kind].count) {
                cam->push_count++;
                cam->push_pos.x = pos_rel.x * pos_scale;
                cam->push_pos.z = pos_rel.z * pos_scale;
                cam->push_target.x = target_rel.x * target_scale;
                cam->push_target.z = target_rel.z * target_scale;
            }
        } else {
            if ((s8)cam->shake_count < camera_shake_table[cam->shake_kind].count) {
                cam->shake_count++;
                cam->shake_pos.x = pos_rel.x * pos_scale;
                cam->shake_pos.z = pos_rel.z * pos_scale;
                cam->shake_target.x = target_rel.x * target_scale;
                cam->shake_target.z = target_rel.z * target_scale;
                cam->slot_armed[1] = 1;
                if (camera_shake_slot_free_ck(cam, 1) == 0)
                    cam->slot_state[1] = 0;
            } else if (camera_shake_slot_free_ck(cam, 1) == 1) {
                cam->shake_pos_peak.x = pos_rel.x * pos_scale;
                cam->shake_pos_peak.z = pos_rel.z * pos_scale;
                cam->shake_target_peak.x = target_rel.x * target_scale;
                cam->shake_target_peak.z = target_rel.z * target_scale;
                camera_shake_slot_arm(cam, 1, camera_shake_table[cam->shake_kind].frames);
            }
        }
        turn = 1;
    }
    if (swing == 0 && cam->slot_armed[0] == 1) {
        cam->shake_pos_peak.y = cam->shake_pos.y;
        cam->shake_target_peak.y = cam->shake_target.y;
        camera_shake_slot_arm(cam, 0, camera_shake_table[cam->shake_kind].frames);
    }
    if (turn == 0 && cam->slot_armed[1] == 1) {
        cam->shake_pos_peak.x = cam->shake_pos.x;
        cam->shake_pos_peak.z = cam->shake_pos.z;
        cam->shake_target_peak.x = cam->shake_target.x;
        cam->shake_target_peak.z = cam->shake_target.z;
        camera_shake_slot_arm(cam, 1, camera_shake_table[cam->shake_kind].frames);
    }
    if (swing == 0 && turn == 0) {
        cam->shake_count = 0;
        cam->shake_latched = 0;
        cam->push_count = 0;
        cam->push_mode = 0;
    }
    if (camera_player_climb_ck(player, 2) == 1) {
        copyVec3(&pos_rel, &vec3_sub(&cam->pos, &cam->goal_pos));
        copyVec3(&target_rel, &vec3_sub(&cam->target, &cam->goal_target));
        pos_rel.y = 0.0f;
        copyVec3(&cam->shake_pos_peak, copyVec3(&cam->shake_pos, &pos_rel));
        copyVec3(&cam->shake_target_peak, copyVec3(&cam->shake_target, &target_rel));
        camera_shake_slot_arm(cam, 0, 100);
        camera_shake_slot_arm(cam, 1, 100);
    }
    for (i = 0; i < 2; i++) {
        f32 t;

        switch (cam->slot_state[i]) {
        case 1:
            cam->slot_rate[i] = 1.0f / (f32)cam->slot_timer[i];
            cam->slot_state[i]++;
        case 2:
            t = camera_blend_ease(cam->slot_timer[i], cam->slot_rate[i], 2.0f);
            cam->slot_timer[i]--;
            if (cam->slot_timer[i] == 0) {
                cam->slot_state[i] = 0;
                cam->slot_timer[i] = 0;
            }
            if (i == 0) {
                cam->shake_pos.y = camera_blend_mix(cam->shake_pos_peak.y, 0.0f, t);
                cam->shake_target.y = camera_blend_mix(cam->shake_target_peak.y, 0.0f, t);
            } else {
                cam->shake_pos.x = camera_blend_mix(cam->shake_pos_peak.x, 0.0f, t);
                cam->shake_pos.z = camera_blend_mix(cam->shake_pos_peak.z, 0.0f, t);
                cam->shake_target.x = camera_blend_mix(cam->shake_target_peak.x, 0.0f, t);
                cam->shake_target.z = camera_blend_mix(cam->shake_target_peak.z, 0.0f, t);
            }
            break;
        }
    }
}

/* Applies the follow camera's push offsets: in push mode 2 it re-aims the pitch from the pushed view, otherwise
 * the yaw, and clears both offsets. */
extern "C" void camera_follow_push_apply(CamFollow* cam)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 target;
    nw4r::math::VEC3 rel;
    u32 pitch;
    u32 yaw;

    VEC3_ctor(&pos);
    VEC3_ctor(&target);
    VEC3_ctor(&rel);
    copyVec3(&pos, &vec3_add(&cam->pos, &cam->push_pos));
    copyVec3(&target, &vec3_add(&cam->target, &cam->push_target));
    if (cam->push_mode == 2) {
        if ((s8)cam->pitch_hold == 0 && (s8)cam->yaw_hold == 0) {
            copyVec3(&rel, &vec3_sub(&pos, &target));
            rel.y += cam->range_rec->target_offset.y;
            calcVecAngXY(&rel, &pitch, &yaw);
            if (cam->tilt != 0)
                cam->pitch += cam->tilt;
            if ((s16)(pitch - (cam->pitch + cam->tilt)) > 0) {
                if ((s16)(pitch + cam->tilt) > 0x3E94)
                    pitch = 0x3E94;
            } else if ((s16)(pitch + cam->tilt) < -0x3E94) {
                pitch = -0x3E94;
            }
            cam->pitch_goal = pitch;
            cam->tilt = 0;
            cam->tilt_goal = 0;
        }
    } else if ((s8)cam->yaw_hold == 0) {
        cam->yaw_goal = calcVecAng2(&target, &pos);
    }
    setVector3(&cam->push_pos, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->push_target, 0.0f, 0.0f, 0.0f);
}

/* Derives the follow camera's lock-on state from the player's lock-on: -1 none, 0/1 on land, 2/3 in water;
 * a lost lock-on lingers for three frames. */
extern "C" void camera_follow_lock_update(CamFollow* cam, _PLW* player)
{
    s32 state;

    state = -1;
    if (pl_target_lock_ck_c1(player) == 1) {
        if (player->kind_0x09 == 3) {
            if (player->field_0x00A == 1)
                state = 2;
            else
                state = 3;
        } else {
            state = (s8)(player->field_0x00A != 1);
        }
    }
    if (state > 0) {
        if ((s8)cam->lock_state < 0) {
            if (state != 0)
                cam->lock_delay--;
            if ((s8)cam->lock_delay > 0)
                state = -1;
        }
    } else {
        cam->lock_delay = 3;
    }
    cam->lock_state = state;
}

/* Reads the current camera work's lock-on state. */
extern "C" u8 camera_lock_state_get(void)
{
    return ((CamWork*)camera_work_get())->follow.lock_state;
}

/* Stops the quakes and resets the event and lobby cameras. */
extern "C" void camera_area_reset(void)
{
    CamWork* work = (CamWork*)camera_work_get();

    work->quake[2].active = 0;
    work->quake[1].active = 0;
    work->quake[0].active = 0;
    camera_event_reset(work);
    camera_lobby_reset(work);
    work->active_kind = 0xFF;
    work->follow.reset = 1;
    work->area.reload = 1;
}

/* Snaps the follow camera behind the player at the distance record of its range. */
extern "C" void camera_follow_snap(CamFollow* cam, _PLW* player)
{
    nw4r::math::VEC3 offset;
    u32 yaw;

    setVec3(&offset, 0.0f, 0.0f, 0.0f);
    yaw = (u16)(player->field_0x058 + 0x8000);
    cam->yaw = yaw;
    cam->yaw_goal = yaw;
    if (player->kind_0x09 == 3) {
        cam->range_rec = &camera_range_water[cam->range];
        cam->dive_step = 0;
        if (player->motion_pos_0x40 > -50.0f && player->field_0x074 != 0) {
            cam->pitch = 0xF000;
            cam->pitch_goal = 0xF000;
        } else {
            cam->pitch = 0;
            cam->pitch_goal = 0;
        }
    } else if (player->kind_0x09 == 1 && cam->range == 2) {
        cam->range_rec = &camera_range_land[5];
    } else {
        cam->range_rec = &camera_range_land[cam->range];
    }
    offset.y = cam->range_rec->offset.y;
    offset.z = cam->range_rec->offset.z;
    rotVecY(&offset, cam->yaw_goal);
    copyVec3(&cam->pos, copyVec3(&cam->goal_pos, &vec3_add(&player->vec_0x03C, &offset)));
    {
        nw4r::math::VEC3 look;

        copyVec3(&cam->target,
                 copyVec3(&cam->goal_target, &vec3_add(&player->vec_0x03C, em_vec3_from(&look, &cam->range_rec->target_offset))));
    }
    setVector3(&cam->shake_pos_peak, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->shake_pos, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->shake_target_peak, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->shake_target, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->push_pos, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->push_target, 0.0f, 0.0f, 0.0f);
    cam->step = 0;
    cam->reset = 0;
}

/* Turns the current camera work's follow camera to the player's flank when the player is the master. */
extern "C" void camera_follow_master_turn(_PLW* player)
{
    if (Pl_master_ck(player) == 1) {
        CamWork* work = (CamWork*)camera_work_get();

        work->follow.yaw_goal = (u16)(player->field_0x058 + 0x2000);
        work->follow.range = 2;
    }
}

/* Reads the current camera work's tilt state. */
extern "C" u8 camera_tilt_state_get(void)
{
    return ((CamWork*)camera_work_get())->follow.tilt_state;
}

/* Runs the kill-cut shot of the follow camera: the monster the player finished picks a keyed shot (or the
 * swing-around for kinds 1 and 2), played from the shot's start frame. */
extern "C" void camera_follow_kill_cut(CamFollow* cam, _PLW* player)
{
    nw4r::math::VEC3 rel;
    nw4r::math::VEC3 pos_key;
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 target_key;
    nw4r::math::VEC3 unused_b;
    nw4r::math::VEC3 center;
    CamCutShot* shot;
    struct _ENEMY_WORK* enemy;
    s32 query;

    VEC3_ctor(&rel);
    VEC3_ctor(&pos_key);
    VEC3_ctor(&unused_a);
    VEC3_ctor(&target_key);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&center);
    enemy = em_kill_cut_target_get(1, player, &query);
    if (enemy == NULL) {
        if (cam->cut_frame > 0)
            cam->cut_frame = 0;
        cam->cut_state = 0;
        cam->cut_yaw = 0x4000;
        cam->cut_pitch = 0xF000;
        vec_to_mh_vec3(&cam->cut_offset, &camera_cut_offset_init);
        cam->fovy = 55.0f;
        cam->cut_step = 0;
        cam->cut_next = 0;
        return;
    }
    switch (enemy->team) {
    case 1:
    case 2:
        shot = &camera_cut_shot_swing;
        break;
    case 7:
        switch (cam->cut_step) {
        case 0:
            if (cam->cut_next == 1) {
                cam->cut_step = 1;
                cam->cut_frame = 0;
                cam->cut_next = 0;
            } else {
                shot = &camera_cut_shot7_0;
                break;
            }
        case 1:
            shot = &camera_cut_shot7_1;
            break;
        }
        break;
    case 4:
        switch (cam->cut_step) {
        case 0:
            if (cam->cut_next == 1) {
                cam->cut_step = 1;
                cam->cut_frame = 0;
                cam->cut_next = 0;
            } else {
                shot = &camera_cut_shot4_0;
                break;
            }
        case 1:
            shot = &camera_cut_shot4_1;
            break;
        }
        break;
    default:
        return;
    }
    if (cam->cut_state < 0) {
        cam->fovy = 55.0f;
        if (cam->cut_state >= -1) {
            u32 yaw;

            cam->cut_frame = 0;
            yaw = enemy->field_0x1C0;
            cam->yaw = yaw;
            cam->yaw_goal = yaw;
            cam->cut_state = -2;
            cam->cut_step = 0;
            cam->cut_next = 0;
        }
        return;
    }
    if (++cam->cut_frame <= shot->start)
        return;
    switch (enemy->team) {
    case 1:
    case 2: {
        f32 t;
        u32 frames;
        s16 step;

        vec_to_mh_vec3(&pos_key, &camera_cut_swing_keys[0]);
        vec_to_mh_vec3(&target_key, &camera_cut_swing_keys[1]);
        t = (f32)(cam->cut_frame - shot->start);
        if (t > (f32)shot->length) {
            t = (f32)shot->length;
            cam->cut_frame = shot->length + shot->start;
        }
        frames = (s32)(80.0f + -40.0f * cosf(90.0f * ((6.2831855f * (t / (f32)shot->length)) / 360.0f)));
        addVec3To(&cam->cut_offset, &vec3_div_v(&vec3_sub(&pos_key, &cam->cut_offset), (f32)(u16)frames));
        copyVec3(&pos_key, &cam->cut_offset);
        cam->range = 3;
        cam->cut_yaw += (s16)((s16)(0xE000 - cam->cut_yaw) / (u16)frames);
        if ((s16)cam->cut_yaw < -0x2000) {
            cam->cut_yaw = 0xE000;
            cam->cut_state = 2;
        }
        step = (s16)(-cam->cut_pitch) / (u16)frames;
        cam->cut_pitch += step;
        if ((s16)cam->cut_pitch > 0)
            cam->cut_pitch = 0;
        if ((u16)(step - 2) > 0xFFFC)
            cam->cut_state = 2;
        rotVecX(&pos_key, cam->cut_pitch);
        rotVecY(&pos_key, enemy->field_0x1C0 + cam->cut_yaw);
        if (cam->cut_state == 0) {
            cam->cut_state++;
            copyVec3(&cam->cut_center,
                     &vec3_add(&player->vec_0x03C, &vec3_sub(&enemy->pos, &player->vec_0x03C)));
        } else if (cam->cut_state == 2) {
            copyVec3(&rel, &vec3_add(&player->vec_0x03C, &vec3_sub(&enemy->pos, &player->vec_0x03C)));
            cam->cut_center.x = rel.x;
            cam->cut_center.z = rel.z;
            copyVec3(&cam->pos, &cam->cut_pos);
            rotVecY(&target_key, enemy->field_0x1C0);
            copyVec3(&cam->target, &vec3_add(&cam->cut_center, &target_key));
            cam->yaw_goal = cam->yaw = calcVecAng2(&cam->target, &cam->pos);
            return;
        }
        copyVec3(&center, &cam->cut_center);
        copyVec3(&cam->pos, copyVec3(&cam->cut_pos, &vec3_add(&center, &pos_key)));
        rotVecY(&target_key, enemy->field_0x1C0);
        copyVec3(&cam->target, &vec3_add(&center, &target_key));
        cam->yaw_goal = cam->yaw = calcVecAng2(&cam->target, &cam->pos);
        break;
    }
    case 4:
    case 7: {
        f32* pos_keys;
        f32* target_keys;
        f32 t;

        if (enemy->team == 4) {
            switch (cam->cut_step) {
            case 0:
                pos_keys = camera_cut4_pos_keys0;
                target_keys = camera_cut4_target_keys0;
                break;
            case 1:
                pos_keys = camera_cut4_pos_keys1;
                target_keys = camera_cut4_target_keys1;
                break;
            }
        } else {
            switch (cam->cut_step) {
            case 0:
                pos_keys = camera_cut7_pos_keys0;
                target_keys = camera_cut7_target_keys0;
                break;
            case 1:
                pos_keys = camera_cut7_pos_keys1;
                target_keys = camera_cut7_target_keys1;
                break;
            }
            cam->range = 2;
        }
        t = (f32)(cam->cut_frame - shot->start);
        if (t > (f32)shot->length) {
            t = (f32)shot->length;
            cam->cut_frame = shot->length + shot->start;
        }
        cam->fovy = (f32)shot->fovy;
        getKeyData3(pos_keys, 2.0f * t, &pos_key.x, &pos_key.y, &pos_key.z);
        getKeyData3(target_keys, 2.0f * t, &target_key.x, &target_key.y, &target_key.z);
        rotVecY(&pos_key, enemy->field_0x1C0);
        rotVecY(&target_key, enemy->field_0x1C0);
        copyVec3(&cam->target, &vec3_add(&enemy->pos, &target_key));
        copyVec3(&cam->pos, &vec3_add(&enemy->pos, &pos_key));
        cam->yaw_goal = cam->yaw = calcVecAng2(&cam->target, &cam->pos);
        break;
    }
    }
}

/* Raises the current camera work's next-cut flag. */
extern "C" void camera_flag_0x145_set(void)
{
    CamWork* work = (CamWork*)camera_work_get();

    work->follow.cut_next = 1;
}

/* Invalidates the current camera work's cut state when `reset` is zero. */
extern "C" void camera_target_slot_clear(u8 reset)
{
    CamWork* work = (CamWork*)camera_work_get();

    if (reset == 0)
        work->follow.cut_state = -1;
}

/* Whether the current camera work's cut state is positive. */
extern "C" bool camera_cut_active_ck(void)
{
    return ((CamWork*)camera_work_get())->follow.cut_state > 0;
}

/* Resets the aim camera: its view, its scope state and its zoom range. */
extern "C" void camera_aim_reset(CamWork* work)
{
    CamAim* cam = &work->aim;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 target;

    copyVec3(&cam->pos, setVec3(&pos, 0.0f, 0.0f, 10.0f));
    copyVec3(&cam->target, setVec3(&target, 0.0f, 0.0f, 0.0f));
    cam->roll = 0.0f;
    cam->fovy = 45.0f;
    camera_scene_posture_init(1, 2);
    camera_view_apply(1, &cam->pos, &cam->target, cam->fovy, cam->roll);
    cam->active = 0;
    cam->flags = 0;
    cam->sub_step = 0;
    cam->step = 0;
    cam->timer_max = 0;
    cam->timer = 0;
    cam->fovy_min = 0.0f;
    cam->fovy_max = 0.0f;
    cam->fovy_scale = 0.0f;
    cam->fovy_near = cam->fovy;
    cam->fovy_far = cam->fovy;
    cam->field_0x9C = 0;
    cam->field_0xA0 = 0;
    cam->kind = 0;
    cam->mode = 0;
    cam->zoom = 0;
}

/* The scope the player has up: 1 the bowgun, 2 the bow, 0 the binoculars, -1 none. */
extern "C" s32 camera_aim_scope_get(_PLW* player)
{
    if (pl_act_no_68_71_ck(player) == 1)
        return 1;
    if (Pl_bari_ck(player, 1) == 1)
        return 2;
    return pl_item_page2_ck(player) == 1 ? 0 : -1;
}

/* Copies the aim camera's mode and zoom bytes out, writes the zoom scaled by the camera's own factor, and
 * returns the scope kind. */
extern "C" u8 camera_aim_zoom_get(u32 unused, u8* out_mode, u8* out_zoom, f32* out_scale)
{
    CamAim* cam = &((CamWork*)camera_work_get())->aim;

    *out_mode = cam->mode;
    *out_zoom = cam->zoom;
    if (cam->zoom != 0) {
        f32 t = cam->fovy - cam->fovy_min;
        *out_scale = t;
        *out_scale = t * cam->fovy_scale;
    }
    return cam->kind;
}

/* Builds a rotation matrix from the pitch and yaw and moves it to `pos`. */
extern "C" void camera_rot_mtx_set(nw4r::math::MTX34* mtx, s32 pitch, u32 yaw, nw4r::math::VEC3* pos)
{
    _CP_VECTOR rot;

    rot.x = pitch;
    rot.y = yaw;
    rot.z = 0;
    cpSetRotMatrix(&rot, mtx);
    mtx->m[0][3] = pos->x;
    mtx->m[1][3] = pos->y;
    mtx->m[2][3] = pos->z;
}

/* Runs the aim camera for one frame: picks the scope, steps its zoom from the player's input and places the
 * view at the scope's eye. */
extern "C" void camera_aim_update(CamWork* work)
{
    CamAim* cam = &work->aim;
    _PLW* player;
    nw4r::math::VEC3 eye;

    player = (_PLW*)get_move_work_adrs(2);
    player = &player[(s8)my_player_no()];
    VEC3_ctor(&eye);
    cam->active = 0;
    if (player->slot_active != 0 && GameMode_ck() != 2) {
        if (player->field_0x5E6 == 0) {
            cam->step = 0;
            return;
        }
        cam->kind = camera_aim_scope_get(player);
        if ((s8)cam->kind == -1)
            return;
        switch (cam->step) {
        case 0:
            cam->zoom = 0;
            switch ((s8)cam->kind) {
            case 0:
                cam->mode = 1;
                if (pl_act_true_ck_c1(player) == 1) {
                    cam->zoom = 1;
                    cam->fovy_min = 15.0f;
                    cam->fovy_max = 60.0f;
                    cam->fovy_scale = 1.0f / (cam->fovy_max - cam->fovy_min);
                }
                break;
            case 1:
                cam->mode = 0;
                cam->zoom = 1;
                cam->fovy_min = 15.0f;
                cam->fovy_max = 60.0f;
                cam->fovy_scale = 1.0f / (cam->fovy_max - cam->fovy_min);
                break;
            case 2:
                cam->mode = 17;
                break;
            }
            cam->step++;
        case 1:
            if (cam->zoom != 0) {
                f32 speed = 1.75f;
                f32* fovy;

                if ((s8)cam->kind == 0)
                    fovy = &cam->fovy_near;
                else
                    fovy = &cam->fovy_far;
                if (pl_id_bit_ck_c1(player, 16) == 1) {
                    *fovy -= speed;
                    if (*fovy < cam->fovy_min)
                        *fovy = cam->fovy_min;
                } else if (pl_id_bit_ck_c1(player, 17) == 1) {
                    *fovy += speed;
                    if (*fovy > cam->fovy_max)
                        *fovy = cam->fovy_max;
                }
            }
            switch ((s8)cam->kind) {
            case 0:
                copyVec3(&cam->origin, &player->vec_0x03C);
                pl_eye_mtx_get(player, &cam->rot);
                cam->fovy = cam->fovy_near;
                break;
            case 1:
                camera_rot_mtx_set(&cam->rot, -player->aim_pitch_0x5E8, player->field_0x058, &player->vec_0x03C);
                cam->fovy = cam->fovy_far;
                break;
            case 2:
                copyVec3(&eye, &player->vec_0x03C);
                if (player->kind_0x09 == 3)
                    eye.y += 50.0f;
                else
                    eye.y += 150.0f;
                camera_rot_mtx_set(&cam->rot, -player->aim_pitch_0x5E8, player->field_0x058, &eye);
                cam->fovy = cam->fovy_far;
                break;
            }
            break;
        }
        {
            nw4r::math::VEC3 offset;
            nw4r::math::VEC3 origin;
            CamAimEye* row;

            VEC3_ctor(&offset);
            VEC3_ctor(&origin);
            setVector3(&origin, cam->rot.m[0][3], cam->rot.m[1][3], cam->rot.m[2][3]);
            offset.x = camera_aim_eye_table[(s8)cam->kind].x;
            offset.y = 0.0f;
            offset.z = camera_aim_eye_table[(s8)cam->kind].z;
            mulVecMat(&offset, &cam->rot);
            copyVec3(&cam->pos, &vec3_add(&origin, &offset));
            row = &camera_aim_eye_table[(s8)cam->kind];
            cam->pos.y += row->height;
            vec_to_mh_vec3(&offset, &row->look);
            mulVecMat(&offset, &cam->rot);
            copyVec3(&cam->target, &vec3_add(&origin, &offset));
            cam->active = 1;
            camera_water_clamp(cam, NULL);
            camera_view_apply(1, &cam->pos, &cam->target, cam->fovy, cam->roll);
        }
    }
}

/* Runs the shoulder camera for one frame: it follows the lock-on, swings to the side the player turns towards,
 * blends in from the follow camera, keeps out of walls and the ground and puts the target on the player's
 * line of sight. */
extern "C" void camera_shoulder_update(CamWork* work)
{
    nw4r::math::MTX34 rot;
    nw4r::math::VEC3 to;
    nw4r::math::VEC3 from;
    nw4r::math::VEC3 eye;
    nw4r::math::VEC3 hit;
    nw4r::math::VEC3 unused;
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 look;
    nw4r::math::VEC3 key;
    _CP_VECTOR angles;
    nw4r::math::VEC3 plane;
    nw4r::math::VEC3 normal;
    nw4r::math::VEC3 dir;
    _PLW* player;
    CamShoulder* cam;
    u8 area;

    player = (_PLW*)get_move_work_adrs(2);
    player = &player[(s8)my_player_no()];
    VEC3_ctor(&eye);
    area = get_now_areano();
    VEC3_ctor(&hit);
    if (player->slot_active != 0 && GameMode_ck() != 2) {
        u16 diff;
        f32 lift;
        u32 done;

        cam = &work->shoulder;
        switch (cam->step) {
        case 0:
            if (work->follow.lock_follow == 0) {
                cam->active = 0;
                cam->sub_step = 0;
                cam->step = 0;
                return;
            }
            copyVec3(&cam->prev_pos, &work->follow.pos);
            cam->pitch = (s16)((player->gauge_0x64F + 100) * 34 - 0xB00);
            cam->yaw_dir = 0x800;
            cam->yaw_offset = 0;
            cam->step++;
            cam->sub_step = 1;
            {
                s32 frames = 9;

                cam->timer = frames;
                cam->rate = 1.0f / (f32)frames;
            }
            break;
        case 1:
            cam->pitch += (s16)camera_angle_approach((u16)((player->gauge_0x64F + 100) * 34 - 0xB00), cam->pitch, 2);
            if (work->follow.lock_follow == 0) {
                cam->active = 0;
                cam->sub_step = 0;
                cam->step = 0;
                return;
            }
            break;
        }
        cam->active = 1;
        diff = (u16)((player->field_0x058 ^ 0x8000) - (work->follow.yaw + cam->yaw_offset));
        if ((u16)(diff + 0xFEFF) <= 0x76FE)
            cam->yaw_dir = -0x800;
        else if ((u16)(diff + 0x77FF) <= 0x76FE)
            cam->yaw_dir = 0x800;
        if (cam->yaw_dir < 0) {
            if (cam->yaw_offset > -0x800)
                cam->yaw_offset += (s16)(((s16)(-0x800 - cam->yaw_offset) * 6) >> 6);
        } else if (cam->yaw_offset < 0x800) {
            cam->yaw_offset += (s16)(((s16)(0x800 - cam->yaw_offset) * 6) >> 6);
        }
        MTX34_ctor(&rot);
        VEC3_ctor(&unused);
        VEC3_ctor(&offset);
        VEC3_ctor(&look);
        VEC3_ctor(&key);
        done = 0;
        if (player->kind_0x09 == 3)
            lift = 70.0f;
        else
            lift = 170.0f;
        switch (cam->sub_step) {
        case 0:
            vec_to_mh_vec3(&offset, &camera_shoulder_eye);
            vec_to_mh_vec3(&look, &camera_shoulder_look);
            break;
        case 1:
            cam->sub_step++;
        case 2: {
            f32 t;

            t = camera_blend_ease(cam->timer, cam->rate, 2.0f);
            if (--cam->timer <= 0)
                cam->timer = 0;
            if (t >= 1.0f)
                done = 1;
            offset.x = 0.0f;
            offset.y = 0.0f;
            offset.z = work->follow.range_rec->offset.z;
            vec_to_mh_vec3(&key, &camera_shoulder_eye);
            vec3_lerp(&offset, &offset, &key, t);
            look.z = 0.0f;
            look.y = 0.0f;
            look.x = 0.0f;
            vec_to_mh_vec3(&key, &camera_shoulder_look);
            vec3_lerp(&look, &look, &key, t);
            {
                f32 other = 1.0f - t;

                lift = work->follow.range_rec->target_offset.y * t + lift * other;
            }
            break;
        }
        }
        cam->target.x = eye.x = player->vec_0x03C.x;
        cam->target.y = eye.y = player->vec_0x03C.y + lift;
        cam->target.z = eye.z = player->vec_0x03C.z;
        angles.x = cam->pitch;
        angles.y = work->follow.yaw + cam->yaw_offset;
        angles.z = 0;
        cpSetRotMatrix(&angles, &rot);
        mulVecMat(&offset, &rot);
        copyVec3(&cam->pos, &vec3_add(&cam->target, &offset));
        if (coll_line_check(&eye, &cam->pos, 25.0f, &hit, 1, 0xFFFF, area, 1) > 0) {
            cam->pos.x = hit.x;
            cam->pos.z = hit.z;
        }
        {
            f32 ground_y;
            f32 floor;

            ground_y = GetGroundHit(&cam->pos, -5, area);
            floor = ground_y + work->follow.range_rec->clearance;
            if (floor > cam->pos.y && 1000.0f > ground_y - cam->pos.y)
                cam->pos.y = floor;
        }
        camera_water_clamp(cam, NULL);
        angles.y = work->follow.yaw;
        cpSetRotMatrix(&angles, &rot);
        mulVecMat(&look, &rot);
        if (done == 0) {
            f32 num;
            f32 t;

            VEC3_ctor(&from);
            VEC3_ctor(&to);
            VEC3_ctor(&plane);
            VEC3_ctor(&normal);
            VEC3_ctor(&dir);
            plane.x = cam->target.x;
            plane.y = 0.0f;
            plane.z = cam->target.z;
            normal.x = look.x;
            normal.y = 0.0f;
            normal.z = look.z;
            copyVec3(&from, &cam->pos);
            copyVec3(&to, &vec3_add(&cam->target, &look));
            num = normal.z * (plane.z - to.z) + (normal.x * (plane.x - to.x) + normal.y * (plane.y - to.y));
            dir.x = to.x - from.x;
            dir.y = to.y - from.y;
            dir.z = to.z - from.z;
            t = num * (1.0f / (dir.z * normal.z + (dir.x * normal.x + dir.y * normal.y)));
            to.x = to.x + dir.x * t;
            to.y = to.y + dir.y * t;
            to.z = to.z + dir.z * t;
            copyVec3(&cam->target, &to);
        }
        cam->roll = work->follow.roll;
        cam->fovy = work->follow.fovy;
        camera_view_apply(2, &cam->pos, &cam->target, cam->fovy, cam->roll);
    }
}

/* Resets the area camera: its view, its blend and its four parameters. */
extern "C" void camera_area_cam_reset(CamWork* work)
{
    CamArea* cam = &work->area;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 target;

    copyVec3(&cam->pos, setVec3(&pos, 0.0f, 30.0f, 50.0f));
    copyVec3(&cam->target, setVec3(&target, 0.0f, 0.0f, 0.0f));
    cam->roll = 0.0f;
    cam->fovy = 50.0f;
    camera_scene_posture_init(4, 2);
    camera_view_apply(4, &cam->pos, &cam->target, cam->fovy, cam->roll);
    cam->active = 0;
    cam->flags = 0;
    cam->sub_step = 0;
    cam->step = 0;
    cam->timer_max = 0;
    cam->timer = 0;
    cam->field_0x4C = 0;
    cam->field_0x50 = 0;
    cam->distance = 690.0f;
    cam->height = 170.0f;
    cam->pitch = 343.0f;
    cam->yaw = 0.0f;
    cam->distance_from = 0.0f;
    cam->height_from = 0.0f;
    cam->pitch_from = 0.0f;
    cam->yaw_from = 0.0f;
    cam->reload = 0;
    setVector3(&cam->rec_pos, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->field_0x90, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->start_pos, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->field_0xA8, 0.0f, 0.0f, 0.0f);
}

/* Runs the area camera for one frame: finds the area record the player stands in, blends to it and places
 * the view the record's kind asks for. */
extern "C" void camera_area_cam_update(CamWork* work)
{
    CamArea* cam = &work->area;
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 player_pos;
    nw4r::math::VEC3 base;
    nw4r::math::VEC3 rec_pos;
    _PLW* player;

    VEC3_ctor(&offset);
    VEC3_ctor(&player_pos);
    VEC3_ctor(&base);
    VEC3_ctor(&rec_pos);
    player = (_PLW*)get_move_work_adrs(2);
    player = &player[(s8)my_player_no()];
    cam->active = 0;
    cam->changed = 0;
    if (get_now_mapno() != 0xFF) {
        CamAreaRec* recs;
        CamAreaSet* set = camera_area_sets[get_now_mapno()];

        if (set[0].id == -1)
            return;
        recs = set[get_now_areano()].recs;
        cam->rec = camera_area_rec_find(cam, recs, set[get_now_areano()].count, &player->vec_0x03C);
    } else {
        cam->rec = NULL;
    }
    if (cam->rec != NULL) {
        cam->active = 1;
        cam->area_id = cam->rec->id;
        if (cam->reload == 1)
            camera_area_rec_take(cam, cam->rec);
    } else {
        cam->area_id = -1;
        cam->sub_step = 0;
        cam->step = 0;
        return;
    }
    if (cam->area_id != cam->prev_area_id && cam->area_id != -1)
        cam->changed = 1;
    camera_area_blend_update(work, cam);
    copyVec3(&player_pos, &player->vec_0x03C);
    switch (cam->rec->kind) {
    case 0:
        setVector3(&offset, 0.0f, 0.0f, cam->distance);
        rotVecX(&offset, (u16)(s32)(0.5f + (65536.0f * cam->pitch) / 360.0f));
        rotVecY(&offset, (u16)(s32)(0.5f + (65536.0f * cam->yaw) / 360.0f));
        copyVec3(&cam->target, &player_pos);
        cam->target.y += cam->height;
        copyVec3(&cam->pos, &vec3_add(&cam->target, &offset));
        break;
    case 1:
        copyVec3(&cam->target, &player_pos);
        cam->target.y += 170.0f;
        break;
    case 2:
        vec_to_mh_vec3(&rec_pos, &cam->rec->pos);
        copyVec3(&cam->pos, &rec_pos);
        setVector3(&offset, 0.0f, 0.0f, 100.0f);
        rotVecX(&offset, (u16)(s32)(0.5f + (65536.0f * cam->pitch) / 360.0f));
        rotVecY(&offset, (u16)(s32)(0.5f + (65536.0f * cam->yaw) / 360.0f));
        copyVec3(&cam->target, &vec3_sub(&cam->pos, &offset));
        break;
    case 3:
        setVector3(&offset, 0.0f, 0.0f, cam->distance);
        rotVecX(&offset, (u16)(s32)(0.5f + (65536.0f * cam->pitch) / 360.0f));
        rotVecY(&offset, (u16)(s32)(0.5f + (65536.0f * cam->yaw) / 360.0f));
        copyVec3(&base, &player_pos);
        base.y += cam->height;
        if (cam->sub_step == 0) {
            copyVec3(&cam->target, &base);
            cam->sub_step++;
        } else {
            copyVec3(&cam->target, &vec3_add(&cam->prev_target, &vec3_div_v(&vec3_sub(&base, &cam->prev_target), 3.0f)));
        }
        copyVec3(&cam->pos, &vec3_add(&cam->target, &offset));
        if (cam->pos.x < cam->rec->pos.x)
            cam->pos.x = cam->rec->pos.x;
        if (cam->pos.x > cam->rec->pos.x + cam->rec->box_x)
            cam->pos.x = cam->rec->pos.x + cam->rec->box_x;
        if (cam->pos.y > cam->rec->pos.y + cam->rec->box_y)
            cam->pos.y = cam->rec->pos.y + cam->rec->box_y;
        if (cam->pos.z < cam->rec->pos.z)
            cam->pos.z = cam->rec->pos.z;
        if (cam->pos.z > cam->rec->pos.z + cam->rec->box_z)
            cam->pos.z = cam->rec->pos.z + cam->rec->box_z;
        break;
    }
    copyVec3(&cam->prev_pos, &cam->pos);
    copyVec3(&cam->prev_target, &cam->target);
    camera_view_apply(4, &cam->pos, &cam->target, cam->fovy, cam->roll);
}

/* Whether `pos` stands inside the area record's floor quad: 0 inside, 2 outside. */
extern "C" s32 camera_area_quad_ck(CamAreaQuad* quad, nw4r::math::VEC3* pos)
{
    nw4r::math::VEC3 d;
    nw4r::math::VEC3 e;

    VEC3_ctor(&d);
    VEC3_ctor(&e);
    setVector3(&d, 0.0f, 0.0f, 0.0f);
    setVector3(&e, 0.0f, 0.0f, 0.0f);
    if (pos->y < quad->corner[0].y || pos->y >= quad->corner[0].y + quad->height)
        return 2;
    d.x = pos->x - quad->corner[0].x;
    d.z = pos->z - quad->corner[0].z;
    e.x = quad->corner[1].x - quad->corner[0].x;
    e.z = quad->corner[1].z - quad->corner[0].z;
    if (e.z * d.x - d.z * e.x < 0.0f)
        return 2;
    d.x = pos->x - quad->corner[3].x;
    d.z = pos->z - quad->corner[3].z;
    e.x = quad->corner[0].x - quad->corner[3].x;
    e.z = quad->corner[0].z - quad->corner[3].z;
    if (e.z * d.x - d.z * e.x < 0.0f)
        return 2;
    d.x = pos->x - quad->corner[2].x;
    d.z = pos->z - quad->corner[2].z;
    e.x = quad->corner[3].x - quad->corner[2].x;
    e.z = quad->corner[3].z - quad->corner[2].z;
    if (e.z * d.x - d.z * e.x < 0.0f)
        return 2;
    d.x = pos->x - quad->corner[1].x;
    d.z = pos->z - quad->corner[1].z;
    e.x = quad->corner[2].x - quad->corner[1].x;
    e.z = quad->corner[2].z - quad->corner[1].z;
    if (e.z * d.x - d.z * e.x < 0.0f)
        return 2;
    return 0;
}

/* The first of `count` area records whose floor quad holds `pos`; remembers the current area id as the
 * previous one when it finds one. */
extern "C" CamAreaRec* camera_area_rec_find(CamArea* cam, CamAreaRec* rec, u8 count, nw4r::math::VEC3* pos)
{
    s32 i;

    for (i = 0; i < count; i++) {
        if (camera_area_quad_ck(&rec->quad, pos) == 0) {
            cam->prev_area_id = cam->area_id;
            return rec;
        }
        rec++;
    }
    return NULL;
}

/* Takes the area record's four camera parameters. */
extern "C" void camera_area_param_take(CamArea* cam, const CamAreaRec* rec)
{
    cam->distance = rec->param[0];
    cam->height = rec->param[1];
    cam->pitch = rec->param[2];
    cam->yaw = rec->param[3];
}

/* Starts the area camera's blend over `frames` frames. */
extern "C" void camera_area_blend_start(CamArea* cam, s32 frames)
{
    cam->step = 1;
    cam->timer = (s16)(frames - 1);
    cam->rate = 1.0f / (f32)frames;
}

/* Starts the blend to a changed area record, or latches it when a blend is already running. */
extern "C" void camera_area_blend_ck(CamArea* cam)
{
    s32 count;

    if (cam->changed == 0)
        return;
    count = 15;
    if (cam->rec != NULL)
        count = cam->rec->blend_frames;
    if (count < 1) {
        cam->step = 0;
        return;
    }
    if (cam->step == 2) {
        cam->sub_step = 1;
        return;
    }
    camera_area_blend_start(cam, count);
}

/* Blends the area camera's four parameters towards the record's (the orbiting kind 0). */
extern "C" void camera_area_orbit_blend(CamWork* work, CamArea* cam)
{
    f32 t;

    switch (cam->step) {
    case 1:
        cam->distance_from = cam->distance;
        cam->height_from = cam->height;
        cam->pitch_from = cam->pitch;
        if (cam->pitch_from >= 180.0f)
            cam->pitch_from = cam->pitch_from - 360.0f;
        cam->yaw_from = cam->yaw;
        if (cam->yaw_from >= 180.0f)
            cam->yaw_from = cam->yaw_from - 360.0f;
        cam->prev_rec = cam->rec;
        cam->step++;
    case 2:
        if (cam->area_id != -1 && cam->step != 0)
            camera_area_param_take(cam, cam->rec);
        t = camera_blend_ease(cam->timer, cam->rate, 2.0f);
        if (--cam->timer <= 0) {
            cam->step = 0;
            if (cam->sub_step != 0)
                cam->sub_step = 0;
        }
        if (cam->sub_step == 1)
            camera_area_param_take(cam, cam->prev_rec);
        cam->distance = camera_blend_mix(cam->distance_from, cam->distance, t);
        cam->height = camera_blend_mix(cam->height_from, cam->height, t);
        if (cam->pitch >= 180.0f)
            cam->pitch = cam->pitch - 360.0f;
        cam->pitch = camera_blend_mix(cam->pitch_from, cam->pitch, t);
        if (cam->yaw >= 180.0f)
            cam->yaw = cam->yaw - 360.0f;
        cam->yaw = camera_blend_mix(cam->yaw_from, cam->yaw, t);
        if (cam->sub_step == 1) {
            cam->distance_from = cam->distance;
            cam->height_from = cam->height;
            cam->pitch_from = cam->pitch;
            if (cam->pitch_from >= 180.0f)
                cam->pitch_from = cam->pitch_from - 360.0f;
            cam->yaw_from = cam->yaw;
            if (cam->yaw_from >= 180.0f)
                cam->yaw_from = cam->yaw_from - 360.0f;
            cam->prev_rec = cam->rec;
            camera_area_blend_start(cam, cam->rec->blend_frames);
            cam->step = 2;
            cam->sub_step = 0;
        }
        break;
    }
}

/* Blends the area camera from the current view to the record's fixed position (kind 1). */
extern "C" void camera_area_fixed_blend(CamWork* work, CamArea* cam)
{
    nw4r::math::VEC3 rec_pos;
    f32 t;

    VEC3_ctor(&rec_pos);
    switch (cam->step) {
    case 1:
        copyVec3(&cam->start_pos, &camera_view_pos(work->kind));
        copyVec3(&cam->start_pos, &get_camera_pos());
        vec_to_mh_vec3(&rec_pos, &cam->rec->pos);
        copyVec3(&cam->rec_pos, &rec_pos);
        cam->prev_rec = cam->rec;
        cam->fovy = camera_fovy_get();
        cam->step++;
    case 2:
        t = camera_blend_ease(cam->timer, cam->rate, 2.0f);
        if (--cam->timer <= 0) {
            cam->step = 0;
            if (cam->sub_step != 0)
                cam->sub_step = 0;
        }
        vec3_lerp(&cam->pos, &cam->start_pos, &cam->rec_pos, t);
        break;
    }
}

/* Starts the area camera's blend and runs the step its record's kind needs. */
extern "C" void camera_area_blend_update(CamWork* work, CamArea* cam)
{
    if (cam->rec != NULL) {
        camera_area_blend_ck(cam);
        switch (cam->rec->kind) {
        case 0:
            camera_area_orbit_blend(work, cam);
            break;
        case 1:
            camera_area_fixed_blend(work, cam);
            break;
        case 2:
        case 3:
            cam->step = 0;
            break;
        }
    }
}

/* Takes an area record's parameters and position as the area camera's current and starting ones. */
extern "C" void camera_area_rec_take(CamArea* cam, CamAreaRec* rec)
{
    nw4r::math::VEC3 rec_pos;

    VEC3_ctor(&rec_pos);
    cam->distance = cam->distance_from = rec->param[0];
    cam->height = cam->height_from = rec->param[1];
    cam->pitch = cam->pitch_from = rec->param[2];
    cam->yaw = cam->yaw_from = rec->param[3];
    vec_to_mh_vec3(&rec_pos, &rec->pos);
    copyVec3(&cam->rec_pos, copyVec3(&cam->start_pos, &rec_pos));
    cam->step = 0;
    cam->sub_step = 0;
    cam->reload = 0;
    cam->prev_area_id = -1;
}

/* The blend factor for `timer` frames into a blend of rate `rate`: an ease-in-out curve of exponent `power`. */
extern "C" f32 camera_blend_ease(s16 timer, f32 rate, f32 power)
{
    f32 t = (f32)timer * rate;

    if (t < 0.5f)
        return 0.5f * powf(2.0f * t, power);
    return 1.0f - 0.5f * powf(2.0f * (1.0f - t), power);
}

/* Blends two values by `t`.  Retail keeps `a * t` and `b * (1 - t)` as separate multiplies and adds, so
 * this function builds with the contract pass off (playbook 40). */

extern "C" f32 camera_blend_mix(f32 a, f32 b, f32 t)
{
    f32 other = 1.0f - t;

    return a * t + b * other;
}

/* Makes the area camera the default and resets the cameras. */
extern "C" void camera_area_default_set(void)
{
    CamWork* work = (CamWork*)camera_work_get();

    work->kind = 4;
    camera_area_reset();
}

/* Resets the lobby camera: its view and its goal. */
extern "C" void camera_lobby_reset(CamWork* work)
{
    CamLobby* cam = &work->lobby;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 target;
    nw4r::math::VEC3 look;

    copyVec3(&cam->pos, setVec3(&pos, 0.0f, 30.0f, 50.0f));
    copyVec3(&cam->target, setVec3(&target, 0.0f, 0.0f, 0.0f));
    cam->roll = 0.0f;
    cam->fovy = 50.0f;
    camera_scene_posture_init(5, 2);
    camera_view_apply(5, &cam->pos, &cam->target, cam->fovy, cam->roll);
    cam->active = 0;
    cam->flags = 0;
    cam->sub_step = 0;
    cam->step = 0;
    cam->timer_max = 0;
    cam->timer = 0;
    cam->goal.step = 0;
    cam->goal.field_0x19 = 0;
    cam->goal.armed = 0;
    cam->goal.kind = 0;
    cam->goal.swing = 0;
    cam->goal.talk_lock = 0;
    copyVec3(&cam->goal.look, setVec3(&look, 0.0f, 0.0f, 0.0f));
    cam->goal.npc = NULL;
    cam->goal.tilt = 0.0f;
}

/* The lobby camera's blend factor for `t` of 20 frames: linear, eased out (curve 1) or eased in (curve 2). */
extern "C" f32 camera_lobby_blend_curve(CamLobbyGoal* goal, s16 t, u8 curve)
{
    f32 f = 0.05f * (f32)t;

    switch (curve) {
    case 1:
        return 1.0f - powf(1.0f - f, 2.2f);
    case 2:
        return powf(f, 1.5f);
    }
    return f;
}

/* Runs the lobby camera for one frame: blends from the area camera towards the goal the lobby set (an NPC,
 * the player under manual control, or a fixed look point). */
extern "C" void camera_lobby_update(CamWork* work, CamLobby* cam)
{
    CamLobbyGoal* goal = &cam->goal;
    nw4r::math::VEC3 rel;
    _PLW* player;
    CamArea* base;
    f32 t;

    VEC3_ctor(&rel);
    player = work->player;
    base = &work->area;
    cam->active = 0;
    switch (cam->goal.step) {
    case 0:
        if (goal->armed != 0) {
            goal->blend = 0;
            goal->step++;
            if (goal->talk_lock != 0)
                goal->swing = 20;
        }
        return;
    case 1:
        if (goal->blend < 20)
            goal->blend++;
        if (goal->armed == 0)
            goal->step++;
        break;
    case 2:
        if (goal->blend > 0)
            goal->blend--;
        if (goal->armed != 0) {
            goal->step = 1;
        } else if (goal->blend == 0) {
            goal->step = 0;
            return;
        }
        break;
    }
    t = camera_lobby_blend_curve(goal, goal->blend, goal->step);
    switch (goal->kind) {
    case 0:
        if (lb_npc_id_ck(goal->npc, 18) == 1 || lb_npc_id_ck(goal->npc, 0) == 1) {
            vec3_lerp(&goal->target, &player->vec_0x03C, &goal->npc->pos_0x10, 0.5f);
            goal->target.y += 64.0f;
        } else {
            vec3_lerp(&goal->target, &player->vec_0x03C, &goal->pos, 0.2f);
            goal->target.y += 100.0f;
            if (goal->talk_lock != 0) {
                if (goal->swing < 20)
                    goal->swing++;
            } else if (goal->swing > 0) {
                goal->swing--;
            }
            if (goal->swing > 0) {
                copyVec3(&rel, &vec3_sub(&goal->pos, &base->pos));
                rotVecY(&rel, -(u16)(s32)(0.5f + (65536.0f * (0.13f * base->fovy)) / 360.0f));
                rel.x = base->pos.x + rel.x;
                rel.y = 100.0f + goal->pos.y;
                rel.z = base->pos.z + rel.z;
                vec3_lerp(&goal->target, &rel, &goal->target,
                            camera_lobby_blend_curve(goal, goal->swing, goal->talk_lock != 0 ? 1 : 2));
            }
        }
        copyVec3(&cam->pos, &base->pos);
        break;
    case 1:
    case 2:
    case 4: {
        CamLobbyRange* range;

        if (work->lobby_kind == 2) {
            range = &camera_lobby_range_kind2;
        } else {
            range = &camera_lobby_range_default;
            if (work->lobby_kind == 1)
                range = &camera_lobby_range_kind1;
        }
        if (goal->manual != 0 && chk_pointer() == 0 && lobby_net_busy_ck() == 0) {
            if (lb_cmd_held_ck(0x100) == 1) {
                goal->distance += 8.0f;
                if (goal->distance > range->distance_max)
                    goal->distance = range->distance_max;
            } else if (lb_cmd_held_ck(0x200) == 1) {
                goal->distance -= 8.0f;
                if (goal->distance < range->distance_min)
                    goal->distance = range->distance_min;
            }
            if ((Psw[0].button_0x2C0.press_0x08 & 0x2000) != 0) {
                goal->height += 2.0f;
                if (goal->height > range->height_max)
                    goal->height = range->height_max;
            } else if ((Psw[0].button_0x2C0.press_0x08 & 0x1000) != 0) {
                goal->height -= 2.0f;
                if (goal->height < range->height_min)
                    goal->height = range->height_min;
            }
        }
        if (goal->kind == 2)
            copyVec3(&goal->target, &goal->look);
        else
            copyVec3(&goal->target, &player->vec_0x03C);
        goal->target.y += goal->height;
        copyVec3(&rel, &vec3_sub(&base->pos, &goal->target));
        if (goal->kind == 4)
            rel.y = 0.0f;
        else
            rel.y += goal->tilt;
        vec3_normalize_into(&rel, &rel);
        vec3_scale_in_place(&rel, goal->distance);
        addVec3To(&rel, &goal->target);
        vec3_lerp(&cam->pos, &rel, &base->pos, t);
        break;
    }
    case 3:
        copyVec3(&goal->target, &goal->look);
        vec3_lerp(&cam->pos, &goal->pos, &base->pos, t);
        break;
    }
    vec3_lerp(&cam->target, &goal->target, &base->target, t);
    cam->pos.x += goal->lift;
    cam->target.x += goal->lift;
    cam->fovy = goal->fovy * t + base->fovy * (1.0f - t);
    cam->roll = base->roll;
    cam->active = 1;
    camera_view_apply(5, &cam->pos, &cam->target, cam->fovy, cam->roll);
}

/* Tracks the lobby kind and re-aims the lobby camera when it changes, then runs it. */
extern "C" void camera_lobby_frame(CamWork* work)
{
    CamLobby* cam = &work->lobby;
    u8 previous = work->lobby_kind;
    u8 kind = system_lobby_flag_get();

    work->lobby_kind = kind;
    if (previous != kind) {
        if (kind != 0)
            camera_lobby_enter();
        else
            camera_lobby_leave();
    }
    camera_lobby_update(work, cam);
}

/* Sends the lobby camera to a goal: an NPC, a look point (with an optional camera position), or the player
 * under manual control; `snap` starts the goal at full blend. */
extern "C" void camera_lobby_goal_set(_LB_NPC* npc, nw4r::math::VEC3* look, nw4r::math::VEC3* pos, u8 snap)
{
    CamWork* work = (CamWork*)camera_work_get();
    CamLobbyGoal* goal = &work->lobby.goal;

    goal->step = 0;
    goal->armed = 1;
    goal->manual = 0;
    goal->talk_lock = 0;
    goal->lift = 0.0f;
    if (npc != NULL) {
        if (snap == 1) {
            goal->step = 1;
            goal->blend = 20;
        }
        goal->kind = 0;
        copyVec3(&goal->pos, &npc->pos_0x10);
        goal->fovy = 20.0f;
        goal->tilt = 0.0f;
    } else if (pos == NULL) {
        goal->height = 120.0f;
        goal->distance = 300.0f;
        goal->fovy = 30.0f;
        goal->tilt = -140.0f;
        if (look != NULL) {
            goal->kind = 2;
            copyVec3(&goal->look, look);
        } else if (snap == 1) {
            goal->kind = 4;
            npc = goal->npc;
            goal->step = 1;
            goal->blend = 20;
            goal->manual = 1;
            if (work->lobby_kind == 2) {
                goal->height = 0.0f;
                goal->distance = camera_lobby_room_distance[system_lobby_room_get()];
            } else {
                goal->height = 84.0f;
                goal->distance = 368.0f;
            }
            goal->tilt = -260.0f;
        } else {
            goal->kind = 1;
        }
    } else {
        if (snap == 1) {
            goal->step = 1;
            goal->blend = 20;
        }
        goal->kind = 3;
        copyVec3(&goal->pos, pos);
        copyVec3(&goal->look, look);
        goal->fovy = camera_fovy_get();
        goal->tilt = 0.0f;
    }
    goal->npc = npc;
}

/* Sends the lobby camera to `npc` with no look point. */
extern "C" void camera_lobby_npc_set(_LB_NPC* npc)
{
    camera_lobby_goal_set(npc, NULL, NULL, 0);
}

/* Disarms the current camera work's lobby goal. */
extern "C" void camera_lobby_disarm(void)
{
    CamWork* work = (CamWork*)camera_work_get();

    work->lobby.goal.armed = 0;
}

/* Stores the lobby goal's talk lock. */
extern "C" void camera_talk_lock_set(u8 value)
{
    ((CamWork*)camera_work_get())->lobby.goal.talk_lock = value;
}

/* Reads the lobby goal's step. */
extern "C" u8 camera_mode_cur_get(void)
{
    return ((CamWork*)camera_work_get())->lobby.goal.step;
}

/* Sends the lobby camera home. */
extern "C" void camera_lobby_menu_enter(void)
{
    camera_lobby_npc_set(NULL);
}

/* Disarms the lobby goal. */
extern "C" void camera_lobby_goal_clear(void)
{
    camera_lobby_disarm();
}

/* Stores the lobby goal's manual flag. */
extern "C" void camera_lobby_manual_set(u8 value)
{
    ((CamWork*)camera_work_get())->lobby.goal.manual = value;
}

/* Sends the lobby camera to the fixed view `index` of the lobby view table. */
extern "C" void camera_lobby_view_set(u8 index, u8 snap)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 look;
    Vec* view;

    VEC3_ctor(&pos);
    VEC3_ctor(&look);
    view = camera_lobby_views[index];
    vec_to_mh_vec3(&pos, &view[0]);
    vec_to_mh_vec3(&look, &view[1]);
    camera_lobby_goal_set(NULL, &look, &pos, snap);
}

/* Disarms the lobby goal. */
extern "C" void camera_lobby_goal_release(void)
{
    camera_lobby_disarm();
}

/* Enters the lobby: resets the lobby g3d camera and puts the lobby camera under manual control. */
extern "C" void camera_lobby_enter(void)
{
    nw4r::g3d::Camera cam;
    s32 handle;

    camera_work_get();
    handle = scene_camera_get(5);
    word_copy_return_dst(&cam, &handle);
    word_read((s32*)&cam);
    camera_lobby_goal_set(NULL, NULL, NULL, 1);
}

/* Leaves the lobby: resets the area and lobby g3d cameras and sends the lobby camera back to its NPC. */
extern "C" void camera_lobby_leave(void)
{
    CamWork* work;
    nw4r::g3d::Camera area_cam;
    nw4r::g3d::Camera lobby_cam;
    s32 area_handle;
    s32 lobby_handle;

    work = (CamWork*)camera_work_get();
    area_handle = scene_camera_get(4);
    word_copy_return_dst(&area_cam, &area_handle);
    word_read((s32*)&area_cam);
    lobby_handle = scene_camera_get(5);
    word_copy_return_dst(&lobby_cam, &lobby_handle);
    word_read((s32*)&lobby_cam);
    camera_lobby_goal_set(work->lobby.goal.npc, NULL, NULL, 1);
}

/* Fits the lobby g3d camera's viewport to the lobby screen window and sets its perspective. */
extern "C" void camera_lobby_viewport_fit(void)
{
    nw4r::g3d::Camera cam;
    s32 handle;
    s16 x;
    s16 y;
    s16 w;
    s16 h;
    f32 aspect;

    camera_work_get();
    handle = scene_camera_get(5);
    word_copy_return_dst(&cam, &handle);
    word_read((s32*)&cam);
    lb_screen_rect_get(&x, &y, &w, &h);
    aspect = (f32)w / (f32)h;
    y = (f32)y * 0.93333334f;
    h = (f32)h * 0.93333334f;
    x = (f32)x * (1.0f / hud_screen_width_get());
    w = (f32)w * (1.0f / hud_screen_width_get());
    camera_set_viewport(&cam, x, y, w, h);
    cam.SetPerspective(30.0f, aspect, 5.0f, 500000.0f);
}

/* Copies the area g3d camera's viewport to the lobby g3d camera and sets its perspective. */
extern "C" void camera_lobby_viewport_copy(void)
{
    nw4r::g3d::Camera area_cam;
    nw4r::g3d::Camera lobby_cam;
    s32 area_handle;
    s32 lobby_handle;
    f32 x;
    f32 y;
    f32 w;
    f32 h;
    f32 near;
    f32 far;

    camera_work_get();
    area_handle = scene_camera_get(4);
    word_copy_return_dst(&area_cam, &area_handle);
    lobby_handle = scene_camera_get(5);
    word_copy_return_dst(&lobby_cam, &lobby_handle);
    camera_get_viewport(&area_cam, &x, &y, &w, &h, &near, &far);
    camera_set_viewport(&lobby_cam, x, y, w, h);
    lobby_cam.SetPerspective(camera_fovy_get(), screen_aspect_get(), 5.0f, 500000.0f);
}

/* Resets the event camera: its view, its request and its four path vectors. */
extern "C" void camera_event_reset(CamWork* work)
{
    CamEvent* cam = &work->event;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 target;

    copyVec3(&cam->pos, setVec3(&pos, 0.0f, 30.0f, 50.0f));
    copyVec3(&cam->target, setVec3(&target, 0.0f, 0.0f, 0.0f));
    cam->roll = 0.0f;
    cam->fovy = 55.0f;
    camera_scene_posture_init(3, 2);
    camera_view_apply(3, &cam->pos, &cam->target, cam->fovy, cam->roll);
    cam->mode = 0;
    cam->result = 0;
    cam->enemy = NULL;
    cam->active = 0;
    cam->flags = 0;
    cam->sub_step = 0;
    cam->step = 0;
    cam->timer_max = 0;
    cam->timer = 0;
    setVector3(&cam->pos_start, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->pos_end, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->target_start, 0.0f, 0.0f, 0.0f);
    setVector3(&cam->target_end, 0.0f, 0.0f, 0.0f);
    cam->talk_step = 0;
}

/* Sets one camera-slot bit: with `set` == 1 the bit for `slot` is raised, otherwise bit 7 is. */
extern "C" void camera_slot_bit_set(u8 slot, u8 set)
{
    CamWork* work = (CamWork*)camera_work_get();

    if (set == 1)
        work->slot_bits |= 1 << slot;
    else
        work->slot_bits |= 0x80;
}

/* Whether the indexed camera-slot bit is set. */
extern "C" bool camera_slot_bit_ck(CamWork* work, u8 slot)
{
    return (work->slot_bits & (1 << slot)) != 0;
}

/* Whether every server slot the camera-slot bits name is ready: the count of flagged slots not in state 4
 * equals the occupied slot count. */
extern "C" u32 camera_slots_ready_ck(CamWork* work)
{
    SysQuestMove* quest;
    s8 count;
    u8 bits;
    s32 i;

    count = 0;
    quest = (SysQuestMove*)get_move_work_adrs(0);
    if (quest == NULL)
        return 0;
    if (isServerSelectState() == 0)
        return 0;
    if (isReadyCountOne() == 0)
        return 0;
    bits = work->slot_bits;
    for (i = 0; i < 4; i++) {
        if ((bits & (1 << i)) != 0 && quest->slot_state_0x22DD[i] != 4)
            count++;
    }
    return countOccupiedServerSlots() == count;
}

/* Whether the high camera-slot bit is set. */
extern "C" bool camera_slot_all_ck(CamWork* work)
{
    return (work->slot_bits & 0x80) != 0;
}

/* Runs the event camera for one frame: the talk handshake of the lobby event demo, then the requested event
 * mode's script or demo-camera keys. */
extern "C" void camera_event_update(CamWork* work)
{
    CamEvent* cam = &work->event;
    _PLW* player;

    player = (_PLW*)get_move_work_adrs(2);
    player = &player[(s8)my_player_no()];
    switch (cam->talk_step) {
    case 0:
        if (pl_id_bit_ck_c1(player, 24) == 1) {
            if (event_demo_ck() == 1) {
                if (lb_server_ready_ck() == 1) {
                    if (camera_slot_bit_ck(work, player->chunk_ofs) == 0) {
                        lb_sub1a_send(player->chunk_ofs, 1);
                        cam->talk_step++;
                    }
                } else if (lb_server_leave_ck() == 1) {
                    camera_talk_reset();
                    fade_set(0, 2);
                }
            } else if (get_now_mapno() != 21 && (cam->flags & 8) == 0) {
                camera_talk_reset();
            }
        }
        break;
    case 1:
        if (camera_slots_ready_ck(work) == 1) {
            lb_sub1a_send(player->chunk_ofs, 2);
            cam->talk_step++;
        }
    case 2:
        if (camera_slot_all_ck(work) == 1) {
            camera_talk_reset();
            fade_set(0, 2);
        }
        break;
    }
    switch (cam->step) {
    case 0:
        if (work->mode_req == 0) {
            cam->active = 0;
            break;
        }
        cam->mode = work->mode_req;
        cam->enemy = work->mode_enemy;
        cam->step++;
        cam->sub_step = 0;
        cam->interp = 1;
        cam->pos_space = 4;
        cam->target_space = 4;
    case 1: {
        s32 result;

        work->mode_req = 0;
        if ((u32)camera_event_scripts[cam->mode] == 0xFFFFFFFF)
            result = camera_event_dcm_run(work, cam);
        else
            result = camera_event_script_run(work, cam);
        if (result <= 0) {
            cam->active = 1;
            if (result == 0)
                cam->result = 1;
            else
                cam->result = -1;
            break;
        }
        cam->step++;
    }
    case 2:
        cam->step = 0;
        cam->sub_step = 0;
        cam->mode = 0;
        cam->result = 0;
        cam->enemy = NULL;
        cam->flags = 0;
        cam->slot = 0;
        cam->active = 0;
        cam->talk_step = 0;
        break;
    }
    camera_view_apply(3, &cam->pos, &cam->target, cam->fovy, cam->roll);
}

/* Clears the event camera's request state. */
extern "C" void camera_talk_reset(void)
{
    CamWork* work = (CamWork*)camera_work_get();

    work->event.step = 0;
    work->event.sub_step = 0;
    work->event.mode = 0;
    work->event.result = 0;
    work->event.enemy = NULL;
    work->event.flags = 0;
    work->event.slot = 0;
    work->event.talk_step = 0;
}

/* Posts the event mode `kind` for `enemy` (or none) unless the quest locks the camera or the monster is out
 * of the area; mode 12 starts with the 0x10 event flag. */
extern "C" void camera_mode_req(u8 kind, struct _ENEMY_WORK* enemy)
{
    CamWork* work = (CamWork*)camera_work_get();
    SysQuestMove* quest = (SysQuestMove*)get_move_work_adrs(0);

    if ((quest == NULL || quest->menu_lock_0x112 != 1) && (enemy == NULL || em_area_ck(enemy) != 0)) {
        work->event.step = 0;
        work->event.sub_step = 0;
        work->mode_req = kind;
        work->mode_enemy = enemy;
        work->event.frames = 0;
        work->event.active = 1;
        work->event.talk_step = 0;
        if (kind == 12)
            work->event.flags = 16;
        else
            work->event.flags = 0;
    }
}

/* Starts the kill cut on `enemy`: the monster's kind picks the event mode (the one kind with two shots picks by
 * its action state). */
extern "C" void camera_kill_cut_start(u8 id, struct _ENEMY_WORK* enemy)
{
    CamWork* work;
    SysQuestMove* quest;
    s16 frames;
    u8 mode;

    work = (CamWork*)camera_work_get();
    quest = (SysQuestMove*)get_move_work_adrs(0);
    frames = 0;
    if ((work->event.flags & 0x20) == 0 && quest_move_sub_state_ck() == 0 &&
        (quest == NULL || quest->menu_lock_0x112 != 1)) {
        system_copy_filter_request();
        if (enemy->team == 24) {
            if (em024_action11_state5_ck(enemy) == 0) {
                mode = 66;
                frames = 119;
            } else {
                mode = 65;
            }
        } else {
            mode = camera_kill_cut_mode[enemy->team];
            if (mode == 0xFF)
                return;
        }
        work->event.step = 0;
        work->event.sub_step = 0;
        work->mode_req = mode;
        work->mode_enemy = enemy;
        work->event.flags = 2;
        work->event.frames = frames;
        work->event.talk_step = 0;
    }
}

/* Requests event mode 2 with a 40 in the event flags. */
extern "C" void camera_mode2_req(void)
{
    CamWork* work = (CamWork*)camera_work_get();

    camera_mode_req(2, 0);
    work->event.flags = 40;
}

/* Requests event mode 3 with an 8 in the event flags. */
extern "C" void camera_mode3_req(void)
{
    CamWork* work = (CamWork*)camera_work_get();

    camera_mode_req(3, 0);
    work->event.flags = 8;
}

/* Requests the map's own event mode for the arena kinds when the player is not in a motion input. */
extern "C" void camera_arena_mode_req(void)
{
    u8 area;
    u8 mode;

    area = get_now_areano();
    if (Pl_motion_input_ck(0) != 1) {
        switch (stage_map_kind_get(get_now_mapno())) {
        case 6:
            if (area == 1)
                mode = 7;
            else
                mode = 8;
            break;
        case 7:
            mode = 9;
            break;
        case 8:
            mode = 10;
            break;
        case 11:
            mode = 11;
            break;
        case 9:
            if (area == 1)
                mode = 11;
            else
                mode = 10;
            break;
        default:
            return;
        }
        camera_mode_req(mode, NULL);
    }
}

/* Requests event mode 12 with the caller's argument. */
extern "C" void camera_mode12_set(struct _ENEMY_WORK* enemy)
{
    camera_mode_req(12, enemy);
}

/* Requests event mode 17. */
extern "C" void camera_lobby_mode_req(void)
{
    camera_mode_req(17, 0);
}

/* Requests `mode` with an 8 in the event flags. */
extern "C" void camera_demo_event_set(u8 mode, struct _ENEMY_WORK* enemy)
{
    CamWork* work = (CamWork*)camera_work_get();

    camera_mode_req(mode, enemy);
    work->event.flags = 8;
}

/* Requests event mode `base + 17`. */
extern "C" void camera_lobby_mode_offset_req(u32 base, struct _ENEMY_WORK* enemy)
{
    camera_mode_req((u8)(base + 17), enemy);
}

/* Returns the event camera's frame, one lower when `full` is set. */
extern "C" s16 camera_frame_get(u8 full)
{
    CamWork* work = (CamWork*)camera_work_get();

    if (full == 0)
        return work->event.timer;
    return (s16)(work->event.timer - 1);
}

/* Returns the event camera's script length. */
extern "C" s16 camera_event_length_get(void)
{
    return ((CamWork*)camera_work_get())->event.length;
}

/* Resets the event request and requests `mode`, but only while the event camera is active. */
extern "C" void camera_event_set(u8 mode, struct _ENEMY_WORK* enemy)
{
    if (camera_work_ck()) {
        camera_talk_reset();
        camera_mode_req(mode, enemy);
    }
}

/* Runs the event camera's script: on the first step it starts the script from the current view, then it steps
 * the interpreter until the script ends. */
extern "C" s32 camera_event_script_run(CamWork* work, CamEvent* cam)
{
    s32 result;

    switch (cam->sub_step) {
    case 0:
        cam->sub_step++;
        cam->mark = cam->script = camera_event_scripts[cam->mode];
        copyVec3(&cam->pos, &get_camera_pos());
        copyVec3(&cam->target, &camera_target_pos_get());
        cam->roll = camera_roll_get();
        cam->fovy = camera_fovy_get();
        copyVec3(&cam->pos_start, &cam->pos);
        copyVec3(&cam->target_start, &cam->target);
        cam->roll_start = (s16)(10430.378f * cam->roll);
        cam->fovy_start = (s16)(10430.378f * cam->fovy);
        cam->field_0xFC = 0;
        cam->field_0xF8 = 0.0f;
        cam->timer_max = -1;
        cam->timer = -1;
    case 1:
        result = camera_event_script_step(work, cam);
        if (result <= 0)
            return result;
        cam->sub_step++;
    case 2:
        return 1;
    }
    return result;
}

/* The event camera's script interpreter: runs the script words until a wait, the end or an interrupt; 1 when
 * the script ended or was interrupted, 0 while it waits, -1 when it asks to hold. */
extern "C" s32 camera_event_script_step(CamWork* work, CamEvent* cam)
{
    CamScriptOp* next;
    CamScriptOp* op;
    s32 run;
    s32 result;

    cam->flags &= ~1;
    next = cam->script;
    run = 1;
    do {
        op = next;
        next = (CamScriptOp*)((u32*)next + op->size);
        switch (op->opcode) {
        case 0:
            cam->interp = op->bytes.arg;
            break;
        case 1:
            cam->pos_space = op->bytes.arg;
            cam->pos_arg = op->bytes.low;
            break;
        case 2:
            camera_script_pos_read(&(&cam->pos_start)[(s8)op->bytes.arg], cam, op);
            break;
        case 3:
            cam->target_space = op->bytes.arg;
            cam->target_arg = op->bytes.low;
            break;
        case 4:
            camera_script_target_read(&(&cam->target_start)[(s8)op->bytes.arg], cam, op);
            break;
        case 5:
            cam->field_0x89 = op->bytes.arg;
            break;
        case 6:
            cam->field_0x98 = op->value;
            break;
        case 7:
            cam->field_0xA0 = op->value;
            break;
        case 8:
            cam->field_0x9C = op->value;
            break;
        case 9:
            cam->field_0xA4 = op->value;
            break;
        case 10:
            cam->field_0x90 = 0.0625f * (f32)op->data[0];
            break;
        case 11:
            cam->field_0x94 = 0.0625f * (f32)op->data[0];
            break;
        case 12:
            cam->roll_start = op->value;
            break;
        case 13:
            cam->roll_end = op->value;
            break;
        case 14:
            cam->fovy_start = op->value;
            break;
        case 15:
            cam->fovy_end = op->value;
            break;
        case 16:
            cam->timer = cam->timer_max = op->value;
            break;
        case 17:
            cam->field_0xB8 = op->value;
            break;
        case 18:
            cam->field_0xBC = op->value;
            break;
        case 19:
            cam->field_0xC0 = op->value;
            break;
        case 20:
            cam->field_0xC2 = op->value;
            break;
        case 21:
            cam->field_0xC4 = op->value;
            break;
        case 22:
            cam->field_0xC8 = op->value;
            break;
        case 23:
            cam->field_0xCC = op->value;
            break;
        case 24:
            cam->field_0xCE = op->value;
            break;
        case 25:
            cam->field_0xD0 = op->value;
            break;
        case 26:
            cam->field_0xD4 = op->value;
            break;
        case 27:
            cam->field_0xD8 = op->value;
            break;
        case 28:
            cam->field_0xDC = op->value;
            break;
        case 29:
            cam->field_0xE0 = op->value;
            break;
        case 30:
            cam->field_0xE4 = op->value;
            break;
        case 31:
            cam->field_0xE8 = op->value;
            break;
        case 32:
            cam->field_0xEC = op->value;
            break;
        case 33:
            cam->field_0xF0[(s8)op->bytes.arg] = op->bytes.low;
            break;
        case 34:
            camera_event_channel_latch(cam, (s8)op->bytes.arg);
            break;
        case 35:
            cam->field_0xFC = (op->value >> 14) & 3;
            cam->field_0xF8 = 0.0009765625f * (f32)(op->value & 0x3FFF);
            break;
        case 36:
            cam->timer = cam->timer_max = op->value;
            cam->loop = next;
            break;
        case 37:
            if ((s8)op->bytes.arg != 0)
                camera_event_path_step(work, cam);
            cam->timer--;
            if (cam->timer_max <= 0 || cam->timer >= 0) {
                next = cam->loop;
                run = 0;
                result = 0;
            } else {
                copyVec3(&cam->pos_start, &cam->pos);
                copyVec3(&cam->target_start, &cam->target);
                cam->field_0x98 = cam->field_0xA0;
                cam->field_0x9C = cam->field_0xA4;
                cam->roll_start = cam->roll_end;
                cam->fovy_start = cam->fovy_end;
                cam->field_0xBC = 0;
                cam->field_0xC8 = 0;
                cam->field_0xD4 = 0;
                cam->field_0xE4 = 0;
                cam->field_0xC0 = cam->field_0xC2;
                cam->field_0xCC = cam->field_0xCE;
                cam->field_0xD8 = cam->field_0xDC;
                cam->field_0xE8 = cam->field_0xEC;
                cam->field_0xFC = 0;
                cam->field_0xF8 = 0.0f;
                cam->timer = -1;
                cam->timer_max = -1;
            }
            break;
        case 38:
            camera_event_path_step(work, cam);
            if ((s8)op->bytes.arg != 0) {
                next = cam->loop;
                run = 0;
                result = 0;
            }
            break;
        case 41:
            cam->flags |= 4;
            break;
        case 43:
            cam->mark = next;
            break;
        case 42:
            if (cam->mark != NULL)
                next = cam->mark;
            break;
        case 39:
        default:
            return 1;
        case 40:
            run = 0;
            result = -1;
            break;
        }
        if ((cam->flags & 1) != 0)
            return 1;
        cam->script = next;
    } while (run != 0);
    return result;
}

/* Reads a script point into `out`: the three 1/4096 coordinates, placed in the frame `pos_space` names (the
 * player's turned frame, the player's position, or the monster's). */
extern "C" void camera_script_pos_read(nw4r::math::VEC3* out, CamEvent* cam, CamScriptOp* op)
{
    _PLW* player;

    player = (_PLW*)get_move_work_adrs(2);
    player = &player[(s8)my_player_no()];
    out->x = 0.00024414062f * (f32)op->data[0];
    out->y = 0.00024414062f * (f32)op->data[1];
    out->z = 0.00024414062f * (f32)op->data[2];
    switch (cam->pos_space) {
    case 0:
        rotVecY(out, player->field_0x058);
        copyVec3(out, &vec3_add(out, &player->vec_0x03C));
        break;
    case 1:
        copyVec3(out, &vec3_add(out, &player->vec_0x03C));
        break;
    case 3: {
        u32 angle;
        nw4r::math::VEC3 base;

        angle = 0;
        setVec3(&base, 0.0f, 0.0f, 0.0f);
        camera_event_enemy_get(cam, &angle, &base);
        rotVecY(out, angle);
        copyVec3(out, &vec3_add(out, &base));
        break;
    }
    }
}

/* Reads a script look point into `out`, placed in the frame `target_space` names. */
extern "C" void camera_script_target_read(nw4r::math::VEC3* out, CamEvent* cam, CamScriptOp* op)
{
    _PLW* player;

    player = (_PLW*)get_move_work_adrs(2);
    player = &player[(s8)my_player_no()];
    out->x = 0.00024414062f * (f32)op->data[0];
    out->y = 0.00024414062f * (f32)op->data[1];
    out->z = 0.00024414062f * (f32)op->data[2];
    switch (cam->target_space) {
    case 0:
        rotVecY(out, player->field_0x058);
        copyVec3(out, &vec3_add(out, &player->vec_0x03C));
        break;
    case 1:
        copyVec3(out, &vec3_add(out, &player->vec_0x03C));
        break;
    case 3: {
        u32 angle;
        nw4r::math::VEC3 base;

        angle = 0;
        setVec3(&base, 0.0f, 0.0f, 0.0f);
        camera_event_enemy_get(cam, &angle, &base);
        rotVecY(out, angle);
        if (camera_event_enemy_mode2_ck(cam) == 1) {
            out->x = base.x;
            out->y = base.y - out->y;
            out->z = base.z;
        } else if (camera_event_enemy_mode1_ck(cam) == 1) {
            copyVec3(out, &base);
        } else {
            copyVec3(out, &vec3_add(out, &base));
        }
        break;
    }
    }
}

/* Latches the end value of every event-camera channel `mask` selects into its start value. */
extern "C" void camera_event_channel_latch(CamEvent* cam, u32 mask)
{
    if ((mask & 1) != 0)
        copyVec3(&cam->pos_end, &cam->pos_start);
    if ((mask & 2) != 0)
        copyVec3(&cam->target_end, &cam->target_start);
    if ((mask & 4) != 0)
        cam->roll_end = cam->roll_start;
    if ((mask & 8) != 0)
        cam->fovy_end = cam->fovy_start;
    if ((mask & 0x10) != 0)
        cam->field_0xC2 = cam->field_0xC0;
    if ((mask & 0x20) != 0)
        cam->field_0xCE = cam->field_0xCC;
    if ((mask & 0x40) != 0)
        cam->field_0xDC = cam->field_0xD8;
    if ((mask & 0x80) != 0)
        cam->field_0xEC = cam->field_0xE8;
}

/* Moves the event camera along its path segment: the position and target blend by the timer, the camera is
 * kept out of the walls and the ground when the script asks for it, and the roll and field of view blend. */
extern "C" void camera_event_path_step(CamWork* work, CamEvent* cam)
{
    nw4r::math::MTX34 unused_mtx;
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    nw4r::math::VEC3 hit;
    u8 area;
    f32 t;
    f32 inv;

    MTX34_ctor(&unused_mtx);
    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&hit);
    area = get_now_areano();
    if (cam->timer_max > 0)
        t = (f32)cam->timer / (f32)cam->timer_max;
    else
        t = 1.0f;
    inv = 1.0f - t;
    if (cam->interp == 1) {
        vec3_lerp(&cam->target, &cam->target_start, &cam->target_end, t);
        vec3_lerp(&cam->pos, &cam->pos_start, &cam->pos_end, t);
    }
    if ((cam->flags & 4) != 0) {
        f32 floor;

        if (coll_line_check(&cam->target, &cam->pos, 10.0f, &hit, 3, -5, area, 1) > 0)
            copyVec3(&cam->pos, &hit);
        floor = 30.0f + GetGroundHit(&cam->pos, -5, area);
        if (floor > cam->pos.y)
            cam->pos.y = floor;
        camera_water_clamp(cam, &cam->target);
    }
    cam->roll = 0.0000958738f * (f32)cam->roll_start * t + 0.0000958738f * (f32)cam->roll_end * inv;
    cam->fovy = (360.0f * (0.0000958738f * (f32)cam->fovy_start * t + 0.0000958738f * (f32)cam->fovy_end * inv)) /
                6.2831855f;
}

/* Copies the event monster's facing angle and position out, or raises the camera's no-target flag when
 * there is no live monster. */
extern "C" void camera_event_enemy_get(CamEvent* cam, u32* angle, nw4r::math::VEC3* pos)
{
    struct _ENEMY_WORK* enemy = cam->enemy;

    if (enemy != NULL && enemy->active != 0) {
        *angle = enemy->field_0x1C0;
        copyVec3(pos, &enemy->pos);
        return;
    }
    cam->flags |= 1;
}

/* Copies the event monster's two angles and its position out, or raises the camera's no-target flag
 * when there is no live monster. */
extern "C" void camera_event_enemy_angles_get(CamEvent* cam, u32* angle_a, u32* angle_b, nw4r::math::VEC3* pos)
{
    struct _ENEMY_WORK* enemy = cam->enemy;

    if (enemy != NULL && enemy->active != 0) {
        *angle_a = enemy->field_0x1BC;
        *angle_b = enemy->field_0x1C0;
        copyVec3(pos, &enemy->pos);
        return;
    }
    cam->flags |= 1;
}

/* Whether the event camera may frame its monster and the monster is alive in its first mode. */
extern "C" bool camera_event_enemy_mode1_ck(CamEvent* cam)
{
    struct _ENEMY_WORK* enemy;

    if ((cam->flags & 0x10) != 0)
        return false;
    enemy = cam->enemy;
    if (enemy != NULL && enemy->active != 0 && enemy->field_0x1E2 == 1)
        return true;
    return false;
}

/* Whether the event camera's monster is alive and in its second mode. */
extern "C" bool camera_event_enemy_mode2_ck(CamEvent* cam)
{
    struct _ENEMY_WORK* enemy = cam->enemy;

    if (enemy != NULL && enemy->active != 0 && enemy->field_0x1E2 == 1 && enemy->field_0x1E3 == 2)
        return true;
    return false;
}

/* Clears the demo-camera key buffers. */
extern "C" void camera_dcm_buffers_clear(void)
{
    u8 i;

    for (i = 0; i < 50; i++)
        stage_dcm_buffer_tbl[i] = NULL;
}

/* Runs the event camera from the demo-camera keys of its mode: finds the loaded key run, then plays it to its
 * end; 1 when it is done (or there are no keys). */
extern "C" s32 camera_event_dcm_run(CamWork* work, CamEvent* cam)
{
    s32 done;
    CamDcmEntry* entry;
    u8 i;

    done = 0;
    switch (cam->sub_step) {
    case 0:
        cam->keys = NULL;
        i = 0;
        for (entry = camera_dcm_table; entry->mode != 0xFFFF; entry++) {
            if (cam->mode == entry->mode) {
                cam->keys = stage_dcm_buffer_tbl[i];
                cam->slot = i;
                break;
            }
            i++;
        }
        if (cam->keys == NULL) {
            done = 1;
            break;
        }
        cam->length = camera_dcm_length_get(cam);
        cam->timer = cam->frames;
        cam->timer_max = 0;
        cam->sub_step++;
    case 1:
        camera_dcm_step(work, cam);
        if (cam->timer_max != 0) {
            cam->timer_max--;
            break;
        }
        cam->sub_step++;
    case 2:
        if (camera_dcm_step(work, cam) == 0) {
            cam->timer++;
            break;
        }
        cam->sub_step++;
    case 3:
        cam->timer = 0;
        done = 1;
        break;
    }
    return done;
}

/* Returns half the last frame of the event camera's key run, or zero when the run is empty. */
extern "C" s16 camera_dcm_length_get(CamEvent* cam)
{
    CamDcmKey* key = cam->keys;
    s32 value = 0;

    if (key->frame >= 0) {
        while (key->frame >= 0)
            key++;
        value = key[-1].frame;
    }
    return (s16)(value / 2);
}

/* Places the event camera at the demo-camera keys' frame (two frames per timer tick): on a key, or between the
 * two around it; then relative to the monster and out of the walls and the ground when the entry asks for it.
 * 1 when the run ended. */
extern "C" s32 camera_dcm_step(CamWork* work, CamEvent* cam)
{
    s32 frame;
    CamDcmKey* key;
    CamDcmEntry* entry;
    u32 pitch;
    u32 yaw;
    s32 loop;
    nw4r::math::VEC3 base;
    nw4r::math::VEC3 rel;

    frame = cam->timer << 1;
    key = cam->keys;
    entry = &camera_dcm_table[cam->slot];
    pitch = 0;
    yaw = 0;
    loop = 0;
    setVec3(&base, 0.0f, 0.0f, 0.0f);
    VEC3_ctor(&rel);
    if ((entry->flags & 8) != 0)
        loop = 1;
    while (key->frame >= 0) {
        if (frame == key->frame) {
            copyVec3(&cam->pos, &key->pos);
            copyVec3(&cam->target, &key->target);
            cam->roll = key->roll;
            cam->fovy = key->fovy;
            break;
        }
        if (frame < key->frame) {
            f32 t = (f32)(frame - key[-1].frame) / (f32)(key->frame - key[-1].frame);

            copyVec3(&rel, &vec3_sub(&key->pos, &key[-1].pos));
            vec3_scale_in_place(&rel, t);
            copyVec3(&cam->pos, &vec3_add(&rel, &key[-1].pos));
            copyVec3(&rel, &vec3_sub(&key->target, &key[-1].target));
            vec3_scale_in_place(&rel, t);
            copyVec3(&cam->target, &vec3_add(&rel, &key[-1].target));
            cam->roll = (key->roll - key[-1].roll) * t + key[-1].roll;
            cam->fovy = (key->fovy - key[-1].fovy) * t + key[-1].fovy;
            break;
        }
        key++;
    }
    if (loop != 0 && key->frame < 0) {
        copyVec3(&cam->pos, &key[-1].pos);
        copyVec3(&cam->target, &key[-1].target);
        cam->roll = key[-1].roll;
        cam->fovy = key[-1].fovy;
    }
    if ((entry->flags & 1) != 0) {
        camera_event_enemy_angles_get(cam, &pitch, &yaw, &base);
        rotVecX(&cam->pos, pitch);
        rotVecX(&cam->target, pitch);
        rotVecY(&cam->pos, yaw);
        rotVecY(&cam->target, yaw);
        copyVec3(&cam->pos, &vec3_add(&cam->pos, &base));
        copyVec3(&cam->target, &vec3_add(&cam->target, &base));
    }
    if ((entry->flags & 2) != 0) {
        nw4r::math::VEC3 hit;
        f32 floor;

        VEC3_ctor(&hit);
        if (coll_line_check(&cam->target, &cam->pos, 10.0f, &hit, 3, -5, get_now_areano(), 1) != 0)
            copyVec3(&cam->pos, &hit);
        floor = 10.0f + GetGroundHit(&cam->pos, -5, get_now_areano());
        if (floor > cam->pos.y)
            cam->pos.y = floor;
    }
    if (key->frame < 0)
        return loop == 0;
    return 0;
}

/* The 16-bit difference of two angles, shifted right. */
extern "C" s32 camera_angle_approach(u32 a, u32 b, u8 shift)
{
    return (s16)(a - b) >> shift;
}

/* The current camera's view matrix. */
CamMtx34 get_current_view_mtx(void)
{
    CamMtx34 mtx;
    nw4r::g3d::Camera cam;
    void* handle;

    handle = scene_current_camera_get();
    word_copy_return_dst(&cam, &handle);
    MTX34_ctor(&mtx);
    cam.GetCameraMtx(&mtx);
    return mtx;
}

/* Writes the current camera's projection matrix. */
extern "C" void camera_proj_mtx_get(nw4r::math::MTX44* out)
{
    nw4r::g3d::Camera cam;
    void* handle;

    handle = scene_current_camera_get();
    word_copy_return_dst(&cam, &handle);
    MTX44_ctor(out);
    camera_get_projection_mtx(&cam, out);
}

/* The current camera's world position. */
CamVec3 get_camera_pos(void)
{
    CamVec3 pos;
    nw4r::g3d::Camera cam;
    void* handle;

    VEC3_ctor(&pos);
    handle = scene_current_camera_get();
    word_copy_return_dst(&cam, &handle);
    camera_get_position(&cam, &pos);
    return pos;
}

/* Writes the position of the camera `kind` names: the follow camera's, the area camera's, or zero. */
extern "C" void camera_view_pos_get(nw4r::math::VEC3* out, u8 kind)
{
    CamWork* work = (CamWork*)camera_work_get();

    switch (kind) {
    case 0:
        assignVec3(out, &work->follow.pos);
        break;
    case 4:
        assignVec3(out, &work->area.pos);
        break;
    default:
        setVec3(out, 0.0f, 0.0f, 0.0f);
        break;
    }
}

/* Writes the current camera's target. */
extern "C" void get_camera_target_pos(nw4r::math::VEC3* out)
{
    nw4r::math::VEC3 unused;
    nw4r::g3d::Camera cam;
    void* handle;
    nw4r::g3d::Camera::PostureInfo info;

    VEC3_ctor(&unused);
    handle = scene_current_camera_get();
    word_copy_return_dst(&cam, &handle);
    camera_posture_info_ctor(&info);
    camera_posture_get(&cam, &info);
    assignVec3(out, (nw4r::math::VEC3*)&info.mTargetX);
}

/* The current camera's view direction: its target minus its position. */
CamVec3 get_camera_direction(void)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 unused;
    nw4r::g3d::Camera cam;
    void* handle;
    nw4r::g3d::Camera::PostureInfo info;

    VEC3_ctor(&pos);
    VEC3_ctor(&unused);
    handle = scene_current_camera_get();
    word_copy_return_dst(&cam, &handle);
    camera_posture_info_ctor(&info);
    camera_posture_get(&cam, &info);
    camera_get_position(&cam, &pos);
    return cam_vec3_sub((nw4r::math::VEC3*)&info.mTargetX, &pos);
}

/* Writes the current camera's two projection parameters at +0xB4 and +0xB8. */
extern "C" void camera_proj_params_get(f32* a, f32* b)
{
    nw4r::g3d::Camera cam;
    void* handle;

    handle = scene_current_camera_get();
    word_copy_return_dst(&cam, &handle);
    *a = camera_ref(&cam)->mProjC;
    *b = camera_ref(&cam)->mProjD;
}

/* The current camera's pitch, from its target to its position. */
extern "C" u32 camera_angle_x_get(void)
{
    nw4r::math::VEC3 rel;
    u32 pitch;
    u32 yaw;

    VEC3_ctor(&rel);
    copyVec3(&rel, &vec3_sub(&get_camera_pos(), &camera_target_pos_get()));
    calcVecAngXY(&rel, &pitch, &yaw);
    return pitch;
}

/* The current camera's yaw, from its target to its position. */
extern "C" s32 camera_angle_y_get(void)
{
    nw4r::math::VEC3 rel;
    u32 pitch;
    u32 yaw;

    VEC3_ctor(&rel);
    copyVec3(&rel, &vec3_sub(&get_camera_pos(), &camera_target_pos_get()));
    calcVecAngXY(&rel, &pitch, &yaw);
    return yaw;
}

/* The current camera's field of view. */
extern "C" f32 camera_fovy_get(void)
{
    nw4r::g3d::Camera cam;
    void* handle;

    handle = scene_current_camera_get();
    word_copy_return_dst(&cam, &handle);
    return camera_ref(&cam)->mProjA;
}

/* The current camera's roll. */
extern "C" f32 camera_roll_get(void)
{
    nw4r::g3d::Camera cam;
    void* handle;

    handle = scene_current_camera_get();
    word_copy_return_dst(&cam, &handle);
    return camera_ref(&cam)->mUnkA4;
}

/* Returns the low half of the view yaw. */
extern "C" u32 get_camera_rot_y(void)
{
    CamWork* work = (CamWork*)camera_work_get();

    return (u16)work->view_yaw;
}

/* Returns the low half of the view pitch. */
extern "C" u32 get_camera_rot_x(void)
{
    CamWork* work = (CamWork*)camera_work_get();

    return (u16)work->view_pitch;
}

/* Whether the map cell under the camera is one of the three underwater kinds and the camera is below the
 * water plane. */
extern "C" u32 camera_underwater_ck(void)
{
    CamWork* work = (CamWork*)camera_work_get();
    u16 cell = work->area_handle;

    if ((cell & 2) != 0 || (cell & 1) != 0 || (cell & 4) != 0) {
        if (get_camera_pos().y < 0.0f)
            return 1;
    }
    return 0;
}

/* Hands a camera view to the g3d camera `index`: its position, target and roll, and its field of view when it
 * has one. */
extern "C" void camera_view_apply(u8 index, nw4r::math::VEC3* pos, nw4r::math::VEC3* target, f32 fovy, f32 roll)
{
    nw4r::g3d::Camera cam;
    s32 handle;
    nw4r::g3d::Camera::PostureInfo info;

    handle = scene_camera_get(index);
    word_copy_return_dst(&cam, &handle);
    camera_posture_info_ctor(&info);
    camera_posture_get(&cam, &info);
    info.mType = 2;
    copyVec3((nw4r::math::VEC3*)&info.mTargetX, target);
    info.mUnk28 = roll;
    camera_position_set(&cam, pos);
    cam.SetPosture(info);
    if (fovy > 0.0f)
        cam.SetPerspective(fovy, screen_aspect_get(), 5.0f, 500000.0f);
}

/* Moves the g3d camera to `pos` when there is one. */
extern "C" void camera_position_set(nw4r::g3d::Camera* cam, const nw4r::math::VEC3* pos)
{
    if (pos != NULL)
        cam->SetPosition(*pos);
}

/* Records the current camera's yaw and pitch in the camera work. */
extern "C" void camera_view_angles_update(CamWork* work)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 target;
    nw4r::math::VEC3 dir;

    VEC3_ctor(&pos);
    VEC3_ctor(&target);
    VEC3_ctor(&dir);
    copyVec3(&pos, &get_camera_pos());
    copyVec3(&target, &camera_target_pos_get());
    work->view_yaw = calcVecAng2(&pos, &target);
    copyVec3(&dir, &vec3_sub(&target, &pos));
    work->view_pitch = calcVecAngX_c1(&dir);
}

/* Whether the follow camera may take the player's input: no event, demo, area camera, kill cut, dodge or
 * climb-up running, and the lock-on not in states 0, 1 or 3. */
extern "C" u32 camera_input_ok_ck(_PLW* player)
{
    if (camera_work_ck() == 1)
        return 0;
    if (event_demo_running_ck_c1() != 0)
        return 0;
    if (camera_area_active_ck() == 1)
        return 0;
    if (camera_cut_active_ck() == 1)
        return 0;
    if (player->camera_lock_0x5E5 != 0)
        return 0;
    if (pl_camera_reset_ck(player, 0) == 1)
        return 0;
    if (camera_player_climb_ck(player, 1) == 1)
        return 0;
    if ((s8)camera_lock_state_get() == 0)
        return 0;
    if ((s8)camera_lock_state_get() == 1)
        return 0;
    return (s8)camera_lock_state_get() != 3;
}

/* Whether the aim camera is active while no event runs. */
extern "C" u32 camera_mode1_ck(void)
{
    CamWork* work = (CamWork*)camera_work_get();

    if (camera_work_ck() == 1)
        return 0;
    return work->aim.active == 1;
}

/* Whether the event camera is active. */
extern "C" bool camera_work_ck(void)
{
    CamWork* work = (CamWork*)camera_work_get();

    return work->event.active == 1;
}

/* Whether the area camera is active. */
extern "C" bool camera_area_active_ck(void)
{
    CamWork* work = (CamWork*)camera_work_get();

    return work->area.active == 1;
}

/* Arms one quake slot: the origin, the kind, the duration `camera_quake_frames` gives it and the level. */
extern "C" void camera_quake_arm(CamQuake* slot, const nw4r::math::VEC3* origin, u8 kind, u8 level)
{
    slot->active = 1;
    slot->kind = kind;
    slot->timer = camera_quake_frames[kind & 0x1F];
    copyVec3(&slot->origin, origin);
    slot->level = level;
}

/* Starts a camera quake at `origin` in the first quake slot. */
void set_quake_sub(u8 kind, nw4r::math::VEC3* origin)
{
    camera_quake_arm(&((CamWork*)camera_work_get())->quake[0], origin, kind, 0);
}

/* Starts the first quake slot from the zero vector, with the kind's high bit set. */
extern "C" void camera_quake_req(u8 kind)
{
    CamWork* work = (CamWork*)camera_work_get();
    nw4r::math::VEC3 origin;

    VEC3_ctor(&origin);
    setVector3(&origin, 0.0f, 0.0f, 0.0f);
    camera_quake_arm(&work->quake[0], &origin, (u8)(kind | 0x80), 0);
}

/* Starts the first quake slot at the player when the player is the local one. */
extern "C" void camera_pl_quake_req(_PLW* player, u8 kind)
{
    CamQuake* slot = &((CamWork*)camera_work_get())->quake[0];

    if (pl_now_area_ck(player) != 0)
        camera_quake_arm(slot, &player->vec_0x03C, kind, 0);
}

/* Starts the first quake slot at the monster when it is in the player's area. */
extern "C" void camera_em_shake_req(struct _ENEMY_WORK* enemy, u32 kind)
{
    CamQuake* slot = &((CamWork*)camera_work_get())->quake[0];

    if (em_area_ck(enemy) != 0)
        camera_quake_arm(slot, &enemy->pos, kind, 0);
}

/* Starts the first quake slot at `pos` when the monster is in the player's area. */
extern "C" void camera_enemy_focus_pos_set(struct _ENEMY_WORK* enemy, s32 kind, nw4r::math::VEC3* pos)
{
    CamQuake* slot = &((CamWork*)camera_work_get())->quake[0];

    if (em_area_ck(enemy) != 0)
        camera_quake_arm(slot, pos, kind, 0);
}

/* Starts the first quake slot at `pos` with `level` when the monster is in the player's area. */
extern "C" void camera_quake_pos_req(struct _ENEMY_WORK* enemy, u8 kind, nw4r::math::VEC3* pos, u8 level)
{
    CamQuake* slot = &((CamWork*)camera_work_get())->quake[0];

    if (em_area_ck(enemy) != 0)
        camera_quake_arm(slot, pos, kind, level);
}

/* Starts the second quake slot at the player when the player is the local one. */
extern "C" void camera_pl_quake2_req(_PLW* player, u8 kind)
{
    CamQuake* slot = &((CamWork*)camera_work_get())->quake[1];

    if (pl_now_area_ck(player) != 0)
        camera_quake_arm(slot, &player->vec_0x03C, kind, 0);
}

/* Starts the third quake slot from the zero vector, with the kind's high bits set. */
extern "C" void camera_shake_req(u8 kind)
{
    CamWork* work = (CamWork*)camera_work_get();
    nw4r::math::VEC3 origin;

    VEC3_ctor(&origin);
    setVector3(&origin, 0.0f, 0.0f, 0.0f);
    camera_quake_arm(&work->quake[2], &origin, (u8)(kind | 0xA0), 0);
}

/* Shakes the g3d camera `index` by one quake slot: the kind's amplitude on a sine of the slot's timer, faded by
 * the distance from the origin, as a lift and a sideways sway. */
extern "C" void camera_quake_apply(CamWork* work, CamQuake* slot, u8 index)
{
    if (slot->active != 0) {
        nw4r::math::VEC3 pos;
        nw4r::math::VEC3 target;
        nw4r::math::VEC3 sway;
        u16 cell;
        f32 height;
        u8 kind;
        s32 row;
        f32 amp;
        f32 side;

        VEC3_ctor(&pos);
        VEC3_ctor(&target);
        VEC3_ctor(&sway);
        copyVec3(&pos, &get_camera_pos());
        copyVec3(&target, &camera_target_pos_get());
        if (index == 1 && stage_water_surface_get(&pos, get_now_areano(), &cell, &height, 4) != 0 &&
            pos.y < 15.0f + height && pos.y > height - 15.0f)
            return;
        f32 fade;

        kind = slot->kind;
        row = kind & 0x1F;
        amp = (f32)camera_quake_amplitude[row];
        amp *= 1.0f - (1.0f + sinf((6.2831855f * (90.0f + 180.0f * ((f32)slot->timer / (f32)camera_quake_frames[row]))) /
                                   360.0f)) *
                          0.5f;
        fade = 1.0f;
        if ((kind & 0x80) == 0) {
            f32 dist = vec3_distance(&slot->origin, &pos);
            f32 range = camera_quake_range[slot->level];

            if (dist > range)
                fade = 0.0f;
            else
                fade = 1.0f - dist * (1.0f / range);
        }
        amp *= fade;
        if (((slot->timer >> 1) & 1) != 0)
            amp = -amp;
        if ((slot->kind & 0x40) != 0 && slot->timer > camera_quake_frames[row] - camera_quake_frames[row] / 2) {
            side = amp * 0.5f;
            if (((slot->timer >> 1) & 2) != 0)
                side = -side;
        } else {
            side = 0.0f;
        }
        setVector3(&sway, side, 0.0f, 0.0f);
        rotVecY(&sway, camera_angle_y_get());
        sway.y = amp;
        addVec3To(&pos, &sway);
        addVec3To(&target, &sway);
        camera_view_apply(index, &pos, &target, camera_fovy_get(), camera_roll_get());
    }
}

/* Ages the three quake timers by one and hands the slot the given camera selects to the per-slot update. */
extern "C" void camera_quake_update(LightWork* self, u8 index)
{
    if (--self->channel[0].timer <= 0) {
        self->channel[0].enable = 0;
        self->channel[0].timer = 0;
    }

    if (--self->channel[1].timer <= 0) {
        self->channel[1].enable = 0;
        self->channel[1].timer = 0;
    }

    if (--self->channel[2].timer <= 0) {
        self->channel[2].enable = 0;
        self->channel[2].timer = 0;
    }

    if (index == 3) {
        camera_quake_apply((CamWork*)self, (CamQuake*)&self->channel[2], index);
    } else if (index == 1) {
        camera_quake_apply((CamWork*)self, (CamQuake*)&self->channel[1], index);
    } else {
        camera_quake_apply((CamWork*)self, (CamQuake*)&self->channel[0], index);
    }
}

/* Keeps a camera view out of the water surface under it: pushes it up (or down) off the surface, with a margin
 * that grows with the view's downward pitch; 1 when it moved the camera. */
extern "C" u8 camera_water_clamp(CamView* cam, nw4r::math::VEC3* target)
{
    f32 lift;
    nw4r::math::VEC3 dir;
    u16 cell;
    f32 height;

    lift = 0.0f;
    VEC3_ctor(&dir);
    if (target != NULL) {
        u16 pitch;

        copyVec3(&dir, &vec3_sub(target, &cam->pos));
        pitch = calcVecAngX(&dir);
        if (pitch != 0 && pitch < 0x4000)
            lift = 6.0f * ((f32)pitch / 16384.0f);
    }
    if (stage_water_surface_get(&cam->pos, get_now_areano(), &cell, &height, 4) != 0) {
        if (camera_cut_active_ck() == 1) {
            f32 top = 6.0f + height;

            if (cam->pos.y < top && cam->pos.y >= height - 60.0f)
                cam->pos.y = top;
        } else if (cam->pos.y >= height) {
            f32 top = 6.0f + height + lift;

            if (cam->pos.y < top) {
                cam->pos.y = top;
                return 1;
            }
        } else {
            f32 bottom = height - 6.0f + lift;

            if (cam->pos.y > bottom) {
                cam->pos.y = bottom;
                return 1;
            }
        }
    }
    return 0;
}

/* Formats the per-map demo-camera file name into the caller's buffer. */
extern "C" int stage_dcm_path_get(char* buffer, u8 mapno)
{
    return sprintf(buffer, stage_dcm_path_format, mapno);
}

/* Returns the current player's camera work: the second one in split screen for player 1, the first
 * otherwise. */
extern "C" LightWork* camera_work_get(void)
{
    if (screen_split_mode_ck() != 0 && (s8)my_player_no() != 0) {
        return &camera_work[1];
    }
    return &camera_work[0];
}

/* Runs camera_mode1_ck for the given player and puts the current player back afterwards. */
extern "C" u32 camera_mode1_ck_player(s8 player)
{
    s32 previous;
    u32 result;

    previous = my_player_no();
    my_player_no_set(player);
    result = camera_mode1_ck();
    my_player_no_set(previous);
    return result;
}

/* Runs camera_aim_zoom_get for the given player and puts the current player back afterwards. */
extern "C" u8 camera_aim_zoom_get_player(u32 unused, u8* out_mode, u8* out_zoom, f32* out_scale, s8 player)
{
    s32 previous;
    u8 result;

    previous = my_player_no();
    my_player_no_set(player);
    result = camera_aim_zoom_get(unused, out_mode, out_zoom, out_scale);
    my_player_no_set(previous);
    return result;
}

/* Runs camera_work_ck for the given player and puts the current player back afterwards. */
extern "C" void* camera_work_ck_player(s8 bank)
{
    s32 previous;
    void* result;

    previous = my_player_no();
    my_player_no_set(bank);
    result = (void*)(u32)camera_work_ck();
    my_player_no_set(previous);
    return result;
}

/* Starts the kill cut for both split-screen players whose camera position is not inside the stage. */
extern "C" void camera_kill_cut_start_split(u8 id, struct _ENEMY_WORK* arg)
{
    s32 previous;

    previous = my_player_no();

    my_player_no_set(0);
    if (!stage_pos_z_low_ck(&camera_work_get()->resource->entry)) {
        camera_kill_cut_start(id, arg);
    }

    my_player_no_set(1);
    if (!stage_pos_z_low_ck(&camera_work_get()->resource->entry)) {
        camera_kill_cut_start(id, arg);
    }

    my_player_no_set(previous);
}

/* Constructs the two camera works. */
extern "C" void camera_work_construct(void)
{
    __construct_array(&camera_work[0], (void*)camera_work_ctor, NULL, 0x4F8, 2);
}

/* Constructs a camera work: every camera in turn, then the three quake slots. */
extern "C" LightWork* camera_work_ctor(LightWork* self)
{
    LightChannel* end;
    LightChannel* channel;

    camera_follow_ctor(&self->root);
    camera_aim_ctor(&self->anim);
    camera_shoulder_ctor(&self->lights);
    camera_event_ctor(&self->params);
    camera_area_ctor(&self->colors);
    camera_lobby_ctor(&self->entry);

    channel = &self->channel[0];
    end = &self->channel[3];
    do {
        camera_quake_ctor(channel);
        channel++;
    } while (channel < end);
    return self;
}

/* Constructs a quake slot's origin vector. */
extern "C" LightChannel* camera_quake_ctor(LightChannel* self)
{
    VEC3_ctor(&self->pos);
    return self;
}

/* Constructs the lobby camera's four vectors and its goal. */
extern "C" LightQuadTriple* camera_lobby_ctor(LightQuadTriple* self)
{
    VEC3_ctor(&self->v[0]);
    VEC3_ctor(&self->v[1]);
    VEC3_ctor(&self->v[2]);
    VEC3_ctor(&self->v[3]);
    camera_lobby_goal_ctor(&self->inner);
    return self;
}

/* Constructs the lobby goal's three vectors. */
extern "C" LightTriple* camera_lobby_goal_ctor(LightTriple* self)
{
    VEC3_ctor(&self->a);
    VEC3_ctor(&self->b);
    VEC3_ctor(&self->c);
    return self;
}

/* Constructs the area camera's eight vectors. */
extern "C" LightOctal* camera_area_ctor(LightOctal* self)
{
    VEC3_ctor(&self->v[0]);
    VEC3_ctor(&self->v[1]);
    VEC3_ctor(&self->v[2]);
    VEC3_ctor(&self->v[3]);
    VEC3_ctor(&self->w[0]);
    VEC3_ctor(&self->w[1]);
    VEC3_ctor(&self->w[2]);
    VEC3_ctor(&self->w[3]);
    return self;
}

/* Constructs the event camera's four vectors and its two two-vector runs. */
extern "C" LightParams* camera_event_ctor(LightParams* self)
{
    nw4r::math::VEC3* vec;
    nw4r::math::VEC3* end;

    VEC3_ctor(&self->v0);
    VEC3_ctor(&self->v1);
    VEC3_ctor(&self->v2);
    VEC3_ctor(&self->v3);

    vec = &self->runs[0];
    end = &self->runs[2];
    do {
        VEC3_ctor(vec);
        vec++;
    } while (vec < end);

    end = &self->runs[4];
    do {
        VEC3_ctor(vec);
        vec++;
    } while (vec < end);
    return self;
}

/* Constructs the shoulder camera's four vectors. */
extern "C" LightQuad* camera_shoulder_ctor(LightQuad* self)
{
    VEC3_ctor(&self->v[0]);
    VEC3_ctor(&self->v[1]);
    VEC3_ctor(&self->v[2]);
    VEC3_ctor(&self->v[3]);
    return self;
}

/* Constructs the aim camera's four vectors, its rotation and its origin. */
extern "C" LightQuadMid* camera_aim_ctor(LightQuadMid* self)
{
    VEC3_ctor(&self->v[0]);
    VEC3_ctor(&self->v[1]);
    VEC3_ctor(&self->v[2]);
    VEC3_ctor(&self->v[3]);
    MTX34_ctor(&self->mid);
    VEC3_ctor(&self->tail);
    return self;
}

/* Constructs the follow camera's fifteen vectors. */
extern "C" LightRoot* camera_follow_ctor(LightRoot* self)
{
    VEC3_ctor(&self->v0);
    VEC3_ctor(&self->v1);
    VEC3_ctor(&self->v2);
    VEC3_ctor(&self->v3);
    VEC3_ctor(&self->v4);
    VEC3_ctor(&self->v5);
    VEC3_ctor(&self->v6);
    VEC3_ctor(&self->v7);
    VEC3_ctor(&self->v8);
    VEC3_ctor(&self->v9);
    VEC3_ctor(&self->v10);
    VEC3_ctor(&self->v11);
    VEC3_ctor(&self->v12);
    VEC3_ctor(&self->v13);
    VEC3_ctor(&self->v14);
    return self;
}
