/* sound/quest_snd.cpp - the quest sound work routines
 *
 * `.text` 0x800EE014..0x800EEAE0, 6 functions written (the rest of the range is not decompiled yet).
 * Each function keeps the `#pragma` state it had in its retired source.
 */

#include "types.h"
#include "sound/sound_work.h"

extern "C" void fn_800EE524(SndCopy28* dst, SndCopy28* src);
extern "C" u32 snd_format_bytes_to_addr(SndModeWord* self, u32 v);
extern "C" u32 snd_format_addr_to_bytes(SndModeWord* self, u32 v);
extern "C" void* snd_heap_alloc(u32 size);

#pragma peephole off

/* Copies one three-half-word record. */
extern "C" void fn_800EE508(SndTri16* dst, SndTri16* src)
{
    dst->a = src->a;
    dst->b = src->b;
    dst->c = src->c;
}

/* Frees an allocation back to the expand heap. */
extern "C" void snd_heap_free(void* p)
{
    if (p == 0) {
        return;
    }
    MEMFreeToExpHeap((void*)snd_exp_heap, p);
}

/* Copies one 0x28-byte five-pair record. */
extern "C" void fn_800EE524(SndCopy28* dst, SndCopy28* src)
{
    *dst = *src;
}

/* Scales `v` by the mode in the word at +0x04. */
extern "C" u32 snd_format_bytes_to_addr(SndModeWord* self, u32 v)
{
    u32 r = 0;

    switch (self->mode) {
    case 0:
    case 1:
        r = v << 1;
        break;
    case 2:
    case 3:
        r = v >> 1;
        break;
    case 4:
    case 5:
        r = v;
        break;
    }
    return r;
}

/* Scales `v` by the mode in the word at +0x04 (the inverse mapping). */
extern "C" u32 snd_format_addr_to_bytes(SndModeWord* self, u32 v)
{
    u32 r = 0;

    switch (self->mode) {
    case 0:
    case 1:
        r = v >> 1;
        break;
    case 2:
    case 3:
        r = v << 1;
        break;
    case 4:
    case 5:
        r = v;
        break;
    }
    return r;
}

/* Allocates `size` bytes from the global allocator (null for a zero request). */
extern "C" void* snd_heap_alloc(u32 size)
{
    void* p = 0;

    if (size != 0) {
        p = MEMAllocFromAllocator(snd_heap_allocator, size);
    }
    return p;
}

/* The expand-heap handle (`.sbss`) and the allocator record (`.bss`, 0x10 B) this unit owns: `snd_heap_free` frees into the one, `snd_heap_alloc` allocates from the other. */
u32 snd_exp_heap;
u8 snd_heap_allocator[0x10];
