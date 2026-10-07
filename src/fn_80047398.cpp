/*
 * The character face/skin render unit.  `.text` 0x80047398..0x80048964 (the loader/driver family around `face_work`, the
 * `drawSpr2TF` GX writers up to the second FIFO family); `.bss` 0x806694E8..0x806699B8 (`face_work`), `.sbss` 0x80794870..78.
 * Phase 4 cut the old 0x80047398..0x8004C9A0 run at 0x80048964: the part from there is `userdata_item.cpp` (user-data / item-table /
 * sprite-transform helpers, `.bss` 0x806699B8 on).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range - no `__FILE__` string, no assert, no runtime-dump name (class 1/2
 * fail), so the file keeps the map's `fn_80047398` stem (evidence class 3, the same outcome as `src/fn_80040598.cpp`) in the
 * game-root `main` lib band (`cflags_main`), next to `mh3_pad.cpp` which ends where this unit begins.  The mangled names the range
 * carries (`drawSpr2TF__FUcP9fltSpr2TFUc`, `subTransSetPrio__FUcUllPUl`, ...) are in `userdata_item.cpp`'s half and are never
 * spelled as callable identifiers (rule 9).
 *
 * What the unit does (from the bodies): it loads the male/female face, skin and `facemake` TPL textures through the 8-byte
 * `{size, name}` tables at 0x80582D38/0x80582E58/0x80582EE0, binds them as GX texture objects into a work block at `face_work`
 * (0x4D0 B, cleared by fn_800474A8, whose three `void*` slots start at 0x04), keeps a per-player face/frame/animation state there
 * (the byte arrays at 0x448, 0x456, 0x4B0, 0x4BA, 0x4C4 and the ten-entry pointer runs at 0x460/0x488) and draws the 2D face with an
 * orthographic GX pipeline (fn_80047D6C builds C_MTXOrtho/GXSetProjection and pushes TPL quads).
 *
 * Status: partial.  Written, in address order: the accessors and the face animation helpers up to `fn_80047D38` and the GX pipe
 * writers `fn_80048764..fn_800487C8`; the biggest unwritten are `fn_80047D6C` (0x9F8), `fn_800478F0` (0x200), `fn_80047634` (0x170).
 *
 * Residual: the four functions that read `Screen_w` (fn_80047398, fn_800473F4, fn_800478F0, fn_80047D6C) are NOT written:
 * `ScreenWork` is defined inside `main.cpp`, and reaching its fields from a second unit needs that type moved to `include/` first
 * (rule 1).  fn_80047634/fn_80047780 are declared and called by fn_80047884/fn_80047CAC but their load_file_req/load_file bodies
 * are unwritten (their real signatures come from the map's `load_file_req__FPcUllUllPUl` / `load_file__FPcUll`, which rule 9
 * forbids spelling as identifiers).
 *
 * RESIDUALS. The object emits no `.data` (0x28), `.sdata` (0x8) or `.sdata2` (0x38) for the claimed ranges yet (flip
 *   blockers); `fn_80047884` loads `lbl_80790EE8` where retail folds it, and `fn_80047CAC` saves r27..r31 through
 *   `_savegpr_27`/`_restgpr_27` in retail, ours does not; `fn_80047470`, `fn_800474A8`, `fn_80047B58` and `fn_80047BE0`
 *   carry an `@ha`/`@l` pair on `face_work` that retail's object does not relocate.
 *
 * Inventory / addresses / sizes: `python tools/units/ledger.py unit fn_80047398.cpp`.
 */

#include "types.h"
#include "gx.h"
#include "Runtime.PPCEABI.H/memset.h" /* owned by Runtime.PPCEABI.H/memset.c (rule 2) */
#include "RVLGX/GXAttr.h"

/* --- the unit's own work block, `face_work` (.bss 0x806694E8, 0x4D0 B) ------------------------ */

/* Byte arrays are indexed by the player (0/1); the two ten-entry pointer runs are the texture-base
 * addresses fn_800474A8 hands out (0x20000 apart) and they are contiguous (ptr488 = ptr460 + 10). */
