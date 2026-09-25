# Relocation audit: undefined-symbol sweep over landed units

Snapshot: MAIN HEAD `f7b49ff090edca83efabdaf7bba67898f7070d05`, `configure.py` sha1 `f66865c281722fc246ecfbea5f58119534784429`.

Command: `python tools/units/relocaudit.py --main <MAIN>`.

Result: **103 registered units, 102 built, 76 clean, 26 suspect** (internal 0.19 s, wall 0.20 s). Suspects with an undefined-set disagreement: 25; with a defined-set disagreement: 2.

Unbuilt (no object): `g3d/g3d_anmchr.cpp`.

`fn_80059550` (the unit the defect was found and repaired in) is **clean** here: its object's
undefined set now matches the target's spelling for all seven symbols.

Excluded, deliberately: locals (`STB_LOCAL`, `...data.N`, `.L...`), `@NNN`/`@etb_*` compiler
labels, `STT_FILE` (`ef_cube.cpp`), `.comment`, and all section/extab naming - none is linkage, and
the metric already covers the sections. EABI `_savegpr_N`/`_restgpr_N` rows are reported but tagged
as register allocation, not linkage.

## 1. Wrong-linkage repair list (the actionable set)

Our object emits the left spelling; the target object emits the right one. The fix is to declare the
callee in its owner header with its true linkage. `relocaudit.py` prints the exact src/header rows
that spell each identifier today.

