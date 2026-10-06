#ifndef MHTRI_PL_PLW_H
#define MHTRI_PL_PLW_H

/* The player work record `_PLW` and the two equipment records it carries by value - the owner's header for
 * every unit that reads the player actor (Pl/ef/enemy/sound).  Deliberately light: it needs only the
 * scalar types, the math vectors and `_CP_VECTOR`, so a unit with its own effect/GX declarations can
 * include it; `pl.h` adds the rest of the player module on top. */

#include "types.h"
#include "nw4r/math.h"
#include "ef/cp_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _SHELL_W;
struct _se_w;

/* One 12-byte equipment record, as the skill code reads it. size: 0xC */
typedef struct _EQUIP {
    u8 kind;         /* +0x0 */  /* equipment kind; picks which skill fields apply (1-5, 6, 7-15) */
    u8 deco_count;   /* +0x1 */  /* number of decoration skill ids in skill_id */
    u16 item_id;     /* +0x2 */  /* 0 when the slot is empty */
    u16 deco_level;  /* +0x4 */  /* two decoration skill levels, low byte first */
    u16 skill_id[3]; /* +0x6 */  /* the decoration skill ids */
} _EQUIP;

/* One 4-byte equipment-slot entry: the item id and its value. size: 0x4 */
typedef struct _SLOTENT {
    u16 item_id; /* +0x0 */
    s16 value;   /* +0x2 */
} _SLOTENT;

/* The player work record, one definition for every consumer (rule 1).  The Pl and ef code reads up to +0x668;
 * `sound/fn_800D7F54.cpp` reads a `_se_w*` at +0xAFC and the record's stride is 0xB20, so the bytes between are
 * padding.  The +0x13C pointer is `_PLW_PHYSICS*` (some consumers spell the same 4 bytes `u8*`).  Where two
 * consumers name one offset differently, the union keeps both names, one member per offset (rule 5).
 * size: 0xB20 */
