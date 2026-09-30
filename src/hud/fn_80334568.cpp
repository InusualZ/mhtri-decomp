/* hud/fn_80334568.cpp - the character-state network sync (`.text` 0x80334568..0x80338808, 77
 * functions / 17056 B).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `tools/symbols/symedit.py list --section .text 0x80334568 0x80338808`: every row in the range is a
 * bare `fn_` stem, and `tools/symbols/dumpmap.py lookup` answers `zz_0334568_` for it), so every body
 * below keeps the map's stem.
 *
 * Home, name and evidence (brief section 2):
 *   - class 4 (nothing supports a name).  No `.data`/`.sdata` reference of the range is a source-file
 *     name: the range's only data refs are `lbl_805E1ED0`, the three switch tables
 *     (`jumptable_805E0EA0`, `jumptable_805E2788`, `jumptable_805E27B0`) and the `.sdata2` constant
 *     `lbl_8079B2B0`, and the shared runtime dump answers `zz_`/`FUN_` for all 77 addresses.  The file
 *     therefore keeps the map's stem.
 *   - module `hud`, decided by the orchestrator on this evidence: `cflags_hud` is the only registered
 *     group whose flags reproduce the target bytes - `-opt nopeephole` (8 of the range's 75 target
 *     objects keep a `clrlwi`/`rlwinm` before a narrowing store that the peephole folds, and none
 *     carries a record-form instruction) and `-Cpp_exceptions on` (the range's extab is 67 8-byte
 *     records and its extabindex 67 12-byte records, exactly one per framed function of the 77).
 *     `cflags_pl` and `cflags_menu` are token-identical, so the flags do not choose between them;
 *     `hud` is also the nearest *preceding* registered unit (`hud/fn_80324F7C.c`, ends 0x803250B0).
 *     Honest caveat: the **content is network, not HUD** - every builder of this unit packs a `_PLW`
 *     or `_ENEMY_WORK` record into a local message and sends it with `broadcastSessionCommand`, the
 *     `NetworkSessionManagerPat` slot 0x128 send guarded by `isServerSelectState()` (`net_ctrl_wk->0x11 == 7`).
 *     If a `Network` lib ever becomes the right home for the multiplayer sync, this unit is the first
 *     candidate to re-home.
 *   - the seam is unproven (brief section 8.3): `tudiscover.py at 0x80334568` must-links only
 *     `fn_80334568` (the asm graph is empty here, so it offers weak cuts at 0x8033112C and 0x803346B4),
 *     and the range is a single maximal unclaimed run whose neighbours are `hud/fn_80324F7C.c` below and
 *     `enemy/...` above.  `include/unsplit/unknown.h` already records this band as module-undecided - it
 *     declares `fn_80335CE8` (0x80335CE8, inside this range) with the note "(hud below, enemy above)
 *     names different modules".  The extent settles as the bodies match.
 *
 * Sections this unit owns: .text 0x80334568..0x80338808, extab 0x8001677C..0x80016994 and extabindex
 * 0x800359A0..0x80035CC4 - 67 records of 8 and of 12 bytes, one per framed function (10 of the 77 have
 * no frame and carry none).  No `.data`/`.sdata`/`.sdata2` range is claimed: the three switch tables
 * belong to the functions that emit them and a partial pool claim is not linkable (playbook 23).
 *
 * Flags: the lib's `cflags_hud` - see `configure.py`'s group evidence and the measurement above.
 *
 * Score at this commit (official report metric against this worktree's own split of the range):
 * **15.66 % fuzzy, 2488 of 17056 `.text` bytes**; 19 of the 77 bodies are written and **18 of them are
 * byte-identical**: fn_803346B4, fn_80334A34, fn_80334A4C, fn_80334C60, fn_8033502C, fn_80335114,
 * fn_80335200, fn_80335328, fn_8033535C, fn_8033536C, fn_80335468, fn_80335574, fn_80335B1C,
 * fn_80335CE8, fn_80335DD8, fn_8033737C, fn_803374B0, fn_80337648 (the per-symbol table is in the
 * outbox).  `.data` 40/40 B pair (the claimed switch table) and `build/RMHE08/main.dol: OK`.
 *
 * Residuals of the written bodies:
 *   - `fn_803356F0` 99.89 %.  Every instruction and the size match; one row differs and it is objdiff's,
 *     not the code's: the forward branch to the shared epilogue is printed as an absolute `.text`
 *     address in each object (`b 0x5ac` ours against `b 0x120c` target) because our object holds only
 *     this unit's 19 functions while the target holds all 77, so the row cannot pair until the rest of
 *     the range is written.
 *   - `fn_803346B4` reached 100 % with `plw->act_step_0x05 += 1` where `= plw->act_step_0x05 + 1`
 *     emitted a redundant `clrlwi` before the narrow store (playbook 38: a compound assignment keeps the
 *     store raw).  Turning the peephole pass *on* would also close this row but costs the rest of the
 *     unit - measured, `-opt peephole` against `cflags_hud`'s `-opt nopeephole`: fn_80335CE8 100 ->
 *     85.58, fn_80335DD8 100 -> 73.33, fn_8033737C 100 -> 89.16, fn_803356F0 99.89 -> 89.02,
 *     fn_80335468 100 -> 97.31, fn_8033502C 100 -> 96.90, fn_80335200 100 -> 97.57 - so the lib's flag
 *     stands and the source shape is the lever.
 *
 * Not written (58, in address order).  Blockers, measured rather than guessed:
 *   - 48 are blocked by bytes the shared headers do not name, so writing them today would be a rule-6
 *     pointer-arithmetic violation: `_PLW` (include/pl.h) names neither +0x26C (`unk26C`), +0x3B8
 *     (`unk3B8`) nor +0x090 (`unk090`), and `_ENEMY_WORK` (include/enemy.h) names neither +0x016,
 *     +0x01E, +0x468, +0x89F, +0x8A6, +0x91C, +0x94C nor +0x954.  The 0x4C-byte player pack/unpack
 *     pair - `fn_80334A9C` (452 B) and `fn_80334C94` (920 B), the range's two largest bodies - needs
 *     +0x26C/+0x3B6/+0x3B8 and +0x090; `fn_80335148`, `fn_8033537C` and `fn_803355A8` need +0x090.
 *     The fold is the `shared-file` request in this unit's outbox.
 *   - 10 are the enemy packers/receivers whose wire layout is only partly known (`fn_8033609C` 944 B,
 *     `fn_80336488` 1120 B, `fn_803368E8` 404 B, `fn_80336AB0` 612 B, `fn_80336D14`/`fn_80336EA0`/
 *     `fn_80337104`/`fn_80337224` 148 B each, `fn_80336F84` 204 B, plus `fn_8033737C`'s seven
 *     receivers): `NetEmStateMsg`'s tail is a stated approximation here.
 *   - The extab/extabindex rows (36.6 % / 47 %) close with the remaining framed bodies, not with a
 *     source change; both runs are claimed correctly (67 records each).
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "pl.h"
#include "enemy.h"
#include "nw4r/math.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef.h"
#include "ef/fn_800CDB2C.h"
#include "Pl/pl_act.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_80273B14.h" /* the owner header of the act-motion setters (rule 2) */
#include "hud/fn_80334568.h"
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */

