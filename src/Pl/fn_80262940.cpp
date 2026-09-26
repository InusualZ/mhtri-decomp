/*
 * Player main/control cluster (proposal 80262940): .text 0x80262940-0x802693C4 (59 functions, 0x6A84 B)
 * with its own exception tables - extab 0x8001241C-0x80012554, extabindex 0x8002F514-0x8002F6E8.
 *
 * Registration (docs/plan.md 12, evidence class 3 + 4):
 *   - no `__FILE__` string covers the band (the .data source-name pool has none between
 *     `enemy_control.cpp` @0x805A1BB8 and `menu_item.cpp` @0x805CDFC8), so class 1 is out;
 *   - `dumpmap.py lookup` gives real names for 8 of the 59 symbols (`player_control_move`,
 *     `init_player_work`, `player_move_start`, `player_control_release`, `set_max_player`,
 *     `player_init_data_load`, `player_move_start_ck`, `parts_mdl_release_all`): the game's own
 *     player-control entry points;
 *   - the code is the `Pl` module (`_PLW` actors, `Pl_master_ck`/`Pl_Skill_ck`/`Pl_act_ck`/
 *     `Pl_cat_skill_ck`/`Pl_condition_ck` gates, `get_move_work_adrs`), and every sibling unit is
 *     `Pl/*.cpp`;
 *   - 51 of the 59 map entries carry only the `fn_XXXXXXXX` stem, so the file keeps the stem.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x80262940 0x802693C4`; 8 of 59 entries are named by the
 * runtime dump, the rest are the map's placeholders).
 *
 * Seam: the range edge is a `--max-bytes` cap, not a TU boundary (nothing in the region names a file).
 * The extabindex run for this file is 0x8002F514-0x8002F6E8 (39 records, first `fn_80262940`, last
 * `fn_8026910C`); the next record is `fn_802695A4` at 0x8002F6E8, i.e. the four functions
 * 0x802693C4-0x802695A4 may belong to this file - they are left to their own proposal
 * (`proposal/802693C4_fn_802693C4`) and recorded here as the open seam question.
 *
 * Flags: the unit uses `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`,
 * `mw_version Wii/1.0`), the Pl lib's measured flag set.
 *
 * Residual: work in progress - functions below the 80 % bar are recorded in
 * `.pi/notes/80262940-fn-80262940-8973.md`.
 */

#include "types.h"
#include "pl.h"
#include "nw4r/math.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_act.h"
#include "Pl/pl_master.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "lobby/fn_8021E1EC.h"
#include "ef/fn_800CDB2C.h"
#include "sound/fn_800D7F54.h"

#ifdef __cplusplus
extern "C" {
#endif

/* this unit's own entry points (unmangled `fn_*` stems, so C linkage - playbook 42/48) */
void fn_80264274(struct _PLW* self, s16 value);
void fn_80264940(struct _PLW* self, u32 value);
u8 fn_8026495C(struct _PLW* self);
void fn_80264A28(struct _PLW* self);
s32 fn_80264A84(struct _PLW* self);
s32 fn_80264EA4(struct _PLW* self);
s32 fn_80264EC0(struct _PLW* self);
s32 fn_80264ED4(struct _PLW* self);
s32 fn_80264EE8(struct _PLW* self);
s32 fn_80264F78(struct _PLW* self);
s32 fn_80264FF0(struct _PLW* self);
s32 fn_8026505C(struct _PLW* self);
s32 fn_802650EC(struct _PLW* self, u8 kind);
s32 fn_80265348(struct _PLW* self);
s32 fn_80265374(struct _PLW* self);
void fn_802656EC(struct _PLW* self);
s32 fn_802656FC(struct _PLW* self);
u32 fn_80265748(struct _PLW* self);
void fn_80265780(struct _PLW* self);
u32 fn_802657AC(struct _PLW* self);
s32 fn_802673B8(void);
void fn_802673D4(u8 index);
u32 fn_80267A78(u8 index);
u8 fn_80267C84(struct _PLW* self);
u32 fn_80268308(struct _PLW* self);
s32 fn_80269394(struct _PLOBJ* obj);
void fn_80267270(struct _PLW* self, u32 action, s32 a, u16 b);
s32 fn_80264B4C(struct _PLW* self);
void fn_80266EB8(struct _PLW* self);
void fn_80268E38(struct _PLW* self, u8* slots, u8 index);
void fn_80268E48(struct _PLW* self, u32 unused, s32 mode, u8** stream);

#ifdef __cplusplus
}
#endif