typedef struct FaceWork {
    /* +0x000 */ u8 done;
    /* +0x001 */ u8 pad_001[3];
    /* +0x004 */ void* tex[3];      /* m_face / f_face / facemake TPL blobs (0x04, 0x08, 0x0C) */
    /* +0x010 */ u8 state[3];       /* 0, 1, 2 per loaded blob; the driver waits for all three = 2 */
    /* +0x013 */ u8 enabled[2];
    /* +0x015 */ u8 pad_015[8];
    /* +0x01D */ u8 colorA[2];      /* fn_80047C68 */
    /* +0x01F */ u8 pad_01F[9];
    /* +0x028 */ u8 pad_028[0x420]; /* the 33 GXTexObj slots fn_800478F0 binds (untouched by the written bodies) */
    /* +0x448 */ u8 face[2];        /* current face texture index */
    /* +0x44A */ u8 pad_44A[0xC];
    /* +0x456 */ u8 frame[2];       /* frame counter, wraps at 100 */
    /* +0x458 */ u8 pad_458[8];
    /* +0x460 */ void* ptr460[10];
    /* +0x488 */ void* ptr488[10];
    /* +0x4B0 */ u8 dirty[2];       /* set by every writer; cleared by the draw */
    /* +0x4B2 */ u8 pad_4B2[8];
    /* +0x4BA */ u8 mouth[2];
    /* +0x4BC */ u8 pad_4BC[8];
    /* +0x4C4 */ u8 colorB[2];
    /* +0x4C6 */ u8 pad_4C6[10];
} FaceWork; /* size: 0x4D0 */

extern FaceWork face_work;

/* The unit's own `.sdata` pool word, declared (never defined) so the object emits only the reference (playbook 29).  The face-load tables
 * (`{size, name}` runs at 0x80582D38/0x80582E58/0x80582EE0, read by the unwritten `fn_80047634`) are `draw_shape.cpp`'s `.data` in the reconciled candidate. */
extern u16* lbl_80790EE8[];

/* --- the unit's own functions, declared so each has one signature --------------------------------- */

extern "C" void* fn_80047470(u8 index, u32 which);
extern "C" void fn_800474A8(void);
extern "C" void fn_80047618(u32 a, u32 b, u32 c, u32* slot);
extern "C" void fn_80047634(u8 index, u32 faceId, u8 mode);
extern "C" void fn_80047780(u8 index, u32 which, u8 mode);
extern "C" void fn_80047840(u8 index, u8 value);
extern "C" void fn_80047884(u8 index, u8 value);
extern "C" void fn_800478F0(u8 index);
extern "C" void fn_80047AF0(u8 index);
extern "C" void fn_80047B0C(u8 index);
extern "C" u32 fn_80047B28(u8 index);
extern "C" u32 fn_80047B40(u8 index);
extern "C" void fn_80047B58(u8 index);
extern "C" void fn_80047BE0(u8 index);
extern "C" void fn_80047C68(u8 index, u8 value);
extern "C" void fn_80047C88(u8 index, u8 a, u8 b);
extern "C" void fn_80047CAC(u8 index, u8 a, u8 b, u8 c, u8 d);
extern "C" u32 fn_80047D38(u8 index);

/* --- bodies, in address order --------------------------------------------------------------------- */

/* The two texture-base words for one player (0x460 / 0x488 runs). */
extern "C" void* fn_80047470(u8 index, u32 which)
{
    if (which == 0) {
        return face_work.ptr460[index];
    }
    return face_work.ptr488[index];
}

/* Clears the work block and hands out the texture base addresses (0x80C0 / 0x32460 / 0x20000 apart). */
extern "C" void fn_800474A8(void)
{
    u32 addr = 0x90CC9000;
    u32 i;
    u32 j;

    memset(&face_work, 0, 0x4D0);
    if (face_work.tex[0] == 0) {
        face_work.tex[0] = (void*)addr;
    }
    addr += 0x80C0;
    if (face_work.tex[1] == 0) {
        face_work.tex[1] = (void*)addr;
    }
    addr += 0x80C0;
    face_work.tex[2] = (void*)addr;
    addr += 0x32460;

    for (i = 0; i < 2; i++) {
        void** p1 = &face_work.ptr460[i * 5];
        void** p2 = &face_work.ptr488[i * 5];
        for (j = 0; j < 5; j++) {
            if (p1[j] == 0) {
                p1[j] = (void*)addr;
            }
            if (p2[j] == 0) {
                u32 high = addr + 0x10000;
                p2[j] = (void*)high;
            }
            addr += 0x20000;
        }
    }
}

/* The load_file completion callback: marks the blob whose slot word the caller passed as loaded. */
extern "C" void fn_80047618(u32 a, u32 b, u32 c, u32* slot)
{
    face_work.state[*slot] = 2;
}

/* Sets one player's face index and re-enables the draw. */
extern "C" void fn_80047840(u8 index, u8 value)
{
    if (value != 0xFF) {
        fn_80047C68(index, value);
        fn_80047AF0(index);
    }
}