/* Advances the act-change state machine: step 0 enters the act (its motion, its SE and the paired
 * `Pl_act_set_motion` step) and step 1 waits for the act's frame check before handing the act's motion on. */
void fn_803346B4(_PLW* plw) {
    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 0, 0);
        Pl_chr_set_attr_default(plw, 0x494, 4, 0x5A);
        Pl_act_set_step_table(plw, (u32)lbl_805E1ED0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        }
        break;
    }
}

/* Writes the 4-byte wire header every sender of this unit starts its message with. */
void fn_80334A34(NetMsgHeader* hdr, u8 from, u8 to, u8 kind) {
    hdr->reserved = 0;
    hdr->from_slot = from;
    hdr->to_slot = to;
    hdr->kind = kind;
}

/* Reports whether this client may send player state: the local move work exists and either the player
 * is in a syncable act or the act layer says so. */
u32 fn_80334A4C(void) {
    if (get_move_work_adrs(0) == NULL) {
        return 0;
    }
    if (fn_8028BD54() == 1) {
        return 1;
    }
    return fn_8027BC48(0);
}

/* Constructs the 0x4C-byte player-state message's position sub-object and returns the message. */
void* fn_80334C60(NetPlStateMsg* msg) {
    VEC3_ctor(&msg->pos);
    return msg;
}

