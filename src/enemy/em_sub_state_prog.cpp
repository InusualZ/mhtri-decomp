/* enemy/em_sub_state_prog.cpp - the stage-area model effect (type 0x25, `eft037_*`) and the spawner, release and
 *   state machine of the camera screen filter (type 0x24, `eft036_*`, whose steps `lobby/fn_802FA9A0.cpp` holds).
 * RANGE. .text 0x802F9994-0x802FA9A0 (11 functions); .data 0x805D8210-0x805D8B00, .bss 0x806BE0C0-0x806BE0D8,
 *   .sdata 0x807928D0-0x80792948, .sdata2 0x8079ABC0-0x8079AC20, extab, extabindex.  The original TU runs on to
 *   0x802FAFC8: eft036's steps, its sphere tables (0x805D8B00..) and its pool (0x8079AC20.., repeating 1.0/0.0/255.0
 *   of this pool) are `lobby/fn_802FA9A0.cpp`'s registered range.
 * FLAGS. `cflags_main`, `-inline noauto`; file-wide `#pragma peephole off` (retail reloads a stored field before its
 *   test) and `#pragma pool_data off` (each table is addressed with its own `lis`/`addi`).
 * NAMES. `em_sub_area_fx_set`/`em_sub_area_fx2_set` are `stage/stg_w.cpp`'s spellings.  GUESS (from the effect type
 *   each spawner seeds and the eft036 names beside them): `eft037_models_release`, `eft037_dispatch`,
 *   `eft037_model_create`, `eft037_move`, `eft037_sin`, `eft037_end`, `eft037_release`, `eft036_models_release`,
 *   `eft036_dispatch`; the tables `eft037_joints_*` (the visible joint per model, 0xFF-ended), `eft037_skip_*`,
 *   `eft037_model_ids_*` and `eft037_pos_*` (model positions per map `mNN` and area `aNN`).  `em_sub_state_prog` is a
 *   GUESS unit name; the unit is the eft037 effect.
 * RESIDUALS. none in code; .sdata is 0x77 of the claim's 0x78 bytes (the last byte is alignment fill).
 *   `Eft036SpawnWork` is this unit's view of `lobby/fn_802FA9A0.cpp`'s file-local `Eft036Work` (one retail TU split
 *   across two registered units).
 * SHAPES. The data is defined in retail order; the position tables are `f32` arrays because three of them end in an
 *   odd zero word.  The create step's kind test is a range test on `(u32)(kind - 1)` with the kind-0 body inside its
 *   fail branch; the nine phases are `(k & 1) * 25`; the kind-3 fade divides by a `u32` local (retail converts 15
 *   unsigned at run time); the kind-2 colour loop uses the material counter `j`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h"
#include "fn_8004CAD8.h"
#include "pl.h"
#include "ef.h"
#include "ef/effect.h"
#include "ef/eft_res.h"
#include "ef/res_eft_UV_model_create_name.h"
#include "ef/fn_800CDB2C.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em_sub_area_fx_set.h"
#include "lobby/fn_802FA9A0.h"
#include "lobby/lb_menu_pos_tbl.h"
#include "stage/stg_w.h"
#include "stage/get_now_mapno.h"
#include "stage/stage_area_word_get.h"
#include "stage/stage_char_res_body_get.h"
#include "camera/camera.h"
#include "camera/camera_angle_x_get.h"
#include "camera/camera_mode_cur_get.h"
#include "Pl/pl_hit_sphere.h"
#include "sound/mhchar_mat_num_get.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/strcat.h"
#include "MSL/mathf.h"

#pragma peephole off
#pragma pool_data off

extern "C" {

/* One model of the area effect: the pooled model, the UV-animated model built for it and the height band its
 * placement fell in (1/2 above/below +-50000, 3/4 above/below +-40000, 5/6 above/below +-60000).  size: 0xC */
struct Eft037Model {
    /* +0x00 */ MHchar* model_0x00;
    /* +0x04 */ EftUvModelObject* uv_0x04;
    /* +0x08 */ u8 band_0x08;
    /* +0x09 */ u8 pad_0x09[3];
};

/* The work block of the area effect: up to 24 pooled models with five g3d works each, the visible-joint list, the
 * kind and the nine colour phases kind 2 cycles.  size: 0x31C (the size `eft_res_slot_get(0x31C)` pools) */
struct Eft037Work {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ Eft037Model models_0x004[24];
    /* +0x124 */ struct _g3d_work* works_0x124[24][5];
    /* +0x304 */ u8* joints_0x304;      /* per model, the one joint left visible; 0xFF ends it */
    /* +0x308 */ u8 kind_0x308;
    /* +0x309 */ u8 pad_0x309;
    /* +0x30A */ s16 phase_0x30A[9];    /* 0..120, the material alpha phase of kind 2 */
};