/* The map spells this one `set_max_player__Fl`, a C++ mangling, so the definition sits at C++ scope and
 * the front-end reproduces the map's name (docs/plan.md 6.5 rule 9). */
void set_max_player(s32 value);

/* 0x80264274 - adds `value` to the actor's hit-stop timer unless the skill overrides it. */
void fn_80264274(_PLW* self, s16 value) {
    if (Pl_Skill_ck(self, 1) == 0) {
        self->field_0x3EA += value;
        return;
    }
    self->field_0x3EA = 0;
}

/* 0x80264940 - accumulates a u16 into the two damage counters it feeds. */
void fn_80264940(_PLW* self, u32 value) {
    u16 sum = self->field_0x398 + (u16)value;

    self->field_0x058 = sum;
    self->field_0x0A8 = sum;
}

/* 0x80264EA4 - the actor's stamina/guard timer has not run out yet. */
s32 fn_80264EA4(_PLW* self) {
    return self->field_0x3FC >= 50;
}

/* 0x80264EC0 - the actor's poison timer is running. */
s32 fn_80264EC0(_PLW* self) {
    return self->field_0x416 > 0;
}

/* 0x80264ED4 - the actor's bleed timer is running. */
s32 fn_80264ED4(_PLW* self) {
    return self->field_0x41C > 0;
}

/* 0x80264EE8 - the "konchu ball" rolling state predicate. */
s32 fn_80264EE8(_PLW* self) {
    u16 motion;

    if (Pl_Skill_ck(self, 0xB2) != 1 && Pl_Skill_ck(self, 0xB3) != 1 && Pl_cat_skill_ck(self, 0x23) != 1) {
        return 0;
    }
    if (self->field_0x00A != 0) {
        return 0;
    }
    motion = self->act_no;
    if ((u32)(motion - 0x33) <= 1 || motion == 0x61) {
        return 1;
    }
    return 0;
}

/* 0x80264F78 - the "rolled up into a ball" predicate. */
s32 fn_80264F78(_PLW* self) {
    if ((Pl_cat_skill_ck(self, 0x21) == 1 || Pl_cat_skill_ck(self, 0x22) == 1) && fn_8027D7EC(self, 0) == 1 &&
        self->field_0x370 > 0) {
        return 1;
    }
    return 0;
}

/* 0x80264FF0 - the sleeping/paralysed state predicate. */
s32 fn_80264FF0(_PLW* self) {
    if ((Pl_cat_skill_ck(self, 0x17) == 1 || Pl_cat_skill_ck(self, 0x18) == 1) && fn_8027BCE0(self) != 0xFFFF) {
        return 1;
    }
    return 0;
}

/* 0x8026505C - the actor is held/knocked down by any of the four status sources. */
s32 fn_8026505C(_PLW* self) {
    if (self->field_0x45A > 0) {
        return 1;
    }
    if (self->field_0x45C > 0) {
        return 1;
    }
    if (fn_80264EE8(self) == 1) {
        return 1;
    }
    if (fn_80264F78(self) == 1) {
        return 1;
    }
    return fn_80264FF0(self) == 1;
}

/* 0x802650EC - maps a motion id onto "the actor is locked in this motion". */
s32 fn_802650EC(_PLW* self, u8 motion) {
    switch (motion) {
    case 6:
    case 7:
    case 8:
        if (fn_8026505C(self) == 0) {
            return 0;
        }
        if (fn_80264EE8(self) == 1 && self->field_0x45A <= 0 && self->field_0x45C <= 0) {
            return 0;
        }
        return 1;
    case 1:
    case 17:
    case 18:
    case 24:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
        return fn_8026505C(self) != 0;
    default:
        return 0;
    }
}

/* 0x80265348 - the actor is in the "konchu ball bounce" motion pair. */
s32 fn_80265348(_PLW* self) {
    if (self->field_0x00A == 6 && (u32)(self->act_no - 0x1F) <= 3) {
        return 1;
    }
    return 0;
}

/* 0x802656EC - clears the two shell timers. */
void fn_802656EC(_PLW* self) {
    self->field_0x418 = 0;
    self->field_0x41E = 0;
}

/* 0x802656FC - the actor is in one of the "charge up a shot" motions. */
s32 fn_802656FC(_PLW* self) {
    u16 motion;

    if (self->field_0x00A != 5) {
        return 0;
    }
    motion = self->act_no;
    if ((u32)(motion - 3) <= 2 || (u32)(motion - 9) <= 2 || (u32)(motion - 16) <= 1) {
        return 1;
    }
    return 0;
}