/* Builds and sends the 0x30-byte player message from the caller's position record and the player work's
 * health pair, act number and two parameter words. */
void fn_8033502C(_PLW* plw, u8 from, u8 to, u8 kind) {
    NetPlPosMsg msg;

    fn_80335114(&msg);
    memset(&msg, 0, sizeof(msg));
    fn_80334A34(&msg.hdr, from, to, kind);
    copyVec3(&msg.pos, &plw->vec_0x03C);
    msg.field_0x10 = plw->field_0x0AC;
    msg.field_0x14 = plw->field_0x0A8;
    msg.field_0x18 = 0;
    msg.kind_0x15 = plw->kind_0x015;
    msg.area_0x16 = plw->area_0x16;
    msg.field_0x3d8 = plw->field_0x3D8;
    msg.field_0x3dc = plw->field_0x3DC;
    msg.health = plw->health;
    msg.health_max = plw->health_max;
    msg.field_0x37a = plw->field_0x37A;
    msg.field_0x37e = plw->field_0x37E;
    broadcastSessionCommand(&msg, 0x30);
}

/* Constructs the 0x30-byte player message's position sub-object and returns the message. */
void* fn_80335114(NetPlPosMsg* msg) {
    VEC3_ctor(&msg->pos);
    return msg;
}

/* Builds and sends the 0x28-byte attack message for the local player, but only when the network session
 * is up, this client may send, and the addressed move work really belongs to the local slot. */
void fn_80335200(u8 attack_kind, const VEC3* pos, u16 param, u8 kind) {
    NetPlAtkMsg msg;
    u8 my;
    u8 next;
    PlMoveWork* work;

    fn_80335328(&msg);
    memset(&msg, 0, sizeof(msg));
    if (isServerSelectState() == 0) {
        return;
    }
    if (fn_80334A4C() == 0) {
        return;
    }
    my = (u8)my_player_no();
    next = my + 1;
    work = (PlMoveWork*)get_move_work_adrs(2);
    if (work == NULL) {
        return;
    }
    work = &work[my];
    if (work->pl.chunk_ofs != my) {
        return;
    }
    fn_80334A34(&msg.hdr, my, next, kind);
    copyVec3(&msg.pos, pos);
    msg.field_0x10 = 0;
    msg.field_0x14 = param;
    msg.field_0x18 = 0;
    msg.kind_0x15 = work->pl.kind_0x015;
    msg.attack_kind = attack_kind;
    msg.field_0x01F = fn_80274808(&work->pl);
    msg.field_0x416 = work->pl.field_0x416;
    msg.field_0x41c = work->pl.field_0x41C;
    msg.field_0x5c4 = work->pl.field_0x5C4;
    msg.field_0x009 = work->pl.kind_0x09;
    msg.field_0x001 = work->pl.field_0x001;
    work->pl.field_0x645 = 1;
    broadcastSessionCommand(&msg, 0x28);
}

/* Constructs the 0x28-byte attack message's position sub-object and returns the message. */
void* fn_80335328(NetPlAtkMsg* msg) {
    VEC3_ctor(&msg->pos);
    return msg;
}