/* The spawner's view of the screen filter's work block (`lobby/fn_802FA9A0.cpp`'s `Eft036Work`): the pooled models
 * and their g3d works.  size: 0x1C */
struct Eft036SpawnWork {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ MHchar* models_0x04[1];
    /* +0x08 */ u8 pad_0x08[4];
    /* +0x0C */ struct _g3d_work* works_0x0C[1][2];
    /* +0x14 */ u8 kind_0x14;
    /* +0x15 */ u8 pad_0x15[7];
};

void eft037_models_release(_EFT* self);
void eft037_dispatch(_EFT* self);
void eft037_model_create(_EFT* self);
void eft037_move(_EFT* self);
f32 eft037_sin(f32 x);
void eft037_end(_EFT* self);
void eft037_release(_EFT* self);
void eft036_models_release(_EFT* self);
void eft036_dispatch(_EFT* self);

/* The visible joint of each model, per area of maps 1/12 and 3/14 (`em_sub_area_fx_set` kinds 1-3), 0xFF-ended. */
u8 eft037_joints_m01_a00[] = {1, 2, 0xFF};
u8 eft037_joints_m01_a02[] = {1, 2, 0xFF};
u8 eft037_joints_m01_a03[] = {0, 0xFF};
u8 eft037_joints_m01_a04[] = {1, 2, 3, 4, 5, 6, 7, 0xFF};
u8 eft037_joints_m01_a05[] = {1, 2, 3, 0xFF};
u8 eft037_joints_m01_a07[] = {1, 2, 3, 4, 0xFF};
u8 eft037_joints_m01_a06[] = {0, 0xFF};
u8 eft037_joints_m01_a08[] = {0, 0xFF};
u8 eft037_joints_m01_a12[] = {0, 0xFF};
u8 eft037_joints_m03_a00[] = {1, 2, 0xFF};
u8 eft037_joints_m03_a02[] = {1, 2, 0xFF};
u8 eft037_joints_m03_a03[] = {1, 2, 3, 0xFF};
u8 eft037_joints_m03_a04[] = {0, 0xFF};
u8 eft037_joints_m03_a06[] = {0, 0xFF};
u8 eft037_joints_m03_a07[] = {1, 2, 3, 0xFF};
u8 eft037_joints_m03_a08[] = {1, 2, 3, 4, 0xFF};
u8 eft037_joints_m03_a10[] = {0, 0xFF};
u8 eft037_joints_m05_a06[] = {0, 0xFF};
u8 eft037_joints_m07_a03[] = {0, 0xFF};
u8 eft037_joints_m22_a00[] = {1, 2, 0xFF};
/* The models the move step leaves unplaced while the item list has row 1 selected (maps 21 and 22). */
u8 eft037_skip_m21_a05[] = {0, 1, 0xFF};
u8 eft037_skip_m21_a06[] = {0, 1, 0xFF};
u8 eft037_skip_m21_a07[] = {0, 1, 0xFF};
u8 eft037_skip_m22_a01[] = {0, 1, 0xFF};

u8* eft037_joints_m01[13] = {
    eft037_joints_m01_a00, NULL, eft037_joints_m01_a02, eft037_joints_m01_a03, eft037_joints_m01_a04,
    eft037_joints_m01_a05, eft037_joints_m01_a06, eft037_joints_m01_a07, eft037_joints_m01_a08, NULL, NULL, NULL,
    eft037_joints_m01_a12,
};
u8* eft037_joints_m03[11] = {
    eft037_joints_m03_a00, NULL, eft037_joints_m03_a02, eft037_joints_m03_a03, eft037_joints_m03_a04, NULL,
    eft037_joints_m03_a06, eft037_joints_m03_a07, eft037_joints_m03_a08, NULL, eft037_joints_m03_a10,
};

/* The model resource id per area: maps 1, 12, 3 and 14. */
u16 eft037_model_ids_m01[14] = {0x53, 0, 0x55, 0x57, 0x59, 0x5B, 0x5D, 0x5F, 0x61, 0, 0, 0, 0x63, 0};
u16 eft037_model_ids_m12[14] = {0x54, 0, 0x56, 0x58, 0x5A, 0x5C, 0x5E, 0x60, 0x62, 0, 0, 0, 0x64, 0};
u16 eft037_model_ids_m03[12] = {0x65, 0, 0x67, 0x69, 0x6B, 0, 0, 0x6F, 0x71, 0, 0x73, 0};
u16 eft037_model_ids_m14[16] = {0x66, 0, 0x68, 0x6A, 0x6C, 0, 0x6E, 0x70, 0x72, 0, 0, 0, 0, 0, 0, 0};

