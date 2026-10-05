/* sound/quest_snd.cpp - the quest sound work routines
 *
 * `.text` 0x800EE014..0x800EEAE0, 6 functions written (the rest of the range is not decompiled yet).
 * Each function keeps the `#pragma` state it had in its retired source.
 */

#include "types.h"
#include "sound/sound_work.h"

extern "C" void fn_800EE524(SndCopy28* dst, SndCopy28* src);
extern "C" u32 fn_800EE868(SndModeWord* self, u32 v);
extern "C" u32 fn_800EE8B4(SndModeWord* self, u32 v);
extern "C" void* fn_800EE9EC(u32 size);

#pragma peephole off

/* Copies one three-half-word record. */
extern "C" void fn_800EE508(SndTri16* dst, SndTri16* src)
{
    dst->a = src->a;
    dst->b = src->b;
    dst->c = src->c;
}

/* Frees an allocation back to the expand heap. */
extern "C" void fn_800EEA2C(void* p)
{
    if (p == 0) {
        return;
    }
    MEMFreeToExpHeap((void*)lbl_80794A20, p);
}

/* Copies one 0x28-byte five-pair record. */
extern "C" void fn_800EE524(SndCopy28* dst, SndCopy28* src)
{
    *dst = *src;
}

/* Scales `v` by the mode in the word at +0x04. */
extern "C" u32 fn_800EE868(SndModeWord* self, u32 v)
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
extern "C" u32 fn_800EE8B4(SndModeWord* self, u32 v)
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
extern "C" void* fn_800EE9EC(u32 size)
{
    void* p = 0;

    if (size != 0) {
        p = MEMAllocFromAllocator(lbl_806A1110, size);
    }
    return p;
}

/* The expand-heap handle (`.sbss`) and the allocator record (`.bss`, 0x10 B) this unit owns: `fn_800EEA2C` frees into the one, `fn_800EE9EC` allocates from the other. */
u32 lbl_80794A20;
u8 lbl_806A1110[0x10];