/* Sends the attack message with the caller's attack kind and position, tagged as kind 3. */
void fn_8033535C(u8 attack_kind, const VEC3* pos, u16 param) {
    fn_80335200(attack_kind, pos, param, 3);
}

/* Sends the attack message with the caller's attack kind and position, tagged as kind 5. */
void fn_8033536C(u8 attack_kind, const VEC3* pos, u16 param) {
    fn_80335200(attack_kind, pos, param, 5);
}

/* Builds and sends the 0x38-byte player message: the position, the running act, the health pair and the
 * armed weapon's id/timer pair. */
void fn_80335468(_PLW* plw, u8 from, u8 to, u8 kind, u16 param) {
    NetPlExtraMsg msg;

    fn_80335574(&msg);
    memset(&msg, 0, sizeof(msg));
    fn_80334A34(&msg.hdr, from, to, kind);
    msg.act_mode = plw->field_0x018;
    msg.field_0x04 = param;
    msg.pos = plw->vec_0x03C;
    msg.field_0x14 = plw->field_0x0AC;
    msg.field_0x18 = plw->field_0x0A8;
    msg.act_kind = plw->field_0x00A;
    msg.act_no = plw->act_no;
    msg.field_0xb6 = plw->field_0x0B6;
    msg.health = plw->health;
    msg.health_max = plw->health_max;
    msg.field_0x001 = plw->field_0x001;
    msg.kind_0x15 = plw->kind_0x015;
    msg.area_0x16 = plw->area_0x16;
    msg.field_0x37e = plw->field_0x37E;
    msg.field_0x650 = plw->field_0x650;
    msg.field_0x652 = plw->field_0x652;
    msg.field_0x655 = plw->field_0x655;
    broadcastSessionCommand(&msg, 0x38);
}

/* Constructs the 0x38-byte player message's position sub-object and returns the message. */
void* fn_80335574(NetPlExtraMsg* msg) {
    VEC3_ctor(&msg->pos);
    return msg;
}

/* Builds and sends the 0x0C-byte short player message: the primary act (kind 7) carries the act's
 * follow-up byte and the caller's value, every other act the follow-up byte, the weapon-class byte and
 * the armed weapon's id/timer pair. */
void fn_803356F0(_PLW* plw, u8 from, u8 to, u8 kind, u16 param) {
    NetPlShortMsg msg;

    memset(&msg, 0, sizeof(msg));
    fn_80334A34(&msg.hdr, from, to, kind);
    if (kind == 7) {
        msg.field_0x06 = plw->chunk_ofs;
        msg.field_0x07 = param;
    } else {
        msg.field_0x04 = param;
        msg.field_0x06 = plw->field_0x656;
        msg.field_0x07 = plw->chunk_ofs;
        msg.field_0x650 = plw->field_0x650;
        msg.field_0x652 = plw->field_0x652;
    }
    broadcastSessionCommand(&msg, 0xC);
}

/* Constructs the 0x4C-byte player-state message's position sub-object and returns the message. */
void* fn_80335B1C(NetPlStateMsg* msg) {
    VEC3_ctor(&msg->pos);
    return msg;
}

/* The client's own sender: when the session is up, this client may send and the addressed move work
 * really is the local slot's, dispatches on the act kind to the matching message builder. */
void fn_80335CE8(_PLW* plw, u8 kind, u16 param) {
    u8 my;
    u8 next;

    if (isServerSelectState() == 0) {
        return;
    }
    if (fn_80334A4C() == 0) {
        return;
    }
    my = (u8)my_player_no();
    next = my + 1;
    if (plw->chunk_ofs != my) {
        return;
    }
    switch (kind) {
    case 1:
        fn_80334A9C(plw, my, next, kind, param);
        break;
    case 2:
        fn_8033502C(plw, my, next, kind);
        break;
    case 6:
        fn_80335468(plw, my, next, kind, param);
        break;
    case 7:
    case 8:
        fn_803356F0(plw, my, next, kind, param);
        break;
    case 9:
        fn_803359A0(plw, my, next, kind, param);
        break;
    }
}