/* The model positions (x, y, z per model) per map and area; a y past +-40000 picks the model's height band. */
f32 eft037_pos_m01_a00[6] = {
    0.0f, 50000.0f, 0.0f,
    0.0f, -50000.0f, 0.0f,
};
f32 eft037_pos_m01_a02_k0[10] = {
    -1697.0f, 2.0f, -1638.0f,
    -814.0f, 1453.0f, -1655.0f,
    -1440.0f, 1375.0f, -1475.0f,
    0.0f,
};
f32 eft037_pos_m01_a02[6] = {
    0.0f, 60000.0f, 0.0f,
    -8000.0f, 500.0f, 1700.0f,
};
f32 eft037_pos_m01_a03[3] = {
    0.0f, 60000.0f, 0.0f,
};
f32 eft037_pos_m01_a04[21] = {
    0.0f, 60000.0f, 0.0f,
    -2000.0f, 1000.0f, -4200.0f,
    -2000.0f, 1000.0f, -3600.0f,
    6600.0f, 1000.0f, 2000.0f,
    6200.0f, 1000.0f, 2000.0f,
    5600.0f, 1000.0f, 1800.0f,
    5200.0f, 1000.0f, 1600.0f,
};
f32 eft037_pos_m01_a05[9] = {
    0.0f, 60000.0f, 0.0f,
    0.0f, 2000.0f, -12000.0f,
    -3000.0f, 2000.0f, -9000.0f,
};
f32 eft037_pos_m01_a06[3] = {
    0.0f, 60000.0f, 0.0f,
};
f32 eft037_pos_m01_a07[12] = {
    0.0f, 50000.0f, 0.0f,
    0.0f, -50000.0f, 0.0f,
    -3850.0f, -42000.0f, 3120.0f,
    -4880.0f, -41800.0f, 230.0f,
};
f32 eft037_pos_m01_a08[3] = {
    0.0f, 60000.0f, 0.0f,
};
f32 eft037_pos_m01_a09[3] = {
    -2460.0f, 1545.0f, -4704.0f,
};
f32 eft037_pos_m01_a10[6] = {
    -2525.0f, 2450.0f, 36.0f,
    -2520.0f, 2450.0f, 53.0f,
};
f32 eft037_pos_m01_a11[12] = {
    2000.0f, 560.0f, 1600.0f,
    2070.0f, 920.0f, 2920.0f,
    3174.0f, 1022.0f, 4109.0f,
    2945.0f, 1376.0f, 5519.0f,
};
f32 eft037_pos_m01_a12[3] = {
    0.0f, -50000.0f, 0.0f,
};
f32* eft037_pos_m01[13] = {
    eft037_pos_m01_a00, NULL, eft037_pos_m01_a02, eft037_pos_m01_a03, eft037_pos_m01_a04, eft037_pos_m01_a05,
    eft037_pos_m01_a06, eft037_pos_m01_a07, eft037_pos_m01_a08, eft037_pos_m01_a09, eft037_pos_m01_a10,
    eft037_pos_m01_a11, eft037_pos_m01_a12,
};
f32 eft037_pos_m02_a09[16] = {
    4734.0f, 342.0f, -995.0f,
    4150.0f, 500.0f, 820.0f,
    905.0f, 800.0f, 914.0f,
    2951.0f, 600.0f, 914.0f,
    5932.0f, 700.0f, 820.0f,
    0.0f,
};
f32 eft037_pos_m02_a11[12] = {
    -3000.0f, 1800.0f, 3000.0f,
    600.0f, 5200.0f, 2800.0f,
    -300.0f, 2560.0f, -2300.0f,
    3200.0f, 2700.0f, 950.0f,
};
f32 eft037_pos_m03_a00[6] = {
    0.0f, 60000.0f, 0.0f,
    0.0f, -50000.0f, 0.0f,
};
f32 eft037_pos_m03_a02[6] = {
    0.0f, 50000.0f, 0.0f,
    0.0f, -50000.0f, 0.0f,
};
f32 eft037_pos_m03_a03[9] = {
    0.0f, 50000.0f, 0.0f,
    0.0f, -50000.0f, 0.0f,
    7700.0f, -40600.0f, 2000.0f,
};
f32 eft037_pos_m03_a04[3] = {
    0.0f, -50000.0f, 0.0f,
};
f32 eft037_pos_m03_a06[3] = {
    0.0f, -50000.0f, 0.0f,
};
f32 eft037_pos_m03_a07[9] = {
    0.0f, 50000.0f, 0.0f,
    7300.0f, 500.0f, -500.0f,
    5700.0f, 500.0f, -500.0f,
};
f32 eft037_pos_m03_a08[12] = {
    0.0f, 50000.0f, 0.0f,
    0.0f, -50000.0f, 0.0f,
    0.0f, 41000.0f, -5000.0f,
    -3200.0f, 41000.0f, -2900.0f,
};
f32 eft037_pos_m03_a10[3] = {
    0.0f, 60000.0f, 0.0f,
};
f32* eft037_pos_m03[11] = {
    eft037_pos_m03_a00, NULL, eft037_pos_m03_a02, eft037_pos_m03_a03, eft037_pos_m03_a04, NULL,
    eft037_pos_m03_a06, eft037_pos_m03_a07, eft037_pos_m03_a08, NULL, eft037_pos_m03_a10,
};
f32 eft037_pos_m05_a02[12] = {
    1650.0f, 1700.0f, 60.0f,
    3140.0f, 1960.0f, -1470.0f,
    -1280.0f, 1800.0f, 886.0f,
    -3080.0f, 2560.0f, 880.0f,
};
f32 eft037_pos_m05_a06[3] = {
    0.0f, 50000.0f, 0.0f,
};
f32 eft037_pos_m06_a02[3] = {
    -5090.0f, 2214.0f, 1038.0f,
};
f32 eft037_pos_m07_a01[70] = {
    -9400.0f, -8590.0f, -9190.0f,
    -8950.0f, -8450.0f, -9620.0f,
    -8730.0f, -8550.0f, -9360.0f,
    -8210.0f, -8680.0f, -9500.0f,
    -7530.0f, -10920.0f, -7010.0f,
    -8360.0f, -9380.0f, -3550.0f,
    -9770.0f, -13850.0f, 5840.0f,
    -10960.0f, -14040.0f, 6340.0f,
    -11130.0f, -13300.0f, 5690.0f,
    -10230.0f, -13480.0f, 5230.0f,
    -11670.0f, -12210.0f, 6720.0f,
    -10930.0f, -10570.0f, 6160.0f,
    1600.0f, -15400.0f, 10670.0f,
    2140.0f, -15240.0f, 10010.0f,
    2160.0f, -15450.0f, 10420.0f,
    1540.0f, -15170.0f, 10260.0f,
    6760.0f, -16940.0f, 10950.0f,
    -9530.0f, -10720.0f, -11960.0f,
    -9530.0f, -10720.0f, -13500.0f,
    -9530.0f, -10720.0f, -15200.0f,
    11430.0f, -17910.0f, 10230.0f,
    12800.0f, -17910.0f, 10230.0f,
    14200.0f, -17910.0f, 10230.0f,
    0.0f,
};
f32 eft037_pos_m07_a02[72] = {
    16190.0f, -17950.0f, -5970.0f,
    16300.0f, -17250.0f, -2690.0f,
    14520.0f, -16820.0f, -1890.0f,
    19430.0f, -10860.0f, 8840.0f,
    18260.0f, -11140.0f, 12540.0f,
    19950.0f, -12280.0f, 11220.0f,
    19350.0f, -11220.0f, 13570.0f,
    17890.0f, -10290.0f, 7460.0f,
    15950.0f, -13040.0f, 9990.0f,
    16800.0f, -12160.0f, 11400.0f,
    7970.0f, -21690.0f, 13750.0f,
    7080.0f, -22620.0f, 11830.0f,
    5610.0f, -23640.0f, 12170.0f,
    4530.0f, -24330.0f, 11230.0f,
    2610.0f, -25970.0f, 9870.0f,
    1220.0f, -25760.0f, 9040.0f,
    230.0f, -27180.0f, 7270.0f,
    18900.0f, -17900.0f, -10300.0f,
    19600.0f, -17900.0f, -11200.0f,
    20300.0f, -17900.0f, -12200.0f,
    -5740.0f, -30070.0f, 670.0f,
    -7280.0f, -30070.0f, -420.0f,
    -8750.0f, -30070.0f, -1560.0f,
    -9600.0f, -30070.0f, -2900.0f,
};
f32 eft037_pos_m07_a03[21] = {
    -7343.0f, -8313.0f, -3028.0f,
    -7029.0f, -7413.0f, -4846.0f,
    -7856.0f, -7827.0f, -910.0f,
    -5211.0f, -7791.0f, -832.0f,
    4434.0f, -14972.0f, 3286.0f,
    1324.0f, -13791.0f, 2220.0f,
    -3526.0f, -7413.0f, -1786.0f,
};
f32 eft037_pos_m07_a03_k2[3] = {
    0.0f, -50000.0f, 0.0f,
};
f32* eft037_pos_m07[4] = {NULL, eft037_pos_m07_a01, eft037_pos_m07_a02, eft037_pos_m07_a03};
f32 eft037_pos_m10_a00[3] = {
    1540.0f, 1700.0f, 1430.0f,
};
f32 eft037_pos_m21_a05[15] = {
    -44.4f, 171.2f, 154.1f,
    106.3f, 196.0f, 196.7f,
    177.7f, 206.2f, -437.1f,
    420.1f, 224.7f, -262.2f,
    800.0f, 200.0f, 0.0f,
};
f32 eft037_pos_m21_a06[6] = {
    393.3f, 70.9f, -414.0f,
    481.7f, 225.2f, -299.2f,
};
f32 eft037_pos_m21_a07[12] = {
    561.0f, 775.0f, -56.0f,
    716.4f, 551.3f, 62.2f,
    -119.2f, 253.8f, -282.4f,
    157.5f, 258.2f, -280.7f,
};
f32 eft037_pos_m22_a00[6];
f32 eft037_pos_m22_a01[24] = {
    -235.0f, 474.0f, 145.0f,
    -423.0f, 381.0f, 233.0f,
    9.0f, 505.0f, 53.0f,
    -5.0f, 668.0f, 40.0f,
    -6.0f, 170.0f, 172.0f,
    302.0f, 172.0f, 270.0f,
    351.0f, 119.0f, 403.0f,
    86.0f, 408.0f, 121.0f,
};
f32 eft037_pos_m22_a02[3] = {
    1107.0f, 688.0f, 772.0f,
};

