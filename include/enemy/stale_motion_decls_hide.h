/* enemy/stale_motion_decls_hide.h - include once before and once after an `#include` of an old header that still declares
 * the two renamed motion helpers under their former `fn_<addr>` names, to keep those stale, mutually clashing declarations out of
 * the translation unit (`include/unsplit/enemy.h` and `include/enemy/fn_80165FC8.h` spell them `u32 (void)` and `u32 (_ENEMY_WORK*)`;
 * nothing calls the old names any more: the helpers are `em_mot_finished_ck` / `em_motion_param_set`, see `enemy/em_mot_finished_ck.h`
 * and `enemy/em_motion_param_set.h`).  No include guard on purpose: the first include defines, the second undefines. */
#ifndef MHTRI_ENEMY_STALE_MOTION_HIDE_ACTIVE
#define MHTRI_ENEMY_STALE_MOTION_HIDE_ACTIVE
#define fn_8012ECF0 fn_8012ECF0_stale_decl
#define fn_8012FCC4 fn_8012FCC4_stale_decl
#else
#undef fn_8012FCC4
#undef fn_8012ECF0
#undef MHTRI_ENEMY_STALE_MOTION_HIDE_ACTIVE
#endif
