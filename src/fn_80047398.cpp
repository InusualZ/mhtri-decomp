/*
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` - every function in 0x80047398..0x8004C9A0 is a bare
 * `.text` entry; the shared runtime dump has only `zz_<addr>_` placeholders, so there is no better name
 * to take).  The four mangled names the range does carry (`drawSpr2TF__FUcP9fltSpr2TFUc`,
 * `subTransSetPrio__FUcUllPUl`, `subTransSet__FUllPUl`, `subTransSetStackSetup`) are spelled by their
 * real declarations, never as callable identifiers (rule 9).
 *
 * The character face/skin render unit.  `.text` 0x80047398..0x8004C9A0 (0x5610 B, 117 functions),
 * `extab` 0x80006A90..0x80006CF0, `extabindex` 0x8001E948..0x8001ECD8 (75 framed functions each).
 * The range is one maximal unclaimed run (attribute.py): its seam is unproven, and the two halves of the
 * body work on different data (the loader/driver family around `lbl_806694E8`, and the sprite/transform
 * family from 0x80048E2C on) - see "Residual" below.
 *
 * Naming - which evidence class decided it.
 *   * Class 1 (`__FILE__` string) FAILS: the unit's `.data` pool (0x805813E8, 0x80582D38, 0x80582E58,
 *     0x80582EE0) and its `.sdata2` run (0x80795B70..0x80795BAC) hold resource paths
 *     (`08/skin/m_face000.tpl` .. `08/skin/f_face008.tpl`, `08/skin/facemake.tpl`, `11/efm/com/*.bin`,
 *     `15/scr_me*.breff`) and geometry floats, but NO bare source-file name, and no function in the range
 *     calls `OSPanic`/`Panic` (so no assert string exists).
 *   * Class 2 (runtime-dump name) FAILS: `dumpmap.py lookup 0x80047398` answers `zz_0047398_` only, and
 *     the `join` pass proposes `DBClose`/`GXPosition3f32` style names that are address coincidences from a
 *     different build's dump - not evidence.
 *   * Class 3 decides it: the file keeps the map's stem `fn_80047398` (the same outcome the sibling
 *     `src/fn_80040598.cpp` records) and is registered in the game-root band - the `main` lib at the
 *     `src/` root, with `cflags_main`, exactly where its link neighbours sit (`main.cpp`, `sys_mem.cpp`,
 *     `fn_80040598.cpp`, `mh3_pad.cpp` 0x800408A8..0x80047398 - which ends where this unit begins - and
 *     `nw_resource.cpp`).  Module `main` is a *recorded decision*, not an invention; a future pass that
 *     finds a `__FILE__` string or a subsystem seam can re-home it.
 *
 * What the unit does (from the bodies): it loads the male/female face, skin and `facemake` TPL textures
 * through the 8-byte `{size, name}` tables at 0x80582D38/0x80582E58/0x80582EE0, binds them as GX texture
 * objects into a work block at `lbl_806694E8` (0x4D0 B, cleared by fn_800474A8, whose three `void*`
 * slots start at 0x04), keeps a per-player face/frame/animation state there (the byte arrays at 0x448,
 * 0x456, 0x4B0, 0x4BA, 0x4C4 and the ten-entry pointer runs at 0x460/0x488) and draws the 2D face with an
 * orthographic GX pipeline (fn_80047D6C builds C_MTXOrtho/GXSetProjection and pushes TPL quads).
 *
 * Status: partial (phase B first pass).  28 of the 117 symbols have bodies, in address order; the rest
 * are unwritten (0 %), biggest first `fn_80047D6C` (0x9F8), `fn_8004991C` (0x8D0), `fn_80048964` (0x33C),
 * `fn_80049F7C` (0x2B0), `fn_8004A5AC` (0x2A8), `fn_800478F0` (0x200), `fn_80047634` (0x170).
 *
 * Residual: the four functions that read `Screen_w` (fn_80047398, fn_800473F4, fn_800478F0, fn_80047D6C)
 * are NOT written: `ScreenWork` is defined inside `main.cpp`, and reaching its fields from a second unit
 * needs that type moved to `include/` first (rule 1) - a cross-unit refactor, recorded here rather than
 * copied (rule 1 forbids a local copy).  fn_80047634/fn_80047780 are declared and called by fn_80047884/
 * fn_80047CAC but their load_file_req/load_file bodies are unwritten (their signatures come from the map's
 * `load_file_req__FPcUllUllPUl` / `load_file__FPcUll`, which rule 9 forbids spelling as identifiers, so
 * their real declarations have to be recovered from the callee first).
 *
 * Inventory / addresses / sizes: `python tools/units/ledger.py unit fn_80047398.cpp`.
 */