/* Loads one player's face textures (from the male/female table picked by `index`) and rebinds them. */
extern "C" void fn_80047884(u8 index, u8 value)
{
    if (index != 0xFF) {
        fn_80047634(index, lbl_80790EE8[index][value], 0);
        fn_80047780(index, 0, 0);
        fn_800478F0(index);
    }
}

/* Enables one player's face draw. */
extern "C" void fn_80047AF0(u8 index)
{
    face_work.enabled[index] = 1;
}

/* Disables one player's face draw. */
extern "C" void fn_80047B0C(u8 index)
{
    face_work.enabled[index] = 0;
}

/* The frame counter of one player's face animation. */
extern "C" u32 fn_80047B28(u8 index)
{
    return face_work.frame[index];
}

/* The face index of one player. */
extern "C" u32 fn_80047B40(u8 index)
{
    return face_work.face[index];
}

/* Advances one player's face animation by a frame; at 100 it steps the face index (0 -> 1 -> 2). */
extern "C" void fn_80047B58(u8 index)
{
    if (face_work.frame[index] < 100) {
        face_work.frame[index]++;
        if (face_work.frame[index] == 100) {
            if (face_work.face[index] < 2) {
                face_work.face[index]++;
                face_work.frame[index] = 0;
            }
        }
        face_work.dirty[index] = 1;
    } else {
        if (face_work.face[index] < 2) {
            face_work.face[index]++;
            face_work.frame[index] = 0;
            face_work.dirty[index] = 1;
        }
    }
}

/* Steps one player's face animation back by a frame; at 0 it drops the face index. */
extern "C" void fn_80047BE0(u8 index)
{
    if (face_work.frame[index] != 0) {
        face_work.frame[index]--;
        if (face_work.frame[index] == 0) {
            if (face_work.face[index] != 0) {
                face_work.face[index]--;
                face_work.frame[index] = 100;
            }
        }
        face_work.dirty[index] = 1;
    } else {
        if (face_work.face[index] != 0) {
            face_work.face[index]--;
            face_work.frame[index] = 100;
            face_work.dirty[index] = 1;
        }
    }
}

/* Sets one player's first colour byte and flags the draw. */
extern "C" void fn_80047C68(u8 index, u8 value)
{
    face_work.colorA[index] = value;
    face_work.dirty[index] = 1;
}

/* Sets one player's two colour bytes and flags the draw. */
extern "C" void fn_80047C88(u8 index, u8 a, u8 b)
{
    face_work.colorB[index] = a;
    face_work.mouth[index] = b;
    face_work.dirty[index] = 1;
}

/* Loads one player's face set and seeds its animation state (runs the real loads, mode 1). */
extern "C" void fn_80047CAC(u8 index, u8 a, u8 b, u8 c, u8 d)
{
    if (d != 0xFF) {
        fn_80047634(a, b, 1);
        fn_80047780(a, 0, 1);
        face_work.face[index] = c;
        face_work.frame[index] = d;
        face_work.enabled[index] = 1;
        face_work.dirty[index] = 1;
    }
}

/* Ready when the draw is still disabled, otherwise whether the state is clean. */
extern "C" u32 fn_80047D38(u8 index)
{
    if (face_work.enabled[index] == 0) {
        return 1;
    }
    return face_work.dirty[index] == 0;
}

/* --- the GX pipe writers --------------------------------------------------------------------------- */

/* Empty stub. */
extern "C" void fn_80048764(void)
{
}

/* Two float vertices. */
extern "C" void fn_80048768(f32 x, f32 y)
{
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* A colour quad (four bytes). */
extern "C" void fn_80048778(u32 r, u32 g, u32 b, u32 a)
{
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
    GXWGFifo.u8 = a;
}

/* A 2D position pair. */
extern "C" void fn_800487A0(s32 x, s32 y)
{
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* The unit's fixed texture-coordinate generator (id 0, GX_TG_MTX2x4/GX_TG_TEX0, GX_PTIDENTITY). */
extern "C" void fn_800487B8(u32 a, u32 b, u32 c, u32 d)
{
    GXSetTexCoordGen2(a, b, c, d, 0, 0x7D);
}

/* Empty stub. */
extern "C" void fn_800487C4(void)
{
}

/* The unit's fixed display-list id. */
extern "C" u32 fn_800487C8(void)
{
    return 0x4880;
}

/* This unit's own `.bss` (`splits.txt` `.bss 0x806694E8..0x806699B8`) and `.sbss` (`0x80794870..0x80794878`), in
 * address order.  Defined at the foot of the file, after every use. */
FaceWork face_work;                    /* +0x806694E8 */
u8* face_buf_tbl[2];                   /* +0x80794870: the two 0x4880-byte buffers `fn_800487D0` carves out of the work heap (GUESS name) */