| unit | our object emits | target emits |
| --- | --- | --- |
| `sound/fn_800D7F54.cpp` | `em_act_ck` | `em_act_ck__FP11_ENEMY_WORKUcUc` |
| `sound/fn_800DCFEC.c` | `get_now_areano` | `get_now_areano__Fv` |
| `sound/fn_800DCFEC.c` | `get_now_mapno` | `get_now_mapno__Fv` |
| `sound/fn_800EF7D8.cpp` | `PlayStream` | `PlayStream__FUlUl` |
| `sound/fn_800EF7D8.cpp` | `load_file__FPcUlUl` | `load_file__FPcUll` |
| `sound/fn_800EF7D8.cpp` | `set_stream_main_vol_flag` | `set_stream_main_vol_flag__FUcUc` |
| `sound/fn_800F2A94.cpp` | `PlayStream` | `PlayStream__FUlUl` |
| `ef/ef_cube.cpp` | `fn_80043EA8__FP4Vec3` | `fn_80043EA8` |
| `ef/ef_cube.cpp` | `fn_80050EDC__FPC4Vec3` | `fn_80050EDC` |
| `ef/ef_cube.cpp` | `fn_80051490__FP4Vec3PC4Vec3` | `fn_80051490` |
| `ef/ef_cube.cpp` | `fn_8009C484__FP4Vec3P4Vec3` | `fn_8009C484` |
| `ef/ef_cube.cpp` | `fn_800A8A08__FPCv` | `fn_800A8A08` |
| `ef/ef_cube.cpp` | `fn_800A99B4__FUiP4Vec3PvP4Vec3P4Vec3P4Vec3P4Vec3` | `fn_800A99B4` |
| `ef/ef_cube.cpp` | `fn_800A9FB0__FUiUsfPv` | `fn_800A9FB0` |
| `ef/ef_cylinder.cpp` | `fn_80041E8C__FP3Vecfff` | `fn_80041E8C` |
| `ef/ef_cylinder.cpp` | `fn_80043EA8__FPv` | `fn_80043EA8` |
| `ef/ef_cylinder.cpp` | `fn_80050BC0__Fff` | `fn_80050BC0` |
| `ef/ef_cylinder.cpp` | `fn_80051490__FP3VecP3Vec` | `fn_80051490` |
| `ef/ef_cylinder.cpp` | `fn_8009C484__FP3VecP3Vec` | `fn_8009C484` |
| `ef/ef_cylinder.cpp` | `fn_8009C760__FPfPff` | `fn_8009C760` |
| `ef/ef_cylinder.cpp` | `fn_800A8A08__FPv` | `fn_800A8A08` |
| `ef/ef_cylinder.cpp` | `fn_800A99B4__FlP3VecP6EfWorkP3VecP3VecP3VecP3Vec` | `fn_800A99B4` |
| `ef/ef_cylinder.cpp` | `fn_800A9FB0__FlUsP6EfWorkf` | `fn_800A9FB0` |
| `ef/ef_cylinder.cpp` | `fn_80463F10__Fff` | `fn_80463F10` |
| `ef/ef_emitterform.cpp` | `fn_8005050C__FPv` | `fn_8005050C` |
| `ef/ef_emitterform.cpp` | `fn_800513CC__FPvPCvPCv` | `fn_800513CC` |
| `ef/ef_emitterform.cpp` | `fn_80051424__FPvPCv` | `fn_80051424` |
| `ef/ef_emitterform.cpp` | `fn_800514FC__FPvPCvPCv` | `fn_800514FC` |
| `ef/ef_emitterform.cpp` | `fn_8009CA30__FPvfff` | `fn_8009CA30` |
| `ef/ef_point.cpp` | `fn_80041E8C__FP6EfVec3fff` | `fn_80041E8C` |
| `ef/ef_point.cpp` | `fn_80043EA8__FP6EfVec3` | `fn_80043EA8` |
| `ef/ef_point.cpp` | `fn_80050BC0__Ff` | `fn_80050BC0` |
| `ef/ef_point.cpp` | `fn_8009C484__FP6EfVec3P6EfVec3` | `fn_8009C484` |
| `ef/ef_point.cpp` | `fn_8009C760__FPfPff` | `fn_8009C760` |
| `ef/ef_point.cpp` | `fn_800A8A08__FP6EfRate` | `fn_800A8A08` |
| `ef/ef_point.cpp` | `fn_800A99B4__FPvP6EfVec3P9EfEmitterP6EfVec3P6EfVec3P6EfVec3P6EfVec3` | `fn_800A99B4` |
| `ef/ef_point.cpp` | `fn_800A9FB0__FPvUsP9EfEmitterf` | `fn_800A9FB0` |
| `ef/fn_800CDB2C.cpp` | `TPLtexLoad` | `TPLtexLoad__FPvP9_tex_info` |
| `ef/eft001.cpp` | `fn_80135998__FPv` | `fn_80135998` |
| `ef/eft001.cpp` | `get_joint_wpos_em__FPvUlPQ34nw4r4math4VEC3` | `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3` |
| `ef/eft_res.cpp` | `getResMemAdrs` | `getResMemAdrs__Fl` |
| `ef/eft_res.cpp` | `get_move_work_adrs` | `get_move_work_adrs__FUc` |
| `ef/eft_res.cpp` | `get_move_work_max` | `get_move_work_max__FUc` |
| `ef/eft_res.cpp` | `get_now_areano` | `get_now_areano__Fv` |
| `ef/eft_res.cpp` | `load_file_req` | `load_file_req__FPcUllUllPUl` |
| `ef/eft_res.cpp` | `nwAddResource` | `nwAddResource__FPcPv` |
| `ef/eft_res.cpp` | `pull_res_mem` | `pull_res_mem__FPcUll` |
| `ef/eft_res.cpp` | `push_g3d_wk` | `push_g3d_wk__FP9_g3d_work` |
| `ef/effect.cpp` | `Panic` | `Panic__Q24nw4r2dbFPCciPCce` |
| `ef/effect.cpp` | `load_file` | `load_file__FPcUll` |
| `ef/effect.cpp` | `ran_suu` | `ran_suu__Fl` |
| `ef/effect.cpp` | `work_mem_alloc` | `work_mem_alloc__FUl` |
| `ef/effect.cpp` | `work_mem_free` | `work_mem_free__FPv` |
| `ef/eft002.cpp` | `fn_8006F304__FPvRCUl` | `fn_8006F304` |
| `ef/fn_800FD864.cpp` | `fn_800FE978__FP4_EFT` | `fn_800FE978` |
| `ef/fn_800FE978.cpp` | `cpSetRotMatrix` | `cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34` |
| `ef/fn_800FE978.cpp` | `fn_802FB8EC__FUc` | `fn_802FB8EC` |
| `ef/fn_800FE978.cpp` | `load_file` | `load_file__FPcUll` |
| `ef/fn_800FE978.cpp` | `ran_suu` | `ran_suu__Fl` |
| `ef/fn_800FE978.cpp` | `work_mem_alloc` | `work_mem_alloc__FUl` |
| `ef/fn_800FE978.cpp` | `work_mem_free` | `work_mem_free__FPv` |
| `ef/eft004.cpp` | `get_now_areano` | `get_now_areano__Fv` |
| `ef/eft007.cpp` | `cpSetRotMatrix` | `cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34` |
| `ef/eft007.cpp` | `eftGetKeyAlpha` | `eftGetKeyAlpha__FPUcl` |
| `ef/eft007.cpp` | `get_now_areano` | `get_now_areano__Fv` |
| `ef/eft007.cpp` | `get_now_mapno` | `get_now_mapno__Fv` |
| `ef/eft007.cpp` | `get_stg_eft_col` | `get_stg_eft_col__FUcUc` |
| `ef/eft007.cpp` | `vec_to_mh_vec3` | `vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec` |
| `ef/fn_80105314.cpp` | `change_color_eff__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3P8_GXColor` | `change_color_eff__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC38_GXColor` |
| `ef/fn_80105314.cpp` | `cpSetRotMatrix` | `cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34` |
| `ef/fn_80105314.cpp` | `fn_80041E40__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3` | `fn_80041E40` |
| `ef/fn_80105314.cpp` | `get_joint_wpos_em` | `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3` |
| `ef/fn_80105314.cpp` | `get_now_areano` | `get_now_areano__Fv` |
| `ef/fn_80105314.cpp` | `get_stg_eft_col` | `get_stg_eft_col__FUcUc` |
| `ef/fn_80105314.cpp` | `push_g3d_wk` | `push_g3d_wk__FP9_g3d_work` |
| `ef/fn_80105314.cpp` | `res_eft_model_create` | `res_eft_model_create__FP6MHcharUsUl` |
| `ef/eft019.cpp` | `cpSetRotMatrix` | `cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34` |
| `ef/eft019.cpp` | `event_demo_ck` | `event_demo_ck__Fv` |
| `ef/eft019.cpp` | `fn_800A4420__Fl` | `fn_800A4420` |
| `ef/eft019.cpp` | `fn_800A51D8__FPQ34nw4r2ef6EffectUl` | `fn_800A51D8` |
| `ef/eft019.cpp` | `fn_800A67E8__FPvPv` | `fn_800A67E8` |
| `ef/eft019.cpp` | `fn_800A970C__Fv` | `fn_800A970C` |
| `ef/eft019.cpp` | `fn_800A9714__FPvUs` | `fn_800A9714` |
| `ef/eft019.cpp` | `get_camera_direction__FPQ34nw4r4math4VEC3` | `get_camera_direction__Fv` |
| `ef/eft019.cpp` | `get_now_areano` | `get_now_areano__Fv` |
| `ef/eft019.cpp` | `get_stg_eft_col` | `get_stg_eft_col__FUcUc` |
| `ef/fn_80114E34.cpp` | `Get_motion_no` | `Get_motion_no__FP4_PLW` |
| `ef/fn_80114E34.cpp` | `Pl_master_ck` | `Pl_master_ck__FP4_PLW` |
| `ef/fn_80114E34.cpp` | `eftGetKeyRGB` | `eftGetKeyRGB__FPUclPUcPUcPUc` |
| `ef/fn_80114E34.cpp` | `em_work_die_ck` | `em_work_die_ck__FP11_ENEMY_WORK` |
| `ef/fn_80114E34.cpp` | `get_joint_wpos_em` | `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3` |
| `ef/fn_80114E34.cpp` | `get_now_areano` | `get_now_areano__Fv` |
| `ef/fn_80114E34.cpp` | `get_now_mapno` | `get_now_mapno__Fv` |
| `ef/fn_80114E34.cpp` | `res_eft_model_create` | `res_eft_model_create__FP6MHcharUsUl` |
| `ef/fn_80114E34.cpp` | `res_eft_model_create_light` | `res_eft_model_create_light__FP6MHcharUsUll` |
| `ef/fn_80114E34.cpp` | `vec_to_mh_vec3` | `vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec` |
| `fn_80056F24.cpp` | `fn_80055EC4__FPv` | `fn_80055EC4` |
| `fn_80056F24.cpp` | `fn_80055F58__Fv` | `fn_80055F58` |
| `fn_80056F24.cpp` | `fn_80056E1C__FPv` | `fn_80056E1C` |
| `fn_80056F24.cpp` | `fn_80056F24__Fv` | `fn_80056F24` |
| `fn_80056F24.cpp` | `fn_800579A4__Fv` | `fn_800579A4` |