/* Starts the area model effect: kind 0 takes the stage's own joint list for the current area, kinds 1-3 this
 * unit's per-map lists; pools one model per list entry. */
void em_sub_area_fx_set(u8 kind) {
    u8* joints;
    u8 area;
    s32 i;
    _EFT* self;
    Eft037Work* work;

    joints = NULL;
    area = get_now_areano();
    if (kind == 0) {
        joints = (u8*)stage_area_word_get(area);
    } else {
        switch (get_now_mapno()) {
        case 1:
        case 12:
            joints = eft037_joints_m01[area];
            break;
        case 3:
        case 14:
            joints = eft037_joints_m03[area];
            break;
        case 5:
        case 16:
            if (area == 6) {
                joints = eft037_joints_m05_a06;
            }
            break;
        case 7:
        case 18:
            if (area == 3) {
                joints = eft037_joints_m07_a03;
            }
            break;
        case 22:
            if (area == 0) {
                joints = eft037_joints_m22_a00;
            }
            break;
        }
    }
    if (joints != NULL) {
        self = (_EFT*)eft_res_slot_get(sizeof(Eft037Work));
        if (self != NULL) {
            self->dispatch_0x34 = eft037_dispatch;
            self->release_0x40 = eft037_models_release;
            work = (Eft037Work*)self->work_0x38;
            work->joints_0x304 = joints;
            work->count_0x000 = 0;
            while (*joints != 0xFF) {
                work->count_0x000++;
                joints++;
            }
            for (i = 0; i < work->count_0x000; i++) {
                work->models_0x004[i].model_0x00 = (MHchar*)eft_res_model_get();
                if (work->models_0x004[i].model_0x00 == NULL) {
                    eft_res_slot_release(self);
                    return;
                }
            }
            memset(work->works_0x124, 0, sizeof(work->works_0x124));
            self->field_0x03 = 37;
            eft_state_flags_set(self, 8, 0);
            self->area_0x44 = area;
            work->kind_0x308 = kind;
        }
    }
}