#include "types.h"
#include "id_value.h"
#include "gx.h"
#include "Runtime.PPCEABI.H/memset.h" /* owned by Runtime.PPCEABI.H/memset.c (rule 2) */

/* --- the unit's own work block, `lbl_806694E8` (.bss 0x806694E8, 0x4D0 B) ------------------------ */

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

extern FaceWork lbl_806694E8;

/* The unit's own `.data`/`.sdata` pool, declared (never defined) so the object emits only the references
 * (playbook 29).  The two face tables are runs of 8-byte `{size, name}` records. */
typedef struct FaceLoadEntry {
    /* +0x00 */ u32 size;
    /* +0x04 */ const char* name;
} FaceLoadEntry; /* size: 0x8 */

extern FaceLoadEntry lbl_80582D38[];
extern FaceLoadEntry lbl_80582E58[];
extern FaceLoadEntry lbl_80582EE0[];
extern u16* lbl_80790EE8[];

/* SDK function the pipe helper tail-calls (no map mangling, so the plain declaration is the real one). */
extern "C" void GXSetTexCoordGen2(u32, u32, u32, u32, u32, u32);

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
        return lbl_806694E8.ptr460[index];
    }
    return lbl_806694E8.ptr488[index];
}

/* Clears the work block and hands out the texture base addresses (0x80C0 / 0x32460 / 0x20000 apart). */
extern "C" void fn_800474A8(void)
{
    u32 addr = 0x90CC9000;
    u32 i;
    u32 j;

    memset(&lbl_806694E8, 0, 0x4D0);
    if (lbl_806694E8.tex[0] == 0) {
        lbl_806694E8.tex[0] = (void*)addr;
    }
    addr += 0x80C0;
    if (lbl_806694E8.tex[1] == 0) {
        lbl_806694E8.tex[1] = (void*)addr;
    }
    addr += 0x80C0;
    lbl_806694E8.tex[2] = (void*)addr;
    addr += 0x32460;

    for (i = 0; i < 2; i++) {
        void** p1 = &lbl_806694E8.ptr460[i * 5];
        void** p2 = &lbl_806694E8.ptr488[i * 5];
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
    lbl_806694E8.state[*slot] = 2;
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
    lbl_806694E8.enabled[index] = 1;
}

/* Disables one player's face draw. */
extern "C" void fn_80047B0C(u8 index)
{
    lbl_806694E8.enabled[index] = 0;
}

/* The frame counter of one player's face animation. */
extern "C" u32 fn_80047B28(u8 index)
{
    return lbl_806694E8.frame[index];
}

/* The face index of one player. */
extern "C" u32 fn_80047B40(u8 index)
{
    return lbl_806694E8.face[index];
}

/* Advances one player's face animation by a frame; at 100 it steps the face index (0 -> 1 -> 2). */
extern "C" void fn_80047B58(u8 index)
{
    if (lbl_806694E8.frame[index] < 100) {
        lbl_806694E8.frame[index]++;
        if (lbl_806694E8.frame[index] == 100) {
            if (lbl_806694E8.face[index] < 2) {
                lbl_806694E8.face[index]++;
                lbl_806694E8.frame[index] = 0;
            }
        }
        lbl_806694E8.dirty[index] = 1;
    } else {
        if (lbl_806694E8.face[index] < 2) {
            lbl_806694E8.face[index]++;
            lbl_806694E8.frame[index] = 0;
            lbl_806694E8.dirty[index] = 1;
        }
    }
}

/* Steps one player's face animation back by a frame; at 0 it drops the face index. */
extern "C" void fn_80047BE0(u8 index)
{
    if (lbl_806694E8.frame[index] != 0) {
        lbl_806694E8.frame[index]--;
        if (lbl_806694E8.frame[index] == 0) {
            if (lbl_806694E8.face[index] != 0) {
                lbl_806694E8.face[index]--;
                lbl_806694E8.frame[index] = 100;
            }
        }
        lbl_806694E8.dirty[index] = 1;
    } else {
        if (lbl_806694E8.face[index] != 0) {
            lbl_806694E8.face[index]--;
            lbl_806694E8.frame[index] = 100;
            lbl_806694E8.dirty[index] = 1;
        }
    }
}

/* Sets one player's first colour byte and flags the draw. */
extern "C" void fn_80047C68(u8 index, u8 value)
{
    lbl_806694E8.colorA[index] = value;
    lbl_806694E8.dirty[index] = 1;
}

/* Sets one player's two colour bytes and flags the draw. */
extern "C" void fn_80047C88(u8 index, u8 a, u8 b)
{
    lbl_806694E8.colorB[index] = a;
    lbl_806694E8.mouth[index] = b;
    lbl_806694E8.dirty[index] = 1;
}

/* Loads one player's face set and seeds its animation state (runs the real loads, mode 1). */
extern "C" void fn_80047CAC(u8 index, u8 a, u8 b, u8 c, u8 d)
{
    if (d != 0xFF) {
        fn_80047634(a, b, 1);
        fn_80047780(a, 0, 1);
        lbl_806694E8.face[index] = c;
        lbl_806694E8.frame[index] = d;
        lbl_806694E8.enabled[index] = 1;
        lbl_806694E8.dirty[index] = 1;
    }
}

/* Ready when the draw is still disabled, otherwise whether the state is clean. */
extern "C" u32 fn_80047D38(u8 index)
{
    if (lbl_806694E8.enabled[index] == 0) {
        return 1;
    }
    return lbl_806694E8.dirty[index] == 0;
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

/* Empty stub. */
extern "C" void fn_80048CA0(void)
{
}

/* Two float vertices (second writer family). */
extern "C" void fn_80048CA4(f32 x, f32 y)
{
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* One raw word. */
extern "C" void fn_80048CB4(u32 value)
{
    GXWGFifo.u32 = value;
}

/* A 2D position pair (second writer family). */
extern "C" void fn_80048CC0(s32 x, s32 y)
{
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* The unit's fixed texture-coordinate generator (second writer family). */
extern "C" void fn_80048CD8(u32 a, u32 b, u32 c, u32 d)
{
    GXSetTexCoordGen2(a, b, c, d, 0, 0x7D);
}

/* --- the small table helpers ----------------------------------------------------------------------- */

/* The record size for one menu kind. */
extern "C" u32 fn_8004AF0C(u32 kind)
{
    return kind == 1 ? 0x20 : 0x18;
}

/* The record block for one layout kind. */
extern "C" u8* fn_8004AF60(u8* base, u32 kind)
{
    if (kind == 1) {
        return base + 0x100;
    }
    return base + 0xA0;
}

/* Whether the given id belongs to the face-record family. */
extern "C" u32 fn_8004B034(u16 id)
{
    if ((u32)(id - 0x1B6) <= 1) {
        return 1;
    }
    if (id == 0) {
        return 1;
    }
    if (id == 0xDF) {
        return 1;
    }
    return 0;
}

/* Looks up an id in a 4-byte `{id, value}` table; 0 when it is absent. */
extern "C" s32 item_count_find(u16 id, const IdValue* table, s32 count)
{
    s32 value = 0;

    if (id != 0 && count > 0) {
        do {
            if (table->id == id) {
                value = table->value;
                break;
            }
            table++;
        } while (--count);
    }
    return value;
}

/* The index of an id in a 4-byte `{id, value}` table, or -1. */
extern "C" s32 item_pair_index_find(u16 id, const IdValue* table, s32 count)
{
    s32 index = 0;

    if (count > 0) {
        do {
            if (table->id == id) {
                return index;
            }
            table++;
            index++;
        } while (--count);
    }
    return -1;
}

/* Clears one record and reports whether it had been in use. */
extern "C" u32 fn_8004BD30(IdValue* entry)
{
    entry->value = 0;
    if (entry->id != 0) {
        entry->id = 0;
        return 1;
    }
    return 0;
}

/* Counts the free entries of a 4-byte `{id, value}` table. */
extern "C" u32 fn_8004C004(const IdValue* table, s32 count)
{
    u32 free = 0;

    if (count > 0) {
        do {
            if (table->id == 0) {
                free++;
            }
            table++;
        } while (--count);
    }
    return free;
}

/* Copies one 4-byte RGBA colour byte by byte. */
extern "C" void color_rgba_copy(u8* dst, const u8* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}
