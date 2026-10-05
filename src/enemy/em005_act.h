/* The enemy unit `enemy/em005_act.cpp` (the em005 TU, 0x801CA8DC..0x801D71C4; the header was
 * `enemy/fn_801D428C.h` until that unit folded in): the enemy action band's
 * per-motion dispatchers and helpers.
 *
 * `fn_801D6694` moved here from `enemy/fn_801B0010.cpp` when this unit landed (docs/plan.md 6.5
 * rule 2: an extern lives with the TU that owns the symbol).  Its previous home was the band header
 * `unsplit/enemy.h`, whose comment said the bracketing registered units named different
 * modules - true until this unit was registered.
 */
#ifndef MHTRI_ENEMY_FN_801CCBC4_H
#define MHTRI_ENEMY_FN_801CCBC4_H

#include "types.h"

struct _ENEMY_WORK;

/* The shared tables `enemy/em007_act.cpp` reads as well; they are declared once here. */
extern u8 lbl_805B52D8[];
extern u8 lbl_805B5310[];
extern u8 lbl_805B531C[];
extern f32 lbl_80799264;
extern f32 lbl_80799270;
extern f32 lbl_80799274;
extern f32 lbl_80799280;
extern f32 lbl_80799288;
extern f32 lbl_8079928C;
extern f32 lbl_80799294;
extern f32 lbl_807992B0;
extern f32 lbl_807992BC;
extern f32 lbl_807992C0;
extern f32 lbl_807992E8;
extern f32 lbl_807992F4;
extern f32 lbl_80799308;
extern f32 lbl_8079930C;
extern f32 lbl_80799310;
extern f32 lbl_80799318;
extern f32 lbl_8079931C;
extern f32 lbl_80799324;
extern f32 lbl_80799348;
extern f32 lbl_8079934C;
extern f32 lbl_80799354;
extern f32 lbl_8079935C;
extern f32 lbl_80799368;
extern f32 lbl_8079936C;
extern f32 lbl_80799378;
extern f32 lbl_80799380;
extern f32 lbl_80799388;
extern f32 lbl_80799390;
extern f32 lbl_8079939C;
extern f32 lbl_807993A0;
extern f32 lbl_807993A4;
extern f32 lbl_807993AC;
extern f32 lbl_807993E8;
extern f32 lbl_807993EC;
extern f32 lbl_807993F0;
extern f32 lbl_807993F4;
extern f32 lbl_807993F8;
extern f32 lbl_807993FC;
extern f32 lbl_80799400;
extern f32 lbl_80799404;
extern f32 lbl_80799408;
extern f32 lbl_8079940C;
extern f32 lbl_80799410;
extern f32 lbl_80799414;
extern f32 lbl_80799418;
extern f32 lbl_8079941C;
extern f32 lbl_80799420;
extern f32 lbl_80799424;
extern f32 lbl_80799428;
extern f32 lbl_8079942C;
extern f32 lbl_80799430;
extern f32 lbl_80799434;
extern f32 lbl_80799438;
extern f32 lbl_8079943C;
extern f32 lbl_80799440;
extern f32 lbl_80799444;
extern f32 lbl_80799448;
extern f32 lbl_8079944C;
extern f32 lbl_80799450;
extern f32 lbl_80799454;
extern f32 lbl_80799458;
extern f32 lbl_8079945C;
extern f32 lbl_80799460;
extern f32 lbl_80799464;
extern f32 lbl_80799468;
extern f32 lbl_8079946C;
extern f32 lbl_80799470;
extern f32 lbl_80799474;
extern f32 lbl_80799478;
extern f32 lbl_8079947C;
extern f32 lbl_80799480;
extern f32 lbl_80799484;
extern f32 lbl_80799488;
extern f32 lbl_8079948C;
extern f32 lbl_80799490;
extern f32 lbl_80799494;
extern f32 lbl_80799498;
extern f32 lbl_8079949C;
extern f32 lbl_807994A0;
extern f32 lbl_807994A4;
extern f32 lbl_807994A8;
extern f32 lbl_807994AC;
extern f32 lbl_807994B0;
extern f32 lbl_807994B4;
extern f32 lbl_807994B8;
extern f32 lbl_807994BC;
extern f32 lbl_807994C0;
extern f32 lbl_807994C4;
extern f32 lbl_807994C8;
extern f32 lbl_807994CC;
extern f32 lbl_807994D0;
extern f32 lbl_807994D4;
extern f32 lbl_807994D8;
extern f32 lbl_807994DC;
extern f32 lbl_807994E0;
extern f32 lbl_807994E4;
extern f32 lbl_807994E8;
extern f32 lbl_807994EC;
extern f32 lbl_807994F0;

/* The `.sdata2` float pool this unit and `enemy/em007_act.cpp` (one TU's code split across the units) both read.
 * Declared, never defined: the pool belongs to the data pass. */
extern f32 lbl_80799220;
extern f32 lbl_80799224;
extern f32 lbl_80799228;
extern f32 lbl_8079922C;
extern f32 lbl_80799230;
extern f32 lbl_80799234;
extern f32 lbl_80799238;
extern f32 lbl_8079923C;
extern f32 lbl_80799240;
extern f32 lbl_80799244;
extern f32 lbl_80799248;
extern f32 lbl_8079924C;
extern f32 lbl_80799250;
extern f32 lbl_80799254;
extern f32 lbl_80799258;
extern f32 lbl_8079925C;
extern f32 lbl_80799260;
extern f32 lbl_807992A0;
extern f32 lbl_807992A4;
extern f32 lbl_807992A8;

#ifdef __cplusplus
extern "C" {
#endif

/* r3 the work record; the per-motion dispatchers this unit defines (0x801CB308, 0x801CB9DC) and `enemy/em007_act.cpp`
 * tail-calls. */
void fn_801CB308(struct _ENEMY_WORK* self);
void fn_801CB9DC(struct _ENEMY_WORK* self);
/* r3 the work record; answers in r3 (its callers compare the result against 1). */
u32 fn_801D6694(_ENEMY_WORK* work);

#ifdef __cplusplus
}
#endif

#endif