/* Returns every model's g3d works (last first) and its pooled model. */
void eft037_models_release(_EFT* self) {
    s32 i;
    s32 j;
    Eft037Work* work = (Eft037Work*)self->work_0x38;

    for (i = 0; i < work->count_0x000; i++) {
        for (j = 4; j >= 0; j--) {
            if (work->works_0x124[i][j] != NULL) {
                push_g3d_wk(work->works_0x124[i][j]);
            }
        }
        if (work->models_0x004[i].model_0x00 != NULL) {
            eft_res_models_release(&work->models_0x004[i].model_0x00, 1);
        }
    }
}

/* Steps the area effect's state: create, move, end, release. */
void eft037_dispatch(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        eft037_model_create(self);
        break;
    case 1:
        eft037_move(self);
        break;
    case 2:
        eft037_end(self);
        break;
    case 3:
        eft037_release(self);
        break;
    }
}

/* Builds every model's UV model for the map and area, places the models at the area's positions with only their
 * listed joint visible, then runs the first move. */
void eft037_model_create(_EFT* self) {
    VEC3 pos;
    u8 mapno;
    u32 i;
    f32* table;
    u8* joints;
    Eft037Work* work;
    u32 plain;
    u32 joint;
    char* name;
    u16 id;
    u32 joint_num;
    s32 k;

    table = NULL;
    VEC3_ctor(&pos);
    mapno = get_now_mapno();
    work = (Eft037Work*)self->work_0x38;
    plain = 0;
    if ((u32)(work->kind_0x308 - 1) > 2) {
        if (work->kind_0x308 == 0) {
        name = (char*)stage_char_res_body_get(0);
        strcat(name, ".brres");
        for (i = 0; i < work->count_0x000; i++) {
            work->models_0x004[i].uv_0x04 = (EftUvModelObject*)res_eft_UV_model_create_name(
                work->models_0x004[i].model_0x00, name, 264, 2, work->works_0x124[i], 4, 2);
            if (work->models_0x004[i].uv_0x04 == NULL) {
                eft037_release(self);
                return;
            }
        }
        }
    } else {
    switch (mapno) {
    case 1:
        id = eft037_model_ids_m01[self->area_0x44];
        break;
    case 12:
        id = eft037_model_ids_m12[self->area_0x44];
        break;
    case 3:
        id = eft037_model_ids_m03[self->area_0x44];
        break;
    case 14:
        id = eft037_model_ids_m14[self->area_0x44];
        break;
    case 5:
        if (self->area_0x44 == 6) {
            id = 116;
        }
        break;
    case 16:
        if (self->area_0x44 == 6) {
            id = 117;
        }
        break;
    case 7:
        if (self->area_0x44 == 3) {
            id = 118;
        }
        break;
    case 18:
        if (self->area_0x44 == 3) {
            id = 119;
        }
        break;
    case 22:
        if (self->area_0x44 == 0) {
            id = 120;
            plain = 1;
        }
        break;
    }
    for (i = 0; i < work->count_0x000; i++) {
        if (plain == 0) {
            work->models_0x004[i].uv_0x04 = (EftUvModelObject*)res_eft_UV_model_create(
                work->models_0x004[i].model_0x00, id, 300, 2, work->works_0x124[i], 4, 1);
            if (work->models_0x004[i].uv_0x04 == NULL) {
                eft037_release(self);
                return;
            }
        } else {
            if (res_eft_model_create(work->models_0x004[i].model_0x00, id, 44) == NULL) {
                eft037_release(self);
                return;
            }
        }
    }
    }
    self->state_0x05++;
    self->flag_0x01 = 1;
    self->timer_0x0C = 0;
    self->field_0x10 = 0;
    switch (mapno) {
    case 1:
    case 12:
        if (work->kind_0x308 == 0 && self->area_0x44 == 2) {
            table = eft037_pos_m01_a02_k0;
        } else {
            table = eft037_pos_m01[self->area_0x44];
        }
        break;
    case 2:
    case 13:
        switch (self->area_0x44) {
        case 9:
            table = eft037_pos_m02_a09;
            break;
        case 11:
            table = eft037_pos_m02_a11;
            break;
        }
        break;
    case 3:
    case 14:
        table = eft037_pos_m03[self->area_0x44];
        break;
    case 5:
    case 16:
        switch (self->area_0x44) {
        case 2:
            table = eft037_pos_m05_a02;
            break;
        case 6:
            table = eft037_pos_m05_a06;
            break;
        }
        break;
    case 6:
    case 17:
        if (self->area_0x44 == 2) {
            table = eft037_pos_m06_a02;
        }
        break;
    case 7:
    case 18:
        if (work->kind_0x308 == 2 && self->area_0x44 == 3) {
            table = eft037_pos_m07_a03_k2;
        } else {
            table = eft037_pos_m07[self->area_0x44];
        }
        break;
    case 10:
        switch (self->area_0x44) {
        case 0:
            table = eft037_pos_m10_a00;
            break;
        }
        break;
    case 21:
        switch (self->area_0x44) {
        case 5:
            table = eft037_pos_m21_a05;
            break;
        case 6:
            table = eft037_pos_m21_a06;
            break;
        case 7:
            table = eft037_pos_m21_a07;
            break;
        }
        break;
    case 22:
        switch (self->area_0x44) {
        case 0:
            table = eft037_pos_m22_a00;
            break;
        case 1:
            table = eft037_pos_m22_a01;
            break;
        case 2:
            table = eft037_pos_m22_a02;
            break;
        }
        break;
    }
    if (table == NULL) {
        eft037_release(self);
        return;
    }
    joints = work->joints_0x304;
    for (i = 0; i < work->count_0x000; i++) {
        setVector3(&work->models_0x004[i].model_0x00->scale_0x1C, 1.0f, 1.0f, 1.0f);
        work->models_0x004[i].model_0x00->field_0x28 = 0;
        work->models_0x004[i].model_0x00->field_0x2C = 0;
        work->models_0x004[i].model_0x00->field_0x28 = 0;
        vec_to_mh_vec3(&pos, (struct Vec*)&table[i * 3]);
        copyVec3(&work->models_0x004[i].model_0x00->pos_0x04, &pos);
        work->models_0x004[i].model_0x00->ready = 1;
        if (work->models_0x004[i].model_0x00->pos_0x04.y >= 60000.0f) {
            work->models_0x004[i].band_0x08 = 5;
            work->models_0x004[i].model_0x00->pos_0x04.y = 0.0f;
            work->models_0x004[i].model_0x00->ready = 2;
        } else if (work->models_0x004[i].model_0x00->pos_0x04.y <= -60000.0f) {
            work->models_0x004[i].band_0x08 = 6;
            work->models_0x004[i].model_0x00->pos_0x04.y = 0.0f;
            work->models_0x004[i].model_0x00->ready = 2;
        } else if (work->models_0x004[i].model_0x00->pos_0x04.y >= 50000.0f) {
            work->models_0x004[i].band_0x08 = 1;
            work->models_0x004[i].model_0x00->pos_0x04.y = 0.0f;
            work->models_0x004[i].model_0x00->ready = 2;
        } else if (work->models_0x004[i].model_0x00->pos_0x04.y <= -50000.0f) {
            work->models_0x004[i].band_0x08 = 2;
            work->models_0x004[i].model_0x00->pos_0x04.y = 0.0f;
            work->models_0x004[i].model_0x00->ready = 2;
        } else if (work->models_0x004[i].model_0x00->pos_0x04.y >= 40000.0f) {
            work->models_0x004[i].band_0x08 = 3;
            work->models_0x004[i].model_0x00->pos_0x04.y -= 40000.0f;
        } else if (work->models_0x004[i].model_0x00->pos_0x04.y <= -40000.0f) {
            work->models_0x004[i].band_0x08 = 4;
            work->models_0x004[i].model_0x00->pos_0x04.y -= -40000.0f;
        } else {
            work->models_0x004[i].band_0x08 = 0;
        }
        joint_num = work->models_0x004[i].model_0x00->get_joint_num();
        for (joint = 0; joint < joint_num; joint++) {
            if (*joints != joint) {
                work->models_0x004[i].model_0x00->setVisibility(joint, false);
            }
        }
        joints++;
    }
    for (k = 0; k < 9; k++) {
        work->phase_0x30A[k] = (k & 1) * 25;
    }
    eft037_move(self);
}