/* The client's own receiver: dispatches a received player message on its kind to the matching applier,
 * ignoring messages this client sent itself. */
void fn_80335DD8(u8 slot, const NetMsgHeader* msg) {
    s8 own;

    if (fn_80334A4C() == 0) {
        return;
    }
    own = (s8)my_player_no();
    if (msg->from_slot == own) {
        return;
    }
    switch (msg->kind) {
    case 1:
        fn_80334C94(slot, (const NetPlStateMsg*)msg);
        break;
    case 2:
        fn_80335148(slot, (const NetPlPosMsg*)msg);
        break;
    case 3:
        fn_8033537C(slot, (const NetPlExtraMsg*)msg, 0);
        break;
    case 5:
        fn_8033537C(slot, (const NetPlExtraMsg*)msg, 1);
        break;
    case 6:
        fn_803355A8(slot, (const NetPlExtraMsg*)msg);
        break;
    case 7:
        fn_803357A8(slot, (const NetPlExtraMsg*)msg);
        break;
    case 8:
        fn_803358D8(slot, (const NetPlExtraMsg*)msg);
        break;
    case 9:
        fn_80335B50(slot, (const NetPlStateMsg*)msg);
        break;
    }
}

/* The client's own enemy sender: when the session is up, this client may send and the enemy carries an
 * id, dispatches on the enemy act kind to the matching message builder. */
void fn_8033737C(_ENEMY_WORK* work, u8 kind, u16 param) {
    u8 own;

    if (isServerSelectState() == 0) {
        return;
    }
    if (work->field_0x01A == 0xFFFF) {
        return;
    }
    if (fn_80334A4C() == 0) {
        return;
    }
    own = (u8)my_player_no();
    switch (kind) {
    case 1:
        fn_8033609C(work, own, 5, kind, param);
        break;
    case 2:
        fn_803368E8(work, own, 5, kind, param);
        break;
    case 3:
        fn_80336D14(work, own, 5, kind, param);
        break;
    case 4:
        fn_80336EA0(work, own, 5, kind, param);
        break;
    case 5:
        fn_80336F84(work, own, 5, kind, param);
        break;
    case 6:
        fn_80337104(work, own, 5, kind, param);
        break;
    case 7:
        fn_80337224(work, own, 5, kind, param);
        break;
    }
}

/* The client's own enemy receiver: resolves the message's enemy id and dispatches on its kind to the
 * matching applier, ignoring messages this client sent itself. */
void fn_803374B0(_ENEMY_WORK* unused_work, const NetEmStateMsg* msg) {
    _ENEMY_WORK* work;
    s8 own;

    if (fn_80334A4C() == 0) {
        return;
    }
    own = (s8)my_player_no();
    if (msg->hdr.from_slot == own) {
        return;
    }
    if ((u8)em_get_unique_work(msg->enemy_id, &work, NULL) != 1) {
        return;
    }
    switch (msg->hdr.kind) {
    case 1:
        fn_80336488(work, msg);
        break;
    case 2:
        fn_80336AB0(work, msg);
        break;
    case 3:
        fn_80336DA8(work, msg);
        break;
    case 4:
        fn_80336F34(work, msg);
        break;
    case 5:
        fn_80337050(work, msg);
        break;
    case 6:
        fn_80337198(work, msg);
        break;
    case 7:
        fn_803372B8(work, msg);
        break;
    }
}

/* Copies the 8-byte per-enemy status record the enemy sender reads out of an enemy work record. */
void fn_80337648(NetEmStatus* dst, const NetEmStatus* src) {
    dst->field_0x0 = src->field_0x0;
    dst->field_0x2 = src->field_0x2;
    dst->field_0x3 = src->field_0x3;
    dst->field_0x4 = src->field_0x4;
    dst->field_0x5 = src->field_0x5;
    dst->field_0x6 = src->field_0x6;
    dst->field_0x7 = src->field_0x7;
}
