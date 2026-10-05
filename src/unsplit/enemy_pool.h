/* unsplit/enemy_pool.h - the enemy band's `.data` pools (strings, float tables, byte tables) at
 * 0x805A1078 and up that no registered unit owns yet: declared, never defined (playbook 29).  Kept
 * apart from `unsplit/enemy.h` so a unit that only reads a pool does not take that header's callee
 * declarations with it. */
#ifndef MHTRI_UNSPLIT_ENEMY_POOL_H
#define MHTRI_UNSPLIT_ENEMY_POOL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
extern u8 lbl_805A1078[];
extern const char* const lbl_805A1358[];
extern const char lbl_805A1398[];
extern const char lbl_805A13A4[];
extern const char lbl_805A13C0[];
extern const char lbl_805A13D4[];
extern const char lbl_805A13FC[];
extern const char lbl_805A1410[];
extern const char lbl_805A1420[];
extern const char lbl_805A143C[];
extern const char lbl_805A1450[];
extern const char lbl_805A146C[];
extern const char lbl_805A1480[];
extern const char lbl_805A14C4[];
extern const char lbl_805A14D8[];
extern const char lbl_805A151C[];
extern u8 lbl_805A1530[];
extern u8 lbl_805A1ADC[];
extern s8 lbl_805A1B08[];
extern u32 lbl_805A1B34[];
extern f32 lbl_805A1CC8[];
extern f32 lbl_805A1D18[];
extern f32 lbl_805A1D40[];
extern f32 lbl_805A1D88[];
extern f32 lbl_805A1DD8[];
extern f32 lbl_805A1E20[];
extern f32 lbl_805A1E50[];
extern f32 lbl_805A1E90[];
extern f32 lbl_805A1F00[];
extern f32 lbl_805A1F98[];
extern f32 lbl_805A1FF0[];
extern f32 lbl_805A2028[];
extern f32 lbl_805A2078[];
extern f32 lbl_805A20A0[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_ENEMY_POOL_H */