typedef struct _PLW _PLW;
typedef struct _PLW_PHYSICS _PLW_PHYSICS; /* defined in ef/fn_80114E34.cpp; only pointed at here */
struct _PLW {
    /* +0x000 */ u8 slot_active;   /* the slot is in use (`fn_800EFAC0`/`fn_800EFDD8`) */
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 field_0x002;
    /* +0x003 */ union { /* the original and later names of the same 2 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x003 */ u8 unk003[0x005 - 0x003];
        };
        struct {   /* the later names, padded to the same byte total */
        union {   /* one byte, two spellings (rule 5) */
            /* +0x003 */ u8 pad_merge_0x003[0x1];
            /* +0x003 */ u8 field_0x003;   /* the actor kind `sound/fn_800D7F54.cpp` tests against 9 and 229..230 */
        };
                                /* +0x004 */ u8 field_0x004;
        };
    };
    /* +0x005 */ u8 act_step_0x05;   /* the per-act state step the lobby act handlers advance */
    /* +0x006 */ u8 field_0x006;      /* the act's follow-up stage: reset when the step advances and
                                      * bumped once the step's frame check passes (`Pl/pl_act_step.cpp`);
                                      * the nested handlers advance it as their second-level counter
                                      * (`Pl/pl_act_step.cpp`) */
    /* +0x007 */ u8 field_0x007;      /* the act's skill tier, 1-3, picked from the cat-skill level
                                      * (`Pl/pl_act_step.cpp`) */
    /* +0x008 */ u8 chunk_ofs;     /* plus 0x14 is the chunk index its files go to */
    /* +0x009 */ u8 kind_0x09;   /* compared against 3; `ef/eft019.cpp` names this same `_PLW` byte
                                  * `kind_0x09` and `enemy.h` names the analogous byte `state_0x009` */
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 unk00B;
    /* +0x00C */ u16 act_no;      /* the action number `Pl_act_ck` compares as its `u16` argument
                                  * (`Pl_bari_ck` matches the rage actions 169..174 against it) */
    /* +0x00E */ union {   /* 0x00E-0x013: the state run kept whole, with the act-entry
                            * save/restore the act-entry code needs inside it (same byte total) */
        /* +0x00E */ u8 act_state_0x00E[0x14 - 0x0E];
        struct {
            /* +0x00E */ u8 field_0x00E;     /* set to 1 by the act-state entry (`pl_act_enter`) */
            /* +0x00F */ u8 prev_act_kind;   /* the +0x00A act kind as it was before the entry,
                                              * saved by `pl_act_enter_raw` */
            /* +0x010 */ u16 prev_act_no;    /* the +0x00C act number the entry replaced */
            /* +0x012 */ u16 field_0x012;    /* set to 0xF when the new act is the master's */
        };
    };
    /* +0x014 */ u8 se_name_set;   /* picks the SE/BGM name table (`fn_800EFAC0`) */
    /* +0x015 */ u8 kind_0x015;   /* the NPC/actor kind `pl_act.cpp` reads as its `kind` */
    /* +0x016 */ u8 area_0x16;
    /* +0x017 */ union { /* the original and later names of the same byte (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x017 */ u8 unk017[0x18 - 0x17];
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x017 */ u8 field_0x017;
        };
    };
    /* +0x018 */ union { /* the actor mode byte; three spellings of one byte, one member per offset */
        /* +0x018 */ u8 field_0x18;   /* `fn_80224AC4` reads the actor mode here, and
                                       * `Pl/pl_act.cpp`'s own view of the record calls it `unk18` */
        /* +0x018 */ u8 field_0x018;  /* the two-way variant the act handlers branch on - cleared on
                                       * entry, and `1` selects the alternate motion/effect set
                                       * (`Pl/pl_act_step.cpp`) */
        /* +0x018 */ u8 unk18;        /* an older spelling, kept as a union member for its
                                       * consumers (rule 5) */
    };
    /* +0x019 */ union {
        /* +0x019 */ s8 field_0x19;    /* `fn_80224AC4` clears the actor's live flag when > 0 */
        /* +0x019 */ u8 pad_0x19[0x1]; /* a padding spelling of the same byte; no consumer
                                       * reads it */
    };
    /* +0x01A */ union { /* the original and later names of the same 2 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x01A */ u16 field_0x01A;
        };
        struct {   /* the later names, padded to the same byte total */
        /* +0x01A */ u8 pad_merge_0x01A[0x1];
                                /* +0x01B */ u8 field_0x01B;   /* the "no surface contact" byte `Pl/pl_act_step.cpp` clears */
        };
    };
    /* +0x01C */ u8 user_profile_0x1C;   /* the local player's profile byte `arena_eqdata_from_vsuser`
                                          * copies into the arena user-data record's byte 8 (the same
                                          * slot the game's user-data block fills at its +158);
                                          * GUESS - derived from that body's store, see the unit header */
    /* +0x01D */ u8 se_name_idx;   /* indexes the `fn_800EFAC0` name table */
    /* +0x01E */ u8 field_0x01E;   /* non-zero suppresses the up-swing gate in `fn_80224AC4` */
    /* +0x01F */ union {   /* one byte, two spellings (rule 5) */
        /* +0x01F */ u8 field_0x01F;   /* the act's follow-up stage latch: `pl_act_stage_latch_set` arms it,
                                       * `fn_80274794` tests it and `pl_act_stage_get` reads it */
        /* +0x01F */ u8 pad_0x01F[0x1]; /* the padding spelling of the same byte */
    };
    /* +0x020 */ union {   /* one u32, two spellings (rule 5) */
        /* +0x020 */ u32 unk020;
        /* +0x020 */ u32 frame_0x020;  /* the act's frame counter the skill gates divide by */
    };
    /* +0x024 */ u8 pad_0x24[0x4];
    /* +0x028 */ s32 field_0x28;  /* the act handlers' own frame/step timer; non-zero also
                                  * suppresses the `a2 == 1` act tail (`fn_80258FCC`) */
    /* +0x02C */ struct _SHELL_W* equip_0x2C;
    /* +0x030 */ union {   /* 0x030-0x03B: one 12-byte, 2-aligned run with three views - the
                            * `flag_0x30` byte view (a split of the same bytes kept whole
                            * inside it) and the act unit's two 16-bit fields.  Every member is 12
                            * bytes and the union is anchored at the aligned +0x030, so the run's
                            * extent is unchanged (the byte total is the invariant). */
        struct {   /* the byte view, carrying a split of the same bytes */
            /* +0x030 */ s8 flag_0x30;
            /* +0x031 */ union {
                /* +0x031 */ u8 pad_0x31[0xB];
                struct {
                    /* +0x031 */ u8 pad_0x31_start[0x3];
                    /* +0x034 */ u8 field_0x034;
                    /* +0x035 */ u8 pad_0x35[0x1];
                    /* +0x036 */ u8 pad_0x36;  /* the low byte of the `s16 field_0x036` below; only
                                                * one 16-bit member may carry that name */
                    /* +0x037 */ u8 pad_0x37[0x5];
                };
            };
        };
        struct {   /* the act unit's own view: the act word the act entry clears (`pl_act_enter_raw`) */
            /* +0x030 */ s16 field_0x030;
            /* +0x032 */ u8 pad_0x32[0xA];
        };
        struct {   /* the act unit's own view: the stagger value `pl_act_name_row_get` branches on.  Retail
                    * loads that pair with `lha`, so the field really is a signed 16-bit one */
            /* +0x030 */ u8 pad_0x30_pair[0x6];
            /* +0x036 */ s16 field_0x036;
            /* +0x038 */ u8 pad_0x38[0x4];
        };
    };
    /* +0x03C */ union { /* the original and later names of the same 12 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x03C */ union {   /* one f32, three spellings: `unk03C`/`field_0x03C` and
                                    * `motion_pos_0x3C` - the three floats the 0x8026A4E4-family integrators
                                    * move are the position at +0x3C, the velocity at +0x78 and the
                                    * acceleration at +0x84 (fn_8026A4E4 adds the velocity into the position,
                                    * fn_8026A518 the acceleration into the velocity) */
                /* +0x03C */ f32 unk03C;
                /* +0x03C */ f32 field_0x03C;
                /* +0x03C */ f32 motion_pos_0x3C;
            };
            /* +0x040 */ union {   /* one f32, three spellings */
                /* +0x040 */ f32 unk40;
                /* +0x040 */ f32 field_0x040;
                /* +0x040 */ f32 motion_pos_0x40;
            };
            /* +0x044 */ union {   /* one f32, three spellings */
                /* +0x044 */ f32 unk44;
                /* +0x044 */ f32 field_0x044;
                /* +0x044 */ f32 motion_pos_0x44;
            };
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x03C */ VEC3 vec_0x03C;   /* the actor's aim/facing vector the motion layer reads */
        };
    };
    /* +0x048 */ union { /* the original and later names of the same 12 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x048 */ u8 unk048[0x54 - 0x48];
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x048 */ VEC3 vec_0x048;
        };
    };
    /* +0x054 */ union {   /* the 0x0C-byte rotation run: one _CP_VECTOR, or its three words */
        /* +0x054 */ _CP_VECTOR rot_0x54;
        struct {
            /* +0x054 */ u32 param_0x54;
            /* +0x058 */ u32 field_0x058;
            /* +0x05C */ u32 rot_z_0x5C;
        };
    };
    /* +0x060 */ f32 ground_y_0x060;  /* the player's base/target y the effect sits on */
    /* +0x064 */ union {   /* one f32, two spellings of the same 4 bytes (rule 5, one member per
                            * offset): `unk064` (`Pl/pl_act.cpp` reads it as the
                            * height its +0x040 motion position is checked against) and
                            * `field_0x064` (`Pl/pl_act_step.cpp` reads and writes it) */
        /* +0x064 */ f32 unk064;
        /* +0x064 */ f32 field_0x064;
    };
    /* +0x068 */ union {   /* one word, two spellings (rule 5) */
        /* +0x068 */ u8 pad_0x68[0x4];  /* the padding spelling */
        /* +0x068 */ f32 field_0x068;   /* the first of the three floats the act entry resets */
    };
    /* +0x06C */ union {   /* one f32, two spellings (rule 5) */
        /* +0x06C */ f32 unk6C;
        /* +0x06C */ f32 field_0x06C;  /* the act's cached vector y (`Pl/pl_act.cpp` reads it) */
    };
    /* +0x070 */ union {
        /* +0x070 */ f32 unk70;
        /* +0x070 */ f32 field_0x070;  /* the act's cached vector z */
    };
    /* +0x074 */ union { /* the original and later names of the same 28 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x074 */ union {   /* two views of the 0x074-0x08F run, both 0x1C bytes (the byte total is
                                    * the invariant; a union starting at +0x075 would round 0x1B up to 0x1C
                                    * and grow `_PLW` by 4, so the union is anchored at the aligned +0x074) */
                struct {   /* the byte view: the +0x074 byte and the `unk075` filler run */
                    /* +0x074 */ u8 unk074;
                    /* +0x075 */ u8 unk075[0x90 - 0x75];
                };
                struct {   /* a split of the same bytes (the part/motion section) */
                    /* +0x074 */ u8 pad_0x74[0x1];
                    /* +0x075 */ u8 pad_0x75[0x3];
                    /* +0x078 */ f32 motion_vel_0x78;
                    /* +0x07C */ f32 motion_vel_0x7C;
                    /* +0x080 */ f32 motion_vel_0x80;
                    /* +0x084 */ f32 motion_acc_0x84;
                    /* +0x088 */ f32 motion_acc_0x88;
                    /* +0x08C */ f32 motion_acc_0x8C;
                };
            };
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x074 */ u8 field_0x074;
                                /* +0x075 */ u8 field_0x075;
                                /* +0x076 */ u8 field_0x076;
        /* +0x077 */ u8 pad_merge_0x077[0x19];
        };
    };
    /* +0x090 */ VEC3 target_pos_0x090;   /* the position the blend walks the actor to (`pl_pos_blend_start` divides the gap to it by its frame count); the net receivers write it */
    /* +0x09C */ f32 unk09C;
    /* +0x0A0 */ f32 unk0A0;
    /* +0x0A4 */ f32 unk0A4;
    /* +0x0A8 */ u32 field_0x0A8;          /* the second counter `fn_80264940` feeds (Pl/pl_act_step.cpp) */
    /* +0x0AC */ union { /* the original and later names of the same 8 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x0AC */ union {   /* the run is `[0xB4 - 0xAC]`, with its first word named */
                /* +0x0AC */ u8 unk0AC[0xB4 - 0xAC];
                struct {
                    /* +0x0AC */ u32 field_0x0AC;  /* mirrors the damage/param word at +0x054 (`fn_802498E0`) */
                    /* +0x0B0 */ u8 pad_0x0B0[0x4];
                };
            };
        };
        struct {   /* the later names, padded to the same byte total */
        /* +0x0AC */ u8 pad_merge_0x0AC[0x6];
                                /* +0x0B2 */ u16 field_0x0B2;
        };
    };
    /* +0x0B4 */ s16 unk0B4;
    /* +0x0B6 */ u16 field_0x0B6;  /* a bit field: bit 0 and bit 15 are tested and the low decimal
                                   * pair feeds the +0x354 scale (`Pl/pl_act_step.cpp`); the low 3
                                   * bits are the cat-skill level `Pl/pl_act_step.cpp` reads */
    /* +0x0B8 */ union {   /* one u16, two spellings: a padding and the sound view's name */
        /* +0x0B8 */ u8 pad_0xB8[0x2];
        /* +0x0B8 */ u16 field_0x0B8;
    };
    /* +0x0BA */ u16 unkBA;
    /* +0x0BC */ u16 unkBC;
    /* +0x0BE */ u16 unkBE;
    /* +0x0C0 */ u8 pad_0xC0[0xC];
    /* +0x0CC */ u16 unkCC;
    /* +0x0CE */ u16 unkCE;
    /* +0x0D0 */ union {   /* 0x0D0-0x10F: the id bit sets as u32 words, and the master band's two per-part u16 tables over the same bytes */
        struct {
            /* +0x0D0 */ u8 pad_0xD0[0xC];
            /* +0x0DC */ u32 id_flags_0xDC;  /* the id bit set/tested/cleared by fn_8026A6D8 / fn_8026A6F4 /
                                              * fn_8026A718 (bit `id & 31`) */
            /* +0x0E0 */ u32 id_flags_0xE0[4];  /* 128 ids, set/tested by fn_8026A618 / pl_part_flag_ck */
            /* +0x0F0 */ u32 id_flags_0xF0[4];  /* 128 ids, set/tested by fn_8026A678 / fn_8026A6A4 */
            /* +0x100 */ u8 pad_0x100[0x10];
        };
        struct {
            /* +0x0D0 */ u16 part_tbl_a_0xD0[2];   /* per-part table A (the `lhz 208(this + idx*2)` reads) */
            /* +0x0D4 */ u16 part_tbl_b_0xD4[0x1E];   /* per-part table B (the `lhz/lha 212(this + idx*2)` reads) */
        };
    };
    /* +0x110 */ f32 unk110;
    /* +0x114 */ u8 pad_0x114[0x14];
    /* +0x128 */ union {   /* one u8, two spellings (rule 5) */
        /* +0x128 */ u8 unk128;
        /* +0x128 */ u8 field_0x128;
    };
    /* +0x129 */ u8 field_0x129;   /* the "initialised" latch of the master band */
    /* +0x12A */ u8 pad_0x12A[0xA];
    /* +0x134 */ union {   /* one u8, two spellings: `unk134` and a name */
        /* +0x134 */ u8 unk134;
        /* +0x134 */ u8 field_0x134;   /* bit 1 is the flag `fn_8026BA04` returns */
    };
    /* +0x135 */ u8 pad_0x135[0x7];
    /* +0x13C */ _PLW_PHYSICS* physics_0x13C;
    /* +0x140 */ _EQUIP equipA[6];
    /* +0x188 */ u8 pad_0x188[0x1C];
    /* +0x1A4 */ u8 effect_key_0x1A4;
    /* +0x1A5 */ u8 pad_0x1A5[0x23];
    /* +0x1C8 */ u32 field_0x1C8;   /* bit 0 picks which of the two slot values `sound/fn_800D7F54.cpp` copies */
    /* +0x1CC */ u8 pad_0x1CC[0x4];
    /* +0x1D0 */ _EQUIP equipB;
    /* +0x1DC */ _EQUIP equipB2;
    /* +0x1E8 */ _EQUIP equipC;
    /* +0x1F4 */ _EQUIP equipD;
    /* +0x200 */ _EQUIP equipE[2];
    /* +0x218 */ s32 set_applied[7];
    /* +0x234 */ s32 set_pending[7];
    /* +0x250 */ u16 equip_valid;
    /* +0x252 */ u16 set_valid;
    /* +0x254 */ u16 deco_dirty;
    /* +0x256 */ union {   /* two bytes, two spellings */
        /* +0x256 */ u8 pad_0x256[0x2];
        /* +0x256 */ s16 field_0x256;  /* the 450-frame timer the "2" act entry presets */
    };
    /* +0x258 */ u8 field_0x258[0x4];  /* passed to `fn_80223830` by `fn_80224AC4` */
    /* +0x25C */ u8 field_0x25C[0x8];  /* passed to `fn_80223708` by `fn_80224AC4` */
    /* +0x264 */ union {
        /* +0x264 */ s16 unk264;
        /* +0x264 */ s16 field_0x264;  /* cleared by the act entry (`pl_act_enter_raw`) */
    };
    /* +0x266 */ union {   /* the 3-byte run, with a named split inside it */
        /* +0x266 */ u8 unk266[0x269 - 0x266];
        struct {
            /* +0x266 */ s8 field_0x266;  /* non-zero holds off the act switch (`fn_80258FCC`) */
            /* +0x267 */ u8 unk267[0x268 - 0x267];
            /* +0x268 */ u8 field_0x268;  /* the byte `fn_8027D738` reports as a boolean */
        };
    };
    /* +0x269 */ u8 unk269;
    /* +0x26A */ u8 unk26A;
    /* +0x26B */ u8 unk26B;
    /* +0x26C */ u8 held_item_kind_0x26C;   /* the held item's kind `fn_80279B84` derives from its item id (GUESS) */
    /* +0x26D */ u8 unk26D;
    /* +0x26E */ u16 unk26E;
    /* +0x270 */ s16 unk270;
    /* +0x272 */ s16 unk272;
    /* +0x274 */ u8 unk274;
    /* +0x275 */ u8 unk275;
    /* +0x276 */ union { /* the original and later names of the same byte (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x276 */ u8 unk276;
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x276 */ u8 field_0x276;
        };
    };
    /* +0x277 */ u8 unk277[0x278 - 0x277];
    /* +0x278 */ union {   /* 0x278-0x2E0: the item/equipment slot run, 0x68 bytes, two views.  The
                            * view names 24 entries and a +0x2D8 padding tail; the item
                            * scan `Pl/pl_act_step.cpp`'s `fn_802466C4` runs walks 0x1A four-byte entries
                            * from +0x278 to +0x2E0, which pins the real count as 26 (the two the
                            * 24-entry view left in its padding run are entries 24 and 25).  Both
                            * views are 0x68 bytes and the `slot_id` name survives with the wider
                            * count, so every consumer keeps compiling and nothing moves */
        struct {   /* the 24-entry view, its padding run kept by name */
            /* +0x278 */ u8 pad_0x278[0x60];
            /* +0x2D8 */ u8 unk2D8[0x2E0 - 0x2D8];
        };
        struct {   /* the item-scan view */
            /* +0x278 */ _SLOTENT slot_id[26];
        };
    };
    /* +0x2E0 */ _SLOTENT spare_slot_id[8];
    /* +0x300 */ u8 unk300[0x304 - 0x300];
    /* +0x304 */ u16 field_0x304;
    /* +0x306 */ union { /* the effect/motion id `pl_item_add` is fed and indexed on; one member per
                          * offset, both spellings kept (rule 5); `Pl/pl_act_step.cpp` and
                          * `Pl/pl_act_step.cpp` read it */
        /* +0x306 */ u16 field_0x306;
        /* +0x306 */ u16 unk306;        /* an older spelling, kept for its consumers */
    };
    /* +0x308 */ union { /* 0x308-0x30B: the 4-byte spelling kept whole as a union member,
                          * with the byte split the fn_80258FCC unit needs inside it (same byte total) */
        /* +0x308 */ u8 unk308[0x30C - 0x308];
        struct {
            /* +0x308 */ u8 field_0x308;  /* the act's hold/charge latch (`fn_8025E32C`) */
            /* +0x309 */ u8 field_0x309;  /* the act's status byte (`fn_8025A7FC` sets 0x40) */
            /* +0x30A */ union {   /* the run's last two bytes, with this unit's armed byte */
                /* +0x30A */ u8 unk30A[0x30C - 0x30A];
                struct {
                    /* +0x30A */ u8 field_0x30A;  /* the act's armed byte (`pl_act_reenter` sets
                                                   * 0xFF); `fn_80274570` tests it */
                    /* +0x30B */ u8 pad_0x30B;
                };
            };
        };
    };
    /* +0x30C */ u8 unk30C;
    /* +0x30D */ u8 unk30D;
    /* +0x30E */ union {   /* 0x30E-0x317: one 10-byte run with two views - the byte view
                            * (`flag_0x30E` and its runs, with the `unk30F_start` split inside
                            * them) and the act entry's own 16-bit `field_0x310` alongside it.  The
                            * run starts at an odd address, so the 2-byte field can only live in the
                            * member that starts even */
        struct {   /* the byte view */
            /* +0x30E */ u8 flag_0x30E;   /* the lobby act family's own flag */
            /* +0x30F */ union {
                /* +0x30F */ u8 unk30F[0x313 - 0x30F];
                struct {
                    /* +0x30F */ u8 unk30F_start;   /* the `Pl/pl_act.cpp` split of the run */
                    /* +0x310 */ u8 pad_0x310[0x3];
                };
            };
            /* +0x313 */ union {   /* one s8, two spellings (rule 5) */
                /* +0x313 */ s8 unk313;
                /* +0x313 */ s8 field_0x313;  /* > 0 suppresses the scan (`fn_8025E298`) */
            };
            /* +0x314 */ union {   /* one u8, two spellings (rule 5) */
                /* +0x314 */ u8 unk314;
                /* +0x314 */ u8 field_0x314;  /* > 0 suppresses the scan (`fn_8025E298`) */
            };
            /* +0x315 */ u8 unk315[0x318 - 0x315];
        };
        struct {   /* the act entry's own view; the halfword and the byte each carry
                    * both spellings as union members, one member per offset */
            /* +0x30E */ u8 pad_0x30E_pair[0x2];
            /* +0x310 */ union {   /* one halfword, two spellings (rule 5) */
                /* +0x310 */ u16 field_0x310;         /* the halfword the act entry clears
                                                       * (`fn_80275C34`); `fn_80280678` clears
                                                       * the same halfword on entry */
                /* +0x310 */ s16 charge_gauge_0x310;  /* the accumulating charge gauge
                                                       * `Pl/fn_802840DC.cpp` advances a frame at a
                                                       * time and saturates at 999 (`fn_802843CC`);
                                                       * `fn_80284474` divides it by the charge rate
                                                       * to get the level, so the byte below stays
                                                       * put */
            };
            /* +0x312 */ union {   /* one byte, two spellings (rule 5) */
                /* +0x312 */ u8 field_0x312;          /* the byte `fn_80280678` clears on entry */
                /* +0x312 */ u8 charge_level_0x312;   /* the level the gauge buys: `fn_80284474`
                                                       * writes gauge/rate here and its callers
                                                       * clamp it to 2 */
            };
            /* +0x313 */ u8 pad_0x313_run[0x5];
        };
    };
    /* +0x318 */ union {   /* one u32, two spellings (rule 5) */
        /* +0x318 */ u32 unk318;
        /* +0x318 */ u32 field_0x318;  /* the scan table `fn_8025E298` walks */
    };
    /* +0x31C */ union {   /* one s16, two spellings (rule 5) */
        /* +0x31C */ s16 unk31C;
        /* +0x31C */ s16 field_0x31C;  /* cleared by the act entry */
    };
    /* +0x31E */ union {
        /* +0x31E */ s16 unk31E;
        /* +0x31E */ s16 field_0x31E;
    };
    /* +0x320 */ union {
        /* +0x320 */ s16 unk320;
        /* +0x320 */ s16 field_0x320;
    };
    /* +0x322 */ union {   /* 0x322-0x353: the scratch run, with the 48-byte array the
                            * act entry clears inside it (same byte total) */
        struct {   /* the byte view, with a split of the 0x332 tail kept inside it */
            /* +0x322 */ u8 unk322[16];
            /* +0x332 */ union {
                /* +0x332 */ u8 unk332[0x354 - 0x332];
                struct {
                    /* +0x332 */ u8 unk332_start[0x352 - 0x332];
                    /* +0x352 */ s16 field_0x352;  /* > 0 is `fn_8027E1E4`'s first arm */
                };
            };
        };
        struct {   /* the act-entry 48-byte array in the same bytes */
            /* +0x322 */ u8 field_0x322[0x30];
            /* +0x352 */ u8 pad_0x352[0x2];
        };
    };
    /* +0x354 */ union {   /* one f32, two spellings: `field_0x354` is
                            * the scale `fn_8026A2BC` hands the model layer */
        /* +0x354 */ f32 unk354;
        /* +0x354 */ f32 field_0x354;
    };
    /* +0x358 */ union {   /* one f32, two spellings (rule 5) */
        /* +0x358 */ f32 unk358;
        /* +0x358 */ f32 field_0x358;
    };
    /* +0x35C */ union {   /* one word, two spellings */
        /* +0x35C */ u8 pad_0x35C[0x4];
        /* +0x35C */ u32 field_0x35C;  /* cleared by the act entry */
    };
    /* +0x360 */ union {
        /* +0x360 */ u32 unk360;
        /* +0x360 */ u32 field_0x360;  /* cleared by the act entry */
    };
    /* +0x364 */ union { /* the original and later names of the same 4 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x364 */ u32 unk364;
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x364 */ u32 field_0x364;
        };
    };
    /* +0x368 */ u8 unk368;
    /* +0x369 */ union { /* the original and later names of the same byte (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x369 */ u8 unk369;
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x369 */ u8 field_0x369;
        };
    };
    /* +0x36A */ union {   /* one u8, two spellings (rule 5) */
        /* +0x36A */ u8 unk36A;
        /* +0x36A */ u8 field_0x36A;  /* cleared by the act entry */
    };
    /* +0x36B */ u8 unk36B;
    /* +0x36C */ s16 field_0x36C;
    /* +0x36E */ u8 unk36E[0x370 - 0x36E];
    /* +0x370 */ union { /* the health pair: current and cap, both signed 16-bit (`pl_act_add_hold_gauge`
                         * adds a signed delta to the current, `fn_80276690` moves the cap and clamps
                         * the current to it, and the reset sets current = cap) */
        /* +0x370 */ s16 health;      /* current health; clamped to [0, cap], and to >= 1 by the act
                                       * path that takes `-0xA` damage through `pl_act_add_hold_gauge` */
        /* +0x370 */ s16 field_0x370; /* the spelling `Pl/pl_act_step.cpp` reads; the
                                       * act's current hold gauge (`fn_80276690`) */
    };
    /* +0x372 */ union {
        /* +0x372 */ s16 health_max; /* the cap: `fn_80276690` clamps it to [1, 150] and clamps
                                      * `health`/`unk376` down to it */
        /* +0x372 */ s16 unk372;     /* an older spelling of the same word */
        /* +0x372 */ s16 field_0x372;  /* the hold gauge's ceiling (`fn_80276690` grows it) */
    };
    /* +0x374 */ union {
        /* +0x374 */ u8 pad_0x374[0x2];
        /* +0x374 */ s16 field_0x374;  /* the soft gauge cap `fn_80276690` grows when `a == 0` */
    };
    /* +0x376 */ union {
        /* +0x376 */ s16 unk376;
        /* +0x376 */ s16 field_0x376;  /* the highest gauge value reached this act */
    };
    /* +0x378 */ union {   /* one s16, two spellings */
        /* +0x378 */ s16 unk378;
        /* +0x378 */ s16 field_0x378;  /* the 150-frame gate the act tail tests (`fn_80258FCC`) */
    };
    /* +0x37A */ union {   /* one s16, two spellings */
        /* +0x37A */ s16 unk37A;
        /* +0x37A */ s16 field_0x37A;  /* the `s16` the act handlers hand to `fn_80276868` and gate a
                                       * motion on (`<= 0x96`) */
    };
    /* +0x37C */ union {
        /* +0x37C */ s16 unk37C;
        /* +0x37C */ s16 field_0x37C;
    };
    /* +0x37E */ union {
        /* +0x37E */ u8 pad_0x37E[0x2];
        /* +0x37E */ s16 field_0x37E;  /* the act's stagger value `fn_802767B4` steps */
    };
    /* +0x380 */ union {
        /* +0x380 */ s16 unk380;
        /* +0x380 */ s16 field_0x380;  /* the act's stagger budget, capped at 150 */
    };
    /* +0x382 */ union {
        /* +0x382 */ u8 pad_0x382[0x2];
        /* +0x382 */ s16 field_0x382;  /* the re-arm flag `fn_80276778`/`fn_802767B4` clear */
    };
    /* +0x384 */ s16 field_0x384;
    /* +0x386 */ union {
        /* +0x386 */ s16 unk386;
        /* +0x386 */ s16 field_0x386;  /* the stagger timer `fn_802748C8` buckets into 0-3 */
    };
    /* +0x388 */ union {
        /* +0x388 */ u8 unk388;
        /* +0x388 */ u8 field_0x388;   /* cleared by the act entry */
    };
    /* +0x389 */ u8 unk389[0x38A - 0x389];
    /* +0x38A */ s16 unk38A;
    /* +0x38C */ s16 unk38C;
    /* +0x38E */ s16 unk38E;
    /* +0x390 */ s16 unk390;
    /* +0x392 */ s16 unk392;
    /* +0x394 */ s16 unk394;
    /* +0x396 */ union {   /* one s16, two spellings (rule 5) */
        /* +0x396 */ s16 unk396;
        /* +0x396 */ s16 field_0x396;  /* armed with the 2-frame hold by `pl_act_stage_latch_set`; > 0 is also
                                       * the first gate this unit's `fn_8027D684` peer tests */
    };
    /* +0x398 */ u16 field_0x398;        /* running damage dealt, `fn_80264940` accumulates into it */
    /* +0x39A */ u8 unk39A[0x39C - 0x39A];
    /* +0x39C */ u16 field_0x39C;         /* shell/element charge gauge (`fn_80265748`) */
    /* +0x39E */ u8 unk39E;
    /* +0x39F */ u8 unk39F[0x3A1 - 0x39F];
    /* +0x3A1 */ u8 field_0x3A1;
    /* +0x3A2 */ union {
        /* +0x3A2 */ s8 unk3A2;
        /* +0x3A2 */ s8 field_0x3A2;  /* cleared by the act entry */
    };
    /* +0x3A3 */ union {
        /* +0x3A3 */ s8 unk3A3;
        /* +0x3A3 */ s8 field_0x3A3;
    };
    /* +0x3A4 */ union {   /* the 8-byte run, with a named split inside it */
        /* +0x3A4 */ u8 unk3A4[0x3AC - 0x3A4];
        struct {
            /* +0x3A4 */ u8 field_0x3A4;
            /* +0x3A5 */ u8 field_0x3A5;
            /* +0x3A6 */ u8 field_0x3A6;
            /* +0x3A7 */ u8 field_0x3A7;
            /* +0x3A8 */ s16 field_0x3A8;  /* the act's 300-frame cooldown (`fn_8025A7D4` re-arms it) */
        };
    };
    /* +0x3AC */ union { /* the original and later names of the same 4 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x3AC */ u32 unk3AC;
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x3AC */ u32 field_0x3AC;
        };
    };
    /* +0x3B0 */ union { /* the original and later names of the same 4 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x3B0 */ u8 unk3B0[0x3B4 - 0x3B0];
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x3B0 */ u32 field_0x3B0;
        };
    };
    /* +0x3B4 */ union { /* the original and later names of the same byte (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x3B4 */ u8 unk3B4;
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x3B4 */ u8 field_0x3B4;
        };
    };
    /* +0x3B5 */ union { /* the original and later names of the same byte (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x3B5 */ u8 unk3B5;
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x3B5 */ u8 field_0x3B5;
        };
    };
    /* +0x3B6 */ u8 sub_area_0x3B6;   /* sent in the state message next to the area byte (GUESS) */
    /* +0x3B7 */ u8 pad_0x3B7;
    /* +0x3B8 */ u16 skill_point_0x3B8;   /* the skill point total (capped at 700) whose fraction +0x3BC holds */
    /* +0x3BA */ u16 unk3BA;
    /* +0x3BC */ f32 unk3BC;
    /* +0x3C0 */ f32 unk3C0;
    /* +0x3C4 */ f32 unk3C4;
    /* +0x3C8 */ f32 unk3C8;
    /* +0x3CC */ f32 unk3CC;
    /* +0x3D0 */ f32 unk3D0;
    /* +0x3D4 */ f32 unk3D4;
    /* +0x3D8 */ union {   /* one u32, two spellings */
        /* +0x3D8 */ u32 unk3D8;
        /* +0x3D8 */ u32 field_0x3D8;  /* act bitfield; `fn_80258FCC` clears the 0x300 pair */
    };
    /* +0x3DC */ union {   /* one u32, two spellings (rule 5) */
        /* +0x3DC */ u32 unk3DC;
        /* +0x3DC */ u32 field_0x3DC;
    };
    /* +0x3E0 */ u32 unk3E0;
    /* +0x3E4 */ union {   /* the 6-byte run, with a named split inside it */
        /* +0x3E4 */ u8 unk3E4[0x3EA - 0x3E4];
        struct {
            /* +0x3E4 */ u8 field_0x3E4;
            /* +0x3E5 */ u8 field_0x3E5;
            /* +0x3E6 */ u8 field_0x3E6;
            /* +0x3E7 */ u8 field_0x3E7;
            /* +0x3E8 */ u8 field_0x3E8;
            /* +0x3E9 */ u8 field_0x3E9;
        };
    };
    /* +0x3EA */ s16 field_0x3EA;         /* hit-stop / stagger timer `fn_80264274` feeds */
    /* +0x3EC */ s16 unk3EC;
    /* +0x3EE */ union {   /* the 4-byte run, with a named split inside it */
        /* +0x3EE */ u8 unk3EE[0x3F2 - 0x3EE];
        struct {
            /* +0x3EE */ u8 pad_0x3EE[0x2];
            /* +0x3F0 */ s16 field_0x3F0;  /* the act's first timer (`fn_80259310`) */
        };
    };
    /* +0x3F2 */ s16 unk3F2;
    /* +0x3F4 */ union {   /* the 4-byte run, split two ways (same byte total) */
        /* +0x3F4 */ u8 unk3F4[0x3F8 - 0x3F4];
        struct {
            /* +0x3F4 */ union {
                /* +0x3F4 */ u8 pad_0x3F4[0x2];
                /* +0x3F4 */ s16 field_0x3F4;   /* the other spelling of the same two bytes */
            };
            /* +0x3F6 */ union {
                /* +0x3F6 */ u8 pad_0x3F6[0x2];
                /* +0x3F6 */ s16 field_0x3F6;  /* the act's second timer (`fn_80259310`) */
            };
        };
    };
    /* +0x3F8 */ s16 unk3F8;
    /* +0x3FA */ u8 unk3FA[0x3FC - 0x3FA];
    /* +0x3FC */ s16 field_0x3FC;         /* stamina/guard timer (`fn_80264EA4`, `fn_80265374`) */
    /* +0x3FE */ s16 unk3FE;
    /* +0x400 */ union {   /* one u16 pair, also spelled as the single `unk400` run */
        /* +0x400 */ u8 unk400[0x404 - 0x400];
        struct {
            /* +0x400 */ u16 field_0x400;   /* the guard/stamina timer family `Pl/pl_act_step.cpp` resets */
            /* +0x402 */ u16 field_0x402;
        };
    };
    /* +0x404 */ union { /* the original and later names of the same 2 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x404 */ s16 unk404;
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x404 */ s16 field_0x404;
        };
    };
    /* +0x406 */ union {   /* the 8-byte run, split by the act fields (same byte total) */
        /* +0x406 */ u8 unk406[0x40E - 0x406];
        struct {   /* the four timer words the act-84 entry clears; all four are `u16` at the
                    * access (`sth`) and the entry is the only writer */
            /* +0x406 */ u16 field_0x406;
            /* +0x408 */ u16 field_0x408;
            /* +0x40A */ u16 field_0x40A;
            /* +0x40C */ u16 field_0x40C;
        };
    };
    /* +0x40E */ s16 unk40E;
    /* +0x410 */ union {   /* the 4-byte run 0x410-0x413, with the act's re-arm word inside */
        /* +0x410 */ u8 unk410[0x414 - 0x410];
        /* +0x410 */ u16 field_0x410;  /* the second timer pair's first word (`Pl/pl_act_step.cpp`) */
        struct {
            /* +0x410 */ u8 pad_0x410[0x2];
            /* +0x412 */ s16 field_0x412;  /* the hold-gauge re-arm word `Pl_act_state_ck` tests */
        };
    };
    /* +0x414 */ s16 unk414;
    /* +0x416 */ s16 field_0x416;
    /* +0x418 */ s16 field_0x418;         /* first shell timer, cleared by `fn_802656EC` */
    /* +0x41A */ s16 unk41A;
    /* +0x41C */ s16 field_0x41C;
    /* +0x41E */ s16 field_0x41E;         /* second shell timer, cleared by `fn_802656EC` */
    /* +0x420 */ s16 unk420;
    /* +0x422 */ union {
        /* +0x422 */ s16 unk422;
        /* +0x422 */ u16 field_0x422;  /* one of the timer pair the act-84 entry clears */
    };
    /* +0x424 */ s16 unk424;
    /* +0x426 */ s16 unk426;
    /* +0x428 */ s16 unk428;
    /* +0x42A */ s16 unk42A;
    /* +0x42C */ s16 unk42C;
    /* +0x42E */ union {   /* the 0x16-byte run 0x42E-0x443, with the act-end word inside it */
        /* +0x42E */ u8 pad_0x42E[0x16];
        struct {
            /* +0x42E */ u16 field_0x42E;   /* the five-word timer run the act-84 entry clears; the
                                            * access is a raw `sth` and nothing reads them else-
                                            * where, so the width is taken from the store */
            /* +0x430 */ u16 field_0x430;
            /* +0x432 */ u16 field_0x432;
            /* +0x434 */ u16 field_0x434;
            /* +0x436 */ u16 field_0x436;
            /* +0x438 */ u8 pad_0x438[0xA];
            /* +0x442 */ u16 field_0x442;  /* cleared by the act-state re-entry (`pl_act_reenter`) */
        };
    };
    /* +0x444 */ s8 field_0x444;  /* the shell timer's re-arm countdown (`fn_80258FCC`) */
    /* +0x445 */ union {   /* one byte, two spellings (rule 5) */
        /* +0x445 */ u8 field_0x445;   /* cleared by the act entry unless the act keeps its timer;
                                        * `fn_8027E048` advances and clamps the same 0-100
                                        * counter */
        /* +0x445 */ u8 pad_0x445[0x1]; /* the padding spelling of the same byte */
    };
    /* +0x446 */ union {   /* one u8, two spellings (rule 5) */
        /* +0x446 */ u8 unk446;
        /* +0x446 */ u8 field_0x446;  /* the strike counter `fn_8025B0D4` saturates at 10 */
    };
    /* +0x447 */ union {
        /* +0x447 */ u8 unk447;
        /* +0x447 */ u8 field_0x447;  /* the act's one-shot cat-skill latch (`fn_8027633C`) */
    };
    /* +0x448 */ s8 unk448;
    /* +0x449 */ s8 unk449;
    /* +0x44A */ u8 unk44A[0x44C - 0x44A];
    /* +0x44C */ s8 unk44C;
    /* +0x44D */ s8 unk44D;
    /* +0x44E */ union {   /* the 12-byte run, split (same byte total) */
        /* +0x44E */ u8 unk44E[0x45A - 0x44E];
        struct {
            /* +0x44E */ u8 unk44E_start[0x450 - 0x44E];
            /* +0x450 */ u8 field_0x450;   /* set by `fn_8027D6A4`, with the s16 at +0x454 */
            /* +0x451 */ u8 field_0x451;   /* set by `fn_8027D6C0`, with the s16 at +0x456 */
            /* +0x452 */ s8 field_0x452;   /* set by `fn_8027D698`, with the s16 at +0x458 */
            /* +0x453 */ u8 pad_0x453[0x1];
            /* +0x454 */ s16 field_0x454;  /* raised to the argument, never lowered (`fn_8027D6A4`) */
            /* +0x456 */ s16 field_0x456;  /* raised to the argument, never lowered (`fn_8027D6C0`) */
            /* +0x458 */ s16 field_0x458;  /* stored raw by `fn_8027D698` */
        };
    };
    /* +0x45A */ s16 field_0x45A;
    /* +0x45C */ s16 field_0x45C;         /* the second hold/knock-down counter (`fn_8026505C`) */
    /* +0x45E */ union {   /* the 8-byte run, with a named split inside it */
        /* +0x45E */ u8 pad_0x45E[0x466 - 0x45E];
        struct {
            /* +0x45E */ u8 pad_0x45E_start[0x460 - 0x45E];
            /* +0x460 */ s16 field_0x460;  /* > 0 is the whole of `Pl_timer_0x460_ck` */
            /* +0x462 */ u8 pad_0x462[0x464 - 0x462];
            /* +0x464 */ s16 field_0x464;  /* the cockpit-quest bar's recompute gate:
                                            * `quest_bar_a_next_id` (`hud/cockpit_quest.cpp`) reads it
                                            * as `lha` and leaves the bar's blend parameter alone
                                            * while it is non-zero (playbook 56 splice: 2 B out of
                                            * the filler, no member moved) */
        };
    };
    /* +0x466 */ union { /* two views of the same 2 bytes, the second `hud/cockpit_quest.cpp`'s
                          * (rule 5: one member per offset) */
        struct {   /* the original layout: the name `Pl/pl_act.cpp` reads */
            /* +0x466 */ s16 unk466;
        };
        struct {   /* this band's name: `quest_bar_b_next_id` reads it as `lha` and stops recomputing the
                    * second quest bar while it is non-zero */
            /* +0x466 */ s16 field_0x466;
        };
    };
    /* +0x468 */ s16 unk468;
    /* +0x46A */ s16 unk46A;
    /* +0x46C */ union { /* the original and later names of the same byte (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x46C */ u8 unk46C;
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x46C */ u8 field_0x46C;
        };
    };
    /* +0x46D */ union { /* the original and later names of the same byte (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x46D */ u8 unk46D;
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x46D */ u8 field_0x46D;
        };
    };
    /* +0x46E */ union { /* the original and later names of the same byte (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x46E */ u8 unk46E;
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x46E */ u8 field_0x46E;
        };
    };
    /* +0x46F */ u8 field_0x46F;
    /* +0x470 */ s16 unk470;
    /* +0x472 */ u8 unk472[0x47B - 0x472];
    /* +0x47B */ u8 field_0x47B;         /* per-player type index `fn_80267C84` reads */
    /* +0x47C */ u8 pad_0x47C[0x480 - 0x47C];
    /* +0x480 */ union { /* the original and later names of the same 96 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x480 */ union {   /* the 0x480..0x4DF run: the field spellings kept whole, with the
                                    * shell band's view of the actor's own attack entry inside it.  The union is
                                    * anchored at the word-aligned +0x480 and its run is 0x60 long, because a
                                    * member carrying the `u32` at +0x4A4 gives it align 4 - an odd-length run
                                    * would round up and grow `_PLW` */
                /* +0x480 */ u8 pad_0x480_run[0x4E0 - 0x480];
                struct {
                    /* +0x480 */ u8 pad_0x480[0x489 - 0x480];
                    /* +0x489 */ u8 unk489;
                    /* +0x48A */ u8 unk48A[0x492 - 0x48A];
                    /* +0x492 */ u8 unk492;
                    /* +0x493 */ u8 pad_0x493[0x11];
                    /* +0x4A4 */ u32 field_0x4A4;
                    /* +0x4A8 */ u8 pad_0x4A8[0x34];
                    /* +0x4DC */ u8 unk4DC;
                    /* +0x4DD */ u8 unk4DD[0x4E0 - 0x4DD];
                };
                struct {   /* the shell band's view: the actor's own attack entry, the record
                            * `Pl/pl_act.cpp` spells `_HIT_W` and `fn_80277974` fills in.  Only its address is
                            * named - the entry runs on into the fields of the view above */
                    /* +0x480 */ u8 pad_0x480_head[0x484 - 0x480];
                    /* +0x484 */ u8 hit_0x484[0x5C];
                };
            };
        };
        struct {   /* the later names, padded to the same byte total */
        /* +0x480 */ u8 pad_merge_0x480[0x4];
                                /* +0x484 */ u8 field_0x484;   /* the per-frame work block `hit_attack_list_push` resets */
        /* +0x485 */ u8 pad_merge_0x485[0x5B];
        };
    };
    /* +0x4E0 */ union { /* the original and later names of the same 5 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x4E0 */ u8 unk4E0[0x4E5 - 0x4E0];
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x4E0 */ u8 field_0x4E0;   /* the second per-frame work block `hit_attack_list_push` resets */
        /* +0x4E1 */ u8 pad_merge_0x4E1[0x4];
        };
    };
    /* +0x4E5 */ u8 unk4E5;
    /* +0x4E6 */ u8 unk4E6[0x4EE - 0x4E6];
    /* +0x4EE */ u8 unk4EE;
    /* +0x4EF */ u8 unk4EF[0x538 - 0x4EF];
    /* +0x538 */ u8 unk538;
    /* +0x539 */ u8 unk539[0x565 - 0x539];
    /* +0x565 */ u8 field_0x565;  /* the primary-act latch `Pl/pl_motion.cpp` sets */
    /* +0x566 */ union {   /* 0x566-0x57F: the `field_0x566` byte and its `unk567`/`unk568`
                            * filler runs, with the act's armed-weapon fields inside the same bytes and
                            * the `atk_act_flag` split in the byte view.  The union is anchored at
                            * the aligned +0x566 and NOT at +0x567: a union whose member contains the
                            * 16-bit `field_0x56E` aligns to 2, so at the odd +0x567 it would be pushed
                            * to +0x568 and rounded to 0x1A bytes, growing `_PLW` by 2 and shifting
                            * every later field (the byte total is the invariant) */
        struct {   /* the byte view, carrying a split of the same bytes */
            /* +0x566 */ u8 field_0x566;   /* the arming byte `Pl/pl_motion.cpp` sets once */
            /* +0x567 */ union {
                /* +0x567 */ u8 unk567[0x580 - 0x567];
                struct {
                    /* +0x567 */ union {   /* one byte, two spellings (rule 5) */
                        /* +0x567 */ u8 atk_act_flag;  /* ORed by `fn_8027E1B8`, masked by
                                                       * `Pl_atk_act_flag_ck` */
                        /* +0x567 */ u8 field_0x567;   /* the act's arming byte; the act
                                                       * entry clears bit 0 */
                    };
                    /* +0x568 */ u8 unk568[0x580 - 0x568];
                };
            };
        };
        struct {   /* the act-entry field view */
            /* +0x566 */ u8 pad_0x566;    /* `field_0x566` is declared in the member above */
            /* +0x567 */ u8 pad_0x567;    /* `atk_act_flag`/`field_0x567` are declared above */
            /* +0x568 */ u8 pad_0x568[0x2];
            /* +0x56A */ u8 field_0x56A;   /* the weapon record's level byte (`fn_802740F4`) */
            /* +0x56B */ u8 field_0x56B;   /* the armed weapon class the act-kind deltas index */
            /* +0x56C */ u8 pad_0x56C[0x2];
            /* +0x56E */ s16 field_0x56E;  /* the armed attack value */
            /* +0x570 */ s8 field_0x570;   /* the ranged attack value `Pl_critical_get` reads */
            /* +0x571 */ u8 pad_0x571[0xF];
        };
    };
    /* +0x580 */ s16 unk580;
    /* +0x582 */ u8 unk582;
    /* +0x583 */ union {
        u8 shell_ang_0x583; /* the player's shell-frame angle: `ef/eft053.cpp`'s shell
                             * projection reads it as a signed per-frame step (its sign
                             * selects the 45/35 degrees it adds) */
        s8 shell_ang_s_0x583;   /* the signed view `Pl/pl_act.cpp` clamps to +/-100 */
    };
    /* +0x584 */ union {   /* the 0x14-byte run; `pl_act_step_offhand_gesture` split it at +0x596
                           * and its head is split again - one member per offset, so the
                           * +0x584 head pair is named once, in the second view below */
        /* +0x584 */ u8 pad_0x584[0x14];
        struct {
            /* +0x584 */ u8 unk584[0x596 - 0x584];
            /* +0x596 */ u8 field_0x596;   /* the act's "no pitfall" latch `Pl/pl_act_step.cpp` arms */
            /* +0x597 */ u8 field_0x597;
        };
        struct {
            /* +0x584 */ u8 field_0x584;   /* set to 1 by the "2" act entry (`fn_80275ADC`) */
            /* +0x585 */ u8 field_0x585;   /* non-zero takes the act's alternate entry path; the
                                            * chase step's re-arm latch `Pl/pl_act_step.cpp`'s
                                            * `fn_80247EF0` clears when it enters the chase */
            /* +0x586 */ u8 pad_0x586[0x12];
        };
    };
    /* +0x598 */ u16 field_0x598;   /* the three act fields `fn_802DE578` is handed the address of
                                    * (`Pl/pl_act_step.cpp` reads it) */
    /* +0x59A */ union {   /* one u16, two spellings */
        /* +0x59A */ u8 pad_0x59A[0x2];
        /* +0x59A */ u16 field_0x59A;
    };
    /* +0x59C */ union {   /* the three armed-motion words (`x | 0x8000`, `Pl/pl_act.cpp:1067`) */
        /* +0x59C */ u16 field_0x59C[3];
        struct {          /* the view of the same six bytes: three separate u16s */
            /* +0x59C */ u16 unk59C;
            /* +0x59E */ u16 unk59E;
            /* +0x5A0 */ u16 unk5A0;
        };
    };
    /* +0x5A2 */ u8 unk5A2[0x5A4 - 0x5A2];
    /* +0x5A4 */ u16 field_0x5A4;
    /* +0x5A6 */ u8 unk5A6;
    /* +0x5A7 */ u8 field_0x5A7;
    /* +0x5A8 */ union { /* the original and later names of the same 16 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x5A8 */ u8 pad_0x5A8[0x10];
        };
        struct {   /* the later names, padded to the same byte total */
        /* +0x5A8 */ u8 pad_merge_0x5A8[0x3];
        /* +0x5AB */ u8 area_cell_0x5AB;   /* the cell row `stage_cell_get` reads for the player's spot */
                                /* +0x5AC */ u8 field_0x5AC;
        /* +0x5AD */ u8 pad_merge_0x5AD[0xB];
        };
    };
    /* +0x5B8 */ s16 field_0x5B8;  /* the guard timer `pl_act_name_row_get` gates the table row on (>= 0xA0) */
    /* +0x5BA */ u8 field_0x5BA;
    /* +0x5BB */ u8 unk5BB;
    /* +0x5BC */ u8 field_0x5BC;   /* `hud/cockpit_quest.cpp`'s `quest_mark_visible_ck` gates the quest
                                  * mark on this byte being 0; meaning not yet derived (GUESS, was
                                  * `unk5BC` - both are one byte, so no offset moves) */
    /* +0x5BD */ u8 pad_0x5BD[0x5C4 - 0x5BD];
    /* +0x5C4 */ union { /* the original and later names of the same 36 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x5C4 */ union {
                /* +0x5C4 */ u8 unk5C4;
                /* +0x5C4 */ u8 field_0x5C4;  /* cleared by the act entry (`fn_80275C34`) */
            };
            /* +0x5C5 */ union {   /* the 0x20-byte run 0x5C5-0x5E4, with the armed-slot bytes inside */
                /* +0x5C5 */ u8 unk5C5[0x5E5 - 0x5C5];
                struct {
                    /* +0x5C5 */ union {   /* the run's 3-byte head, two spellings (rule 5) */
                        /* +0x5C5 */ u8 pad_0x5C5[0x3];
                        /* +0x5C5 */ u8 unk5C5_start[0x5C8 - 0x5C5];
                        struct {
                            /* +0x5C5 */ u8 held_attack_0x5C5;   /* the held-attack flag the master band's action dispatchers test */
                            /* +0x5C6 */ u8 field_0x5C6;   /* cleared by the paired act entry (`Pl_act_pair_enter`) */
                            /* +0x5C7 */ u8 pad_0x5C7_tail;
                        };
                    };
                    /* +0x5C8 */ u8 field_0x5C8;   /* the armed weapon class `fn_802745DC` reads; non-zero
                                                    * also lets `fn_8027D6DC` send its motion
                                                    * request */
                    /* +0x5C9 */ u8 field_0x5C9;   /* set once the armed value table has been read; the latch
                                                    * `fn_8027D6DC` sets on every call */
                    /* +0x5CA */ union {   /* the 0x1B-byte tail, two spellings (rule 5) */
                        /* +0x5CA */ u8 pad_0x5CA[0x1B];
                        /* +0x5CA */ u8 user_profile_0x5CA[0x5E5 - 0x5CA]; /* the 17 bytes `arena_eqdata_from_userdata` copies out of the game's user-data block (its offsets 3..19); GUESS - the run has no name in the dump */
                    };
                };
            };
            /* +0x5E5 */ u8 unk5E5;
            /* +0x5E6 */ union {   /* one u8, two spellings (rule 5) */
                /* +0x5E6 */ u8 unk5E6;
                /* +0x5E6 */ u8 field_0x5E6;  /* the act's boolean latch (`fn_8025ECF0` toggles it) */
            };
            /* +0x5E7 */ u8 unk5E7;
        };
        struct {   /* the later names, padded to the same byte total */
        /* +0x5C4 */ u8 pad_merge_0x5C4[0x3];
                                /* +0x5C7 */ u8 field_0x5C7;
        /* +0x5C8 */ u8 pad_merge_0x5C8[0x20];
        };
    };
    /* +0x5E8 */ s16 unk5E8;
    /* +0x5EA */ u8 pad_0x5EA[0x8];
    /* +0x5F2 */ u16 unk5F2[8];
    /* +0x602 */ u8 unk602[8];
    /* +0x60A */ u8 unk60A[0x612 - 0x60A];
    /* +0x612 */ u16 deco_skill_id[4];
    /* +0x61A */ u16 unk61A[8];
    /* +0x62A */ u8 unk62A[8];
    /* +0x632 */ u8 unk632[0x634 - 0x632];
    /* +0x634 */ u32 unk634;
    /* +0x638 */ u32 unk638;
    /* +0x63C */ u32 unk63C;
    /* +0x640 */ u32 unk640;
    /* +0x644 */ union {   /* 0x644-0x64F: 12 bytes, the padding block with the `unk64F`
                            * byte inside it and the split of the same bytes that `Pl/pl_act.cpp`
                            * and `Pl/pl_act_step.cpp` share - one member per offset in the split
                            * view, with the names kept.  The union reaches +0x64F so its own
                            * size stays even: an 11-byte union holding the 2-byte `field_0x646` aligns
                            * to 2 and would round up to 12, shifting every later field (the
                            * byte total is the invariant) */
        struct {   /* the padding view: the 11-byte padding run plus the +0x64F byte */
            /* +0x644 */ u8 pad_0x644[0xB];
            /* +0x64F */ u8 pad_0x64F;
        };
        struct {   /* the split both units read; every byte named once */
            /* +0x644 */ u8 pad_0x644_pair[0x1];
            /* +0x645 */ u8 field_0x645;  /* the input-hold latch `Pl/pl_act_step.cpp`'s `fn_80245E20`
                                           * reports (the pad layer's answer is dropped while it is
                                           * set) */
            /* +0x646 */ s16 field_0x646;  /* cleared by the act entry unless the act keeps them */
            /* +0x648 */ s16 field_0x648;
            /* +0x64A */ s16 field_0x64A;
            /* +0x64C */ u8 pad_0x64C[0x3];
            /* +0x64F */ union {   /* one byte, three spellings (rule 5) */
                /* +0x64F */ u8 unk64F;
                /* +0x64F */ u8 field_0x64F;  /* cleared by the act entry */
                /* +0x64F */ s8 gauge_0x64F;  /* the signed (-100..100) gauge `Pl/pl_act.cpp` accumulates and scales */
            };
        };
    };
    /* +0x650 */ u16 field_0x650;   /* the id `Pl_item_timer_get`/`pl_item_add` are handed */
    /* +0x652 */ s16 field_0x652;   /* the amount `fn_8027D76C` compares against and negates */
    /* +0x654 */ union { /* the original and later names of the same 16 bytes (rule 5) */
        struct {   /* the original layout: every member keeps its own offset */
            /* +0x654 */ u8 pad_0x654[0x1]; /* 0x650-0x655 is otherwise padding; the split keeps
                                            * the parent's layout and the run's byte total */
            /* +0x655 */ u8 field_0x655;
            /* +0x656 */ union {   /* the padding run; the act-end flags named here */
                /* +0x656 */ u8 pad_0x656[0x8];
                struct {
                    /* +0x656 */ u8 act_end_request;      /* nonzero asks the running act handler to end; `1`
                                                          * selects the alternate end path */
                    /* +0x657 */ u8 act_handler_entered;  /* every act handler sets it on entry */
                    /* +0x658 */ u8 pad_0x658[0x6];
                };
            };
            /* +0x65E */ u8 field_0x65E;
            /* +0x65F */ union {   /* the 3-byte run; the action/handler cluster split it at +0x660 */
                /* +0x65F */ u8 unk65F[0x662 - 0x65F];
                struct {
                    /* +0x65F */ u8 field_0x65F;
                    /* +0x660 */ u8 pad_0x660[0x2];
                };
            };
            /* +0x662 */ s16 field_0x662;
        };
        struct {   /* the later names, padded to the same byte total */
                                /* +0x654 */ u8 field_0x654;
        /* +0x655 */ u8 pad_merge_0x655[0x1];
                                /* +0x656 */ u8 field_0x656;
                                /* +0x657 */ u8 field_0x657;
                                /* +0x658 */ u8 field_0x658;   /* the 90-frame lockout `fn_80262688` sets */
        /* +0x659 */ u8 pad_merge_0x659[0x3];
                                /* +0x65C */ u8 field_0x65C;
                                /* +0x65D */ u8 field_0x65D;
        /* +0x65E */ u8 pad_merge_0x65E[0x6];
        };
    };
    /* +0x664 */ union {
        u16 field_0x664;
        s16 field_0x664_s;   /* the signed view `Pl/pl_act.cpp` tests against 0 */
    };
    /* +0x666 */ u16 field_0x666;
    /* +0x668 */ u8 talk_wait_0x668;    /* re-armed to 5 each frame a lobby NPC talk runs; 0 ends the talk */
    /* +0x669 */ u8 talk_left_0x669;    /* the talks the NPC still has (3 once the progress flag is set); swaps open at 6 or less */
    /* +0x66A */ u8 pad_0x66A[0x2];
    /* +0x66C */ u16 trade_want_0x66C;   /* the item the quest NPC asks for (`npc_trade_pick`) */
    /* +0x66E */ s16 trade_count_0x66E;  /* -1 once the trade is settled, else the count */
    /* +0x670 */ u16 trade_give_0x670;   /* the item the NPC gives for it */
    /* +0x672 */ u8 pad_0x672[0x482];
    /* +0xAF4 */ struct _se_w* field_0xAF4;
    /* +0xAF8 */ struct _se_w* field_0xAF8;
    /* +0xAFC */ struct _se_w* field_0xAFC;
    /* +0xB00 */ u8 field_0xB00;   /* the lobby act latch (`fn_80208A3C`/`fn_80208A48` write it) */
    /* +0xB01 */ u8 field_0xB01;
    /* +0xB02 */ u8 field_0xB02;
    /* +0xB03 */ u8 field_0xB03;
    /* +0xB04 */ u8 field_0xB04;   /* `fn_80224AC4`: non-zero suppresses the up-swing gate;
                                    * the lobby's latch block runs to this byte as well (+0xB00..+0xB04) */
    /* +0xB05 */ u8 name_0xB05[10];  /* the hunter name the lobby compares with the move work */
    /* +0xB0F */ u8 pad_0xB0F[0xB];
    /* +0xB1A */ u8 field_0xB1A;
    /* +0xB1B */ u8 pad_0xB1B[0x1];
    /* +0xB1C */ u8 user_profile_0xB1C[0x4];  /* the four profile bytes `arena_eqdata_from_vsuser`
                                               * serialises into the arena user-data record's bytes
                                               * 9/10/12/13; GUESS - derived from that body's four
                                               * stores, see the unit header */
};

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PLW_H */
