/* sound/se_req.cpp - the SE (`se_w`) request cluster's middle: the `_PLW`-keyed request helpers, `map_se_req`, `shell_se_req`, the ryugeki SE
 * requests, `.text` 0x800DD40C..0x800E0504 (35 symbols, 0x30F8 bytes), `.data` 0xD8, `.sdata` 0x8, `.sdata2` 0x1C.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for the unsplit functions of the range (checked with symedit); the map's real names
 * (the `*_se_req` helpers) are used verbatim below.  The module is `sound`: the neighbours (`sound/fn_800D7F54.cpp`, `sound/mhchar.cpp`)
 * are `sound` and unsplit/sound.h is the band that declares this range's symbols for its consumers.  The language is C++: the map
 * carries C++ manglings (`shell_se_req__FP5_se_wPQ34nw4r4math4VEC3UcUl`, `map_se_req__FUcPQ34nw4r4math4VEC3`) and the range manipulates
 * `nw4r::math` types by value.
 *
 * Phase 4: this is what remains of the old `sound/fn_800DD1F0.cpp` (0x800DD1F0..0x800E3CBC), which held three TUs' worth.  The head
 * (0x800DD1F0..0x800DD40C: the map-dependent SE request and the `st_ice*` requests) is folded into `sound/fn_800D7F54.cpp`, the `MHchar`
 * model class is `sound/mhchar.cpp` and the job request `sound/sound_job.cpp`.  The unit is renamed from the placeholder stem
 * `fn_800DD1F0` (the address was never its start; GUESS `se_req`: it is the SE request cluster's body, no `__FILE__` string names it).
 *
 * What is reconstructed here: filled in per function as it is measured.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "sound/se.h"

/* The range's codegen carries the non-record forms the peephole pass would fold (`extsb` before a
 * compare and the `clrlwi` narrowing of the byte arguments), so the original TU was peephole-off - the
 * same per-unit pragma the SE cluster next door (sound/fn_800D7F54.cpp) carries. */
#pragma peephole off

/* --- the SE request layer's tail; `_PLW::field_0xAFC` is the `_se_w` the request lands in --- */




extern "C" SeSlot* fn_800DD504(nw4r::math::VEC3* pos)
{
    return fn_800DA72C(46, 64, pos);
}

extern "C" SeSlot* fn_800DD514(nw4r::math::VEC3* pos)
{
    return fn_800DA72C(46, 13, pos);
}

extern "C" void fn_800DD524(_PLW* self)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        return fn_800D9CC8(work, 0, 3, 3);
    }
}

extern "C" void fn_800DD544(_PLW* self, u32 flag)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id = ((u8)flag == 0) ? 1 : 2;
    return fn_800D9CC8(work, id, 3, 3);
}

extern "C" void fn_800DD574(_PLW* self, u32 arg)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id = 0;
    switch ((u8)arg) {
    case 0:
        id = 7;
        break;
    case 1:
        id = 8;
        break;
    case 2:
        id = 9;
        break;
    }
    return fn_800D9CC8(work, id, 3, 3);
}

extern "C" void fn_800DD5CC(_PLW* self)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        return fn_800D9CC8(work, 7, 3, 3);
    }
}

extern "C" void fn_800DD5EC(_PLW* self)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        return fn_800D9CC8(work, 0, 3, 3);
    }
}

extern "C" void fn_800DD60C(_PLW* self)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        return fn_800D9CC8(work, 2, 3, 3);
    }
}

extern "C" void fn_800DD62C(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 18 : 8;
        se_req_pos_ps(work, id, ((work->field_0x0C + 1) << 24) | 2, pos);
    }
}

extern "C" void fn_800DD69C(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 14 : 13;
        se_req_pos_ps(work, id, 2, pos);
    }
}

extern "C" void fn_800DD700(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 17 : 7;
        se_req_pos_ps(work, id, ((work->field_0x0C + 1) << 24) | 2, pos);
    }
}

extern "C" void fn_800DD770(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 17 : 7;
        se_req_pos_ps(work, id, ((work->field_0x0C + 1) << 24) | 2, pos);
    }
}

extern "C" void fn_800DD7E0(_PLW* self, nw4r::math::VEC3* pos, s8 flag)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id;
    switch (flag) {
    case 0:
        if (fn_800D8D8C(pos) == 1) {
            id = 12;
        } else {
            id = 10;
        }
        break;
    case 1:
        if (fn_800D8D8C(pos) == 1) {
            id = 13;
        } else {
            id = 11;
        }
        break;
    case -1:
        id = 0;
        break;
    default:
        return;
    }
    se_req_pos_ps(work, id, 2, pos);
}

/* The ryugeki (shell-shot) SE: bank 46 id selected by the caller's kind flag. */
SeSlot* ryugeki_shot_se_req(_PLW* self, nw4r::math::VEC3* pos, u8 kind)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return 0;
    }
    s32 id = kind ? 31 : 11;
    return se_req_pos_ps(work, id, 2, pos);
}

/* Clear the in-use flag of all 32 SE slots. */
extern "C" void fn_800DF948(_se_w* work)
{
    for (int i = 0; i < 32; i++) {
        work->slots[i].in_use = 0;
    }
}


/* Claim a free slot for the ryugeki positional SE (bank 46) and fill it in. */
extern "C" void fn_800DD9FC(_PLW* self, nw4r::math::VEC3* pos, u8 kind)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id = kind ? 30 : 10;
    SeSlot* slot = fn_800D8E58(work, id);
    if (!slot) {
        return;
    }
    slot->state = 3;
    slot->kind = 2;
    slot->owner = work->field_0x0C;
    slot->id = id;
    slot->field_0x30 = -1;
    slot->field_0x34 = -1;
    slot->param = 0;
    copyVec3(&slot->pos, pos);
    slot->field_0x10 = slot->pos;
}

/* Find the first live slot owned by `owner`/`id` whose hold is still positive. */
extern "C" SeSlot* fn_800DDF44(_se_w* work, u32 a4, s32 a5)
{
    SeSlot* slot = &work->slots[0];
    for (int i = 0; i < 32; i++, slot++) {
        if (slot->in_use == 0) {
            continue;
        }
        if (slot->field_0x03 == 0) {
            continue;
        }
        if (slot->field_0x44 != a4) {
            continue;
        }
        if (slot->owner != (u32)work->field_0x0C) {
            continue;
        }
        if (slot->id != (u32)a5) {
            continue;
        }
        if (slot->field_0x48 <= -1) {
            continue;
        }
        return slot;
    }
    return 0;
}