## 2. Undefined references with no target spelling (not linkage, but hidden by the metric)

| kind | unit | our object emits |
| --- | --- | --- |
| compiler-helper | `sound/fn_800E3CBC.cpp` | `_restgpr_20` |
| compiler-helper | `sound/fn_800E3CBC.cpp` | `_savegpr_20` |
| compiler-helper | `sound/fn_800EF7D8.cpp` | `_restgpr_17` |
| compiler-helper | `sound/fn_800EF7D8.cpp` | `_restgpr_19` |
| compiler-helper | `sound/fn_800EF7D8.cpp` | `_restgpr_21` |
| compiler-helper | `sound/fn_800EF7D8.cpp` | `_savegpr_17` |
| compiler-helper | `sound/fn_800EF7D8.cpp` | `_savegpr_19` |
| compiler-helper | `sound/fn_800EF7D8.cpp` | `_savegpr_21` |
| extra | `ef/ef_particlemanager.cpp` | `fn_80501EE0` |
| extra | `ef/ef_drawsmoothstripestrategy.cpp` | `GXSetTexCoordGen2` |
| extra | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C8A80` |
| extra | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C8B9C` |
| extra | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C8CB8` |
| extra | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C8DE4` |
| extra | `ef/ef_drawsmoothstripestrategy.cpp` | `lbl_80594A40` |
| extra | `ef/ef_drawsmoothstripestrategy.cpp` | `lbl_80594A70` |
| extra | `ef/ef_drawsmoothstripestrategy.cpp` | `lbl_80594A7C` |
| extra | `ef/ef_drawsmoothstripestrategy.cpp` | `lbl_80594AAC` |
| compiler-helper | `ef/eft_res.cpp` | `_restgpr_23` |
| compiler-helper | `ef/eft_res.cpp` | `_savegpr_23` |
| compiler-helper | `ef/fn_800FE978.cpp` | `_restgpr_21` |
| compiler-helper | `ef/fn_800FE978.cpp` | `_savegpr_21` |
| compiler-helper | `ef/fn_8010BDE4.cpp` | `_restgpr_19` |
| compiler-helper | `ef/fn_8010BDE4.cpp` | `_restgpr_24` |
| compiler-helper | `ef/fn_8010BDE4.cpp` | `_restgpr_25` |
| compiler-helper | `ef/fn_8010BDE4.cpp` | `_savegpr_19` |
| compiler-helper | `ef/fn_8010BDE4.cpp` | `_savegpr_24` |
| compiler-helper | `ef/fn_8010BDE4.cpp` | `_savegpr_25` |
| compiler-helper | `ef/fn_80114E34.cpp` | `_restgpr_15` |
| compiler-helper | `ef/fn_80114E34.cpp` | `_restgpr_21` |
| compiler-helper | `ef/fn_80114E34.cpp` | `_savegpr_15` |
| compiler-helper | `ef/fn_80114E34.cpp` | `_savegpr_21` |
| extra | `RSO/runtime.c` | `lbl_80629B90` |

## 3. Defined symbols the target does not define under that spelling

`linkage` rows are repaired like section 1; `stray` rows are definitions whose function belongs to
another unit (the ~44 `ef_drawsmoothstripestrategy.cpp` rows are all of the latter).

| kind | unit | our object defines | target |
| --- | --- | --- | --- |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C68B8` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C68C0` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C68C8` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C68D0` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C68D8` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C68E0` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C6F90` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C6F94` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C6FA4` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C6FB4` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C6FC0` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C6FD0` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C6FE0` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C6FF0` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C7000` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C7010` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C7028` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C7040` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C7058` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C7070` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C7080` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C7090` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C70B8` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C70D8` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C70E8` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C70F8` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C710C` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C712C` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C714C` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C715C` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C716C` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C717C` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C7194` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C71AC` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C71C4` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C71DC` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C71F0` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C7210` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C7230` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C7250` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C8674` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C8954` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C89D0` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C8A48` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C8A64` | - |
| stray | `ef/ef_drawsmoothstripestrategy.cpp` | `fn_800C9434` | - |
| linkage | `ef/fn_800CDB2C.cpp` | `TPLtexLoad` | TPLtexLoad__FPvP9_tex_info |