/* Each frame: cycles kind 2's material alpha, fades kind 3 by the camera's distance from its point, steps every
 * listed model and spawns it, lifted or dropped by its height band when the camera is on the matching side of the
 * water. */
void eft037_move(_EFT* self) {
    Eft037Work* work;
    u8* skip;
    u8 mapno;
    VEC3 offset;
    VEC3 center;
    VEC3 cam_pos;
    u32 near;
    u32 i;
    u32 j;
    _GXColor color;
    VEC3* lift;
    f32 radius;
    u32 fade_frames;

    work = (Eft037Work*)self->work_0x38;
    skip = NULL;
    mapno = get_now_mapno();
    VEC3_ctor(&offset);
    VEC3_ctor(&center);
    VEC3_ctor(&cam_pos);
    near = 0;
    fade_frames = 15;
    if (getItemListSelection() == 1) {
        if (mapno == 21) {
            switch (self->area_0x44) {
            case 5:
                skip = eft037_skip_m21_a05;
                break;
            case 6:
                skip = eft037_skip_m21_a06;
                break;
            case 7:
                skip = eft037_skip_m21_a07;
                break;
            }
        } else if (mapno == 22) {
            switch (self->area_0x44) {
            case 1:
                skip = eft037_skip_m22_a01;
                break;
            }
        }
    }
    if (work->kind_0x308 == 2) {
        for (j = 0; j < 9; j++) {
            color.a = 255.0f * eft037_sin(6.2831855f * (180.0f * (work->phase_0x30A[j] / 120.0f)) / 360.0f);
            work->models_0x004[0].model_0x00->setMatColor(j, GX_ALPHA0, color, false);
            work->phase_0x30A[j]++;
            if (work->phase_0x30A[j] > 120) {
                work->phase_0x30A[j] = 0;
            }
        }
    }
    if (work->kind_0x308 == 3) {
        VEC3 point;
        copyVec3(&center, setVec3(&point, -300.0f, 550.0f, 240.0f));
        copyVec3(&cam_pos, &get_camera_pos());
        if (camera_mode_cur_get() == 1) {
            radius = 600.0f;
        } else {
            radius = 300.0f;
        }
        if (hit_point_sphr(&cam_pos, &center, radius) == 1) {
            near = 1;
        }
        if (near) {
            if (self->timer_0x0C > 0) {
                self->timer_0x0C--;
            }
        } else {
            if (self->timer_0x0C < 15) {
                self->timer_0x0C++;
            }
        }
    }
    for (i = 0; i < work->count_0x000; i++) {
        if (skip != NULL && *skip != 0xFF) {
            if (i == *skip) {
                skip++;
                continue;
            }
            skip++;
        }
        if (work->kind_0x308 == 3) {
            for (j = 0; j < mhchar_mat_num_get(work->models_0x004[i].model_0x00); j++) {
                color.a = 255.0f * ((f32)self->timer_0x0C / (f32)fade_frames);
                work->models_0x004[i].model_0x00->setMatColor(j, GX_ALPHA0, color, false);
            }
        }
        work->models_0x004[i].model_0x00->move(0);
        if (work->models_0x004[i].uv_0x04 != NULL) {
            work->models_0x004[i].uv_0x04->update();
        }
        lift = NULL;
        switch (work->models_0x004[i].band_0x08) {
        case 1:
            if (camera_underwater_ck() == 1) {
                continue;
            }
            setVector3(&offset, 0.0f, 1000.0f, 0.0f);
            lift = &offset;
            break;
        case 2:
            if (camera_underwater_ck() == 0) {
                continue;
            }
            setVector3(&offset, 0.0f, -1000.0f, 0.0f);
            lift = &offset;
            break;
        case 3:
            if (camera_underwater_ck() == 1) {
                continue;
            }
            break;
        case 4:
            if (camera_underwater_ck() == 0) {
                continue;
            }
            break;
        case 5:
            setVector3(&offset, 0.0f, 1000.0f, 0.0f);
            lift = &offset;
            break;
        case 6:
            setVector3(&offset, 0.0f, -1000.0f, 0.0f);
            lift = &offset;
            break;
        }
        eft_res_models_spawn(self, (void**)&work->models_0x004[i].model_0x00, 2, 1, lift);
    }
}

