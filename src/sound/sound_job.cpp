/* sound/sound_job.cpp - the primitive-record job request.
 *
 * RANGE. `.text` 0x800E3B3C..0x800E3CBC: split_screen_size_class, sound_job_request, prim_rec_alloc, prim_rec_init;
 *   `.sdata2` 0x80796450..0x80796458 (the 1000.0f / 10000.0f cull bounds prim_rec_init seeds, emitted as its
 *   literal pool); extab 0x8000B1C4..0x8000B1DC; extabindex 0x80024AEC..0x80024B10.
 * FLAGS. The `sound` lib's cflags_main (Wii/1.3); `#pragma peephole off` for the whole file.
 * NAMES. No `__FILE__` string names the TU; `sound_job` follows the map's `sound_job_request`.
 *   `split_screen_size_class`, `prim_rec_alloc` and `prim_rec_init` are GUESS names (from the bodies).
 * RESIDUALS. prim_rec_alloc: `list*4` and the loaded count swap r5/r6 (target `slwi r6` / `lwzx r5`); the same in
 *   every measured shape (local order, index local, pointer-typed pool, `peephole`/`scheduling` scoped).
 * SHAPES. `unsplit/sound.h`'s foreign `sound_job_request` is renamed away around its include.
 *   split_screen_size_class tests `== 0 ? 0x400 : 0x800` (`li 0x800` before the branch); the `!= 0` spelling
 *   or an if/return pair inverts the branch.
 *   prim_rec_alloc indexes the pool as a `PrimRec` array (`mulli 0x34` + `add`).
 */

#define MHTRI_SPLIT_SCREEN_SIZE_CLASS_TAKES_ACTOR 1

#include "types.h"
#include "nw4r/math.h"
#include "pl.h" /* _PLW (split_screen_size_class reads its +0x08 byte) */
#include "pad_connect.h" /* screen_split_mode_ck (rule 2) */
#include "mh3_pad/vec3.h" /* copyVec3 (rule 2) */
#include "sound/fn_800E3CBC.h" /* PrimRec, prim_rec_pool, prim_rec_count (rule 1) */
#define sound_job_request sound_job_request_band_spelling
#include "unsplit/sound.h" /* prim_rec_capacity (rule 2: the band) */
#undef sound_job_request
#include "sound/sound_job.h"
#include "sound/sound_job_request.h"

#pragma peephole off

/* 0x800E3B3C (0x50): Returns the actor's effect size class: 0x800 or 0x400 by its +0x08 byte on a split
 * screen, 0x8 otherwise. */
extern "C" u32 split_screen_size_class(struct _PLW* self)
{
    if (screen_split_mode_ck() != 0) {
        return self->chunk_ofs == 0 ? 0x400 : 0x800;
    }
    return 8;
}

/* 0x800E3B8C (0x90): Takes a record from list `list` and fills it; returns it, or null when the list is full. */
extern "C" PrimRec* sound_job_request(u8 list, u8 layer, s32 handle, u8 kind, u16 flags, const VEC3* pos,
                                      void (*callback)(PrimRec*))
{
    PrimRec* rec = prim_rec_alloc(list);
    if (rec != 0) {
        rec->layer = layer;
        rec->handle = (void*)handle;
        rec->kind = kind;
        rec->flags = flags;
        if (pos != 0) {
            copyVec3(&rec->pos, pos);
            rec->flags |= 1;
        }
        rec->callback = callback;
    }
    return rec;
}

/* 0x800E3C1C (0x74): Takes the next free record of list `list` and clears it; returns null when the list is full. */
extern "C" PrimRec* prim_rec_alloc(u8 list)
{
    u32 count = prim_rec_count[list];
    PrimRec* rec;
    if (count >= prim_rec_capacity[list]) {
        return 0;
    }
    rec = (PrimRec*)prim_rec_pool[list] + count;
    prim_rec_count[list] = count + 1;
    prim_rec_init(rec);
    return rec;
}

/* 0x800E3C90 (0x2C): Clears a record's state, handle, flags and callback and seeds its two cull bounds. */
extern "C" void prim_rec_init(PrimRec* rec)
{
    rec->handle = 0;
    rec->callback = 0;
    rec->flags = 0;
    rec->state = 0;
    rec->started = 0;
    rec->limit_lo = 1000.0f;
    rec->limit_hi = 10000.0f;
}