/* 0x80265748 - the shell/element gauge is at or past its first charge step. */
u32 fn_80265748(_PLW* self) {
    u16 gauge = self->field_0x39C;

    if ((u16)(gauge + 0x1555) > 0x2AAB) {
        return (((u32)(gauge - (u16)0x8001) >> 31) + 1);
    }
    return 0;
}

/* 0x80265780 - fires the "draw the weapon" action request. */
void fn_80265780(_PLW* self) {
    if (self->kind_0x09 == 3) {
        fn_80275AC4(self, 6, 0x22, 0);
        return;
    }
    fn_80275AC4(self, 6, 0x20, 0);
}

/* 0x802673A4 - stores the maximum player count. */
void set_max_player(s32 value) {
    if (lbl_80794B28 == NULL) {
        return;
    }
    lbl_80794B28->count = value;
}

/* 0x802673B8 - reads the maximum player count back. */
s32 fn_802673B8(void) {
    if (lbl_80794B28 == NULL) {
        return 0;
    }
    return lbl_80794B28->count;
}

/* 0x802673D4 - marks one player slot as ready. */
void fn_802673D4(u8 index) {
    lbl_80794B2C[index] = 1;
}

/* 0x8026495C - the actor's ball-spin charge level after the skill adjustments. */
u8 fn_8026495C(_PLW* self) {
    u8 level = self->field_0x3A1;

    if (Pl_Skill_ck(self, 0x22) == 1) {
        if (level > 0x15) {
            level -= 0x14;
        } else {
            level = 1;
        }
    } else if (Pl_Skill_ck(self, 0x21) == 1 || fn_8026FE98((_ENEMY_WORK*)self, 0x2000) != 0) {
        if (level > 0xB) {
            level -= 0xA;
        } else {
            level = 1;
        }
    } else if (Pl_Skill_ck(self, 0x23) == 1) {
        level = (level < 0xF5) ? level + 0xA : 0xFF;
    }
    return level;
}

/* 0x80264A28 - converts the ball-spin charge level into the motion's knock-back distance. */
void fn_80264A28(_PLW* self) {
    u8 level = fn_8026495C(self);
    s16 distance = -0x96;

    if (level >= 0x28) {
        distance = -0x168;
    } else if (level >= 0xF) {
        distance = -0xC8;
    }
    fn_80276B58(self, distance);
}

/* 0x80264A84 - the motion's charge tier for the actor's current state. */
s32 fn_80264A84(_PLW* self) {
    u8 level = fn_8026495C(self);
    s32 tier = 0;

    switch ((u32)(self->field_0x002 - 4) <= 2 ? 1 : self->field_0x002) {
    case 0:
        if (level >= 0x28) {
            tier = 2;
        } else if (level >= 0xF) {
            tier = 1;
        }
        break;
    case 1:
        if (level >= 0x15) {
            tier = 2;
        } else if (level >= 0xF) {
            tier = 1;
        }
        break;
    case 3:
        if (level >= 0x32) {
            tier = 2;
        } else if (level >= 0x28) {
            tier = 1;
        }
        break;
    }
    return tier;
}

/* 0x802657AC - the actor is aiming a shot. */
u32 fn_802657AC(_PLW* self) {
    if (self->kind_0x09 != 3) {
        return 0;
    }
    return Pl_act_ck(self, 1, 0x15) != 0;
}

/* 0x802681D4 - arms the player data load. */
void player_init_data_load(void) {
    if (lbl_80794B28 == NULL) {
        return;
    }
    lbl_80794B28->state = 1;
}

/* 0x802681EC - every live player's 11 state slots have left the "not yet started" value. */
u32 player_move_start_ck(void) {
    _PLGLOBAL* global = lbl_80794B28;
    _PLWORK* work = global->objects;
    _PLOBJ* obj = global->table;
    s32 pending = 0;
    s32 i;

    for (i = 0; i < global->count; i++) {
        if (work[i].active != 0) {
            s32 slot;

            for (slot = 0; slot < 11; slot++) {
                if (obj[i].state[slot] != 1) {
                    pending++;
                }
            }
        }
    }
    return pending == 0;
}

/* 0x80267A78 - one player's per-player work word. */
u32 fn_80267A78(u8 index) {
    if (lbl_80794B28 == NULL) {
        return 0;
    }
    return lbl_80794B28->players_0x44C[index];
}

/* 0x80268E38 - marks one record's slot as "loaded". */
void fn_80268E38(_PLW* self, u8* slots, u8 index) {
    slots[index] = 3;
}