/* The sine the colour phase reads. */
f32 eft037_sin(f32 x) {
    return sinf(x);
}

/* Advances the effect to its end state. */
void eft037_end(_EFT* self) {
    self->state_0x05++;
}

/* Releases the effect's pooled slot. */
void eft037_release(_EFT* self) {
    eft_res_slot_release(self);
}

/* Starts the camera screen filter `kind` (type 0x24) with one pooled model. */
void em_sub_area_fx2_set(u8 kind) {
    s32 i;
    _EFT* self;
    Eft036SpawnWork* work;

    self = (_EFT*)eft_res_slot_get(sizeof(Eft036SpawnWork));
    if (self != NULL) {
        self->dispatch_0x34 = eft036_dispatch;
        self->release_0x40 = eft036_models_release;
        work = (Eft036SpawnWork*)self->work_0x38;
        work->count_0x00 = 1;
        for (i = 0; i < work->count_0x00; i++) {
            work->models_0x04[i] = (MHchar*)eft_res_model_get();
            if (work->models_0x04[i] == NULL) {
                eft_res_slot_release(self);
                return;
            }
        }
        memset(work->works_0x0C, 0, sizeof(work->works_0x0C));
        self->field_0x03 = 36;
        eft_state_flags_set(self, 8, 0);
        work->kind_0x14 = kind;
    }
}

/* Returns the screen filter's g3d works (last first) and its pooled models. */
void eft036_models_release(_EFT* self) {
    s32 i;
    s32 j;
    Eft036SpawnWork* work = (Eft036SpawnWork*)self->work_0x38;

    for (i = 0; i < work->count_0x00; i++) {
        for (j = 1; j >= 0; j--) {
            if (work->works_0x0C[i][j] != NULL) {
                push_g3d_wk(work->works_0x0C[i][j]);
            }
        }
        eft_res_models_release(&work->models_0x04[i], 1);
    }
}

/* Steps the screen filter's state: create, move, end, release. */
void eft036_dispatch(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        eft036_model_create((_ENEMY_WORK*)self);
        break;
    case 1:
        eft036_move((_ENEMY_WORK*)self);
        break;
    case 2:
        eft036_end((_ENEMY_WORK*)self);
        break;
    case 3:
        eft036_release((_ENEMY_WORK*)self);
        break;
    }
}

} /* extern "C" */