/* 0x80268E48 - advances one streamed record's state from "queued" to "loading". */
void fn_80268E48(_PLW* self, u32 unused, s32 mode, u8** stream) {
    u8* base;
    u32 offset;

    if (mode != 4) {
        return;
    }
    base = stream[0];
    offset = (u32)stream[2];
    if (base[offset] == 4) {
        base[offset] = 5;
    }
}


/* 0x80267C84 - the player-type index the current game mode uses. */
u8 fn_80267C84(_PLW* self) {
    u8 type = 0;
    u8 mode = fn_800CF208();

    if (mode == 2) {
        if (PlayMode_ck() == 5) {
            type = 1;
        } else {
            type = 2;
        }
    } else if (mode == 0 || mode == 3) {
        type = self->field_0x47B;
    }
    return type;
}

/* 0x80268308 - the player record is loaded and every one of its 11 slots has left "empty". */
u32 fn_80268308(_PLW* self) {
    s32 global;
    s32 any;
    s32 i;
    u8 index;

    if (lbl_80794B28 == NULL) {
        return 0;
    }
    index = self->chunk_ofs;
    if (lbl_80794B28->slot_state[index] != 2) {
        return 0;
    }
    any = 0;
    for (i = 0; i < 11; i++) {
        if (lbl_80794B28->table[index].state[i] != 1) {
            any = 1;
        }
    }
    return any != 0;
}

/* 0x80268260 - starts the move for one player (`-1` = every live player). */
void player_move_start(s32 index) {
    _PLGLOBAL* global = lbl_80794B28;
    s32 i;

    if (global == NULL) {
        return;
    }
    if (index == -1) {
        for (i = 0; i < global->count; i++) {
            if (global->objects[i].active != 0) {
                fn_80223E54((s32)global->objects[i].model_0x13C);
            }
        }
    } else if (global->objects[index].active != 0) {
        fn_80223E54((s32)global->objects[index].model_0x13C);
    }
    global->state = 4;
}

/* 0x80269394 - no slot of this record has advanced past "empty". */
s32 fn_80269394(_PLOBJ* obj) {
    s32 count = 0;
    s32 i;

    for (i = 0; i < 11; i++) {
        if (obj->state[i] > 1) {
            count++;
        }
    }
    return count == 0;
}

/* 0x80267270 - forwards an action request to the actor's G3D work. */
void fn_80267270(_PLW* self, u32 action, s32 a, u16 b) {
    if (Pl_master_ck(self) != 0 || action == 3) {
        switch (action) {
        case 2:
            fn_8035B700(2, a, b);
            return;
        case 1:
            fn_8035B700(1, a, b);
            return;
        case 3:
            fn_8035B700(3, a, self->chunk_ofs);
            break;
        }
    }
}


/* 0x80264B4C - the motion belongs to the "uncontrollable" set for the actor's current state. */
s32 fn_80264B4C(_PLW* self) {
    s32 motion = self->act_no;

    switch (self->field_0x00A) {
    case 0:
        if (motion < 0x64) {
            if (motion < 0x51) {
                if (motion < 0x1B) {
                    if (motion < 0x17) {
                        return 0;
                    }
                    return 1;
                }
                return 0;
            }
            if (motion < 0x53) {
                return 1;
            }
            return 0;
        }
        if (motion < 0x8C) {
            if (motion < 0x66) {
                return 1;
            }
            return 0;
        }
        if (motion < 0x96) {
            return 1;
        }
        return 0;
    case 1:
        if (motion == 0x14) {
            return 1;
        }
        return 0;
    case 6:
        if (motion != 0x1A) {
            if (motion < 0x1A) {
                if (motion < 0xC) {
                    if (motion < 0xA) {
                        if (motion < 2) {
                            return 0;
                        }
                        return 1;
                    }
                    return 0;
                }
                if (motion < 0xE) {
                    return 1;
                }
                return 0;
            }
            if (motion < 0x37) {
                if (motion != 0x26) {
                    return 0;
                }
                return 1;
            }
            if (motion < 0x3B) {
                return 1;
            }
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 0x80266EB8 - tells the SE layer the actor's action has finished (or was superseded). */
void fn_80266EB8(_PLW* self) {
    s32 settled = 0;
    u16 motion;

    if (Pl_master_ck(self) != 0) {
        if (self->field_0x00A == 0) {
            motion = self->act_no;
            if ((u32)(motion - 0x6F) <= 9 || (u32)(motion - 0x22) <= 3) {
                settled = 1;
            }
        }
        if (settled == 0) {
            fn_800DB2DC();
        }
    }
}

