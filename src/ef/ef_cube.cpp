#include "mh3_pad/vec3.h" /* the owner header (rule 2) */
/* auto/800C9DD0_fn_800C9DD0.c - the ef_cube.cpp translation unit, .text 0x800C9DD0..0x800CB948.
 *
 * fn_800C9DD0 (0x800C9DD0, 1072 B) is complete and matches except for one register choice (99.94 %).
 * fn_800CA200 (0x800CA200, 5960 B) has only its three pointer asserts so far (12.27 %) - the body is
 * still open, and what is known about it is below and in the outbox.
 *
 * fn_800CA200 - what is established (measurement: `recompile.py --measure fn_800CA200`):
 *   * signature, from the prologue and its seven fn_800C9DD0 call sites:
 *     `(u32 a [r3], Em* em [r4], Pm* pm [r5], u32 n [r6], u32 flags [r7],
 *      Params* params [r8], u16 d [r9], u32 e [r10], f32 f [f1])`
 *     The three asserts are lines 94/95/96 with the "em"/"pm"/"params" messages.
 *   * a `Vec3` array of 14 elements at r1+8 (offsets 8,20,...,164, stride 0xC); every block below
 *     works on one *pair* of it, in descending order: (152,164), (128,140), (104,116), (80,92).
 *   * six near-identical blocks, each: `VEC3_ctor(&pair[0])`, `VEC3_ctor(&pair[1])`, fill both,
 *     scale one component by `fn_800A8A08(&em->field_0xEC)`, take `fn_80463F04` (fabsf) of a component
 *     and compare it against `lbl_80796268`, then `fn_80463F98` + `fn_800610AC` + `fn_80050BC0`
 *     (nw4r::math::FrSqrt) and finish with `fn_800C9DD0(a, &pair[1], &pair[0], em, pm, d, f, e)`.
 *   * `fn_800C9DCC` is a 4-byte `b fn_80463F04` thunk and `fn_80463F04` is `fabsf`; the body calls it
 *     69 times, so the source's absolute-value calls are what the 69 `bl`s are.
 *   * the two big loops are over `n*n` (r23 vs r24 = n*n) and over `n` (r19), and the inner one
 *     carries a 4-state `switch` (r20) that walks a ring (r21/r22 = the two half-extents).
 *   * `nw4r::db::Panic` line numbers 94/95/96 here; `#pragma peephole off
#pragma fp_contract off` is required (see below).
 *   A structural draft from `tools/m2c` (with `goto` and `M2C_ERROR` placeholders, so it needs a
 *   rewrite into real loops and named locals before it is usable) is the fastest way back in:
 *     `python tools/units/m2cinput.py build/RMHE08/obj/auto/800C9DD0_fn_800C9DD0.o -o build/tmp/t.o`
 *     `python tools/m2c/m2c.py -t ppc-mwcc-c --no-cache -f fn_800CA200 build/tmp/t.o`
 *   The remaining work is source shape, not flags: the peephole pragma below and the lib's cflags
 *   already reproduce the retail idioms this function uses (`psq_lx` FPR restore, `clrlwi` on the u16
 *   argument, `fn_80463F04`/`fn_80463F98`/`fn_800610AC` calls).
 *
 * The original source file is named: the unit's own `.data` pool starts with the `__FILE__` string
 * "ef_cube.cpp" (`lbl_80594C68`, confirmed by the shared dump's symbol map as `s_ef_cube.cpp_80594c68`),
 * which is the file argument of every `nw4r::db::Panic` call below. The unit name is still the
 * provisional `auto/` one; renaming it is a configure.py + splits.txt batch, requested in the outbox.
 *
 * Flag evidence - `#pragma peephole off
#pragma fp_contract off` below is load-bearing, not cosmetic:
 *   * the FPR epilogue is `li r0,136; psq_lx f31,r1,r0,0,0` in retail. With the peephole on MWCC
 *     rewrites that pair to the displacement form `psq_l f31,136(r1),0,0` (verified on Wii/1.0, 1.0a,
 *     1.1 and 1.3 - all four emit `psq_l`), and the peephole also deletes the `clrlwi r4,r30,16` the
 *     retail call site keeps. Off, fn_800C9DD0 scores 99.94 %; on, 94.24 %.
 *   * `-opt nopeephole` on the command line reproduces the same object, so this is a peephole
 *     difference and not a compiler version or a `-O` level one.
 *
 * Flag evidence - `#pragma fp_contract off` below, same shape:
 *   * retail keeps every `a*b+c` as two instructions here (`fmuls f1,f0,f1; fsubs f0,f1,f0` and
 *     `fmuls f0,f0,f2; fmuls f1,f0,f1; fadds f30,f0,f1`), which `-fp_contract on` fuses into
 *     `fmsubs`/`fmadds`. Writing each site through a temporary reproduces retail's pairs too, so the
 *     pragma is not strictly required for this unit - it is the honest form of the same finding, and
 *     the neighbouring ef unit (auto/800CB948_fn_800CB948, ef_cylinder.cpp) measured the identical
 *     thing on four sites there. With the pragma fn_800C9DD0 measures 99.94 %, exactly as with the
 *     temporaries; without either, 94.24 %.
 *
 * Residual (fn_800C9DD0, 99.94 %): the last `lwz` pair is `lwz r12,28(r3); lwz r12,20(r12)` in retail
 * and `lwz r11,28(r29); lwz r12,20(r11)` here - the allocator keeps `pm` in its incoming r29 where
 * retail colours it into r3 (the copy it just made for the call's `this`). Declaration order, a named
 * `Pm*`/`PmVtbl*` local, a cast and `-lang=c++` were all tried and none moves it.
 *
 * The float pool (`lbl_80796240..lbl_80796260`) and the three message strings are referenced as
 * externals because the split does not own them yet: the target object has no `.sdata2`/`.data`, so
 * emitting them here would add sections the target does not have. Claiming them is the measured data
 * pass (`tools/units/dataclaim.py`), which is why the magic-double reloc here reads `@118` where the
 * target reads `lbl_80796258`.
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (source): source file change ef_cube.cpp -> ef_cylinder.cpp
 *   pinned seam (pool): .sdata2 run jump lbl_8079626C -> lbl_80796270
 *
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x80023BB0..0x80023BC8   2 labels  proposed    (dataclaim: no queue run)
 *   .data        0x80594C68..0x80594D11   4 labels  proposed    (dataclaim: unowned)
 *   .sdata2      0x80796240..0x80796270  10 labels  proposed    (dataclaim: unowned)
 *
 *   The `extab`/`extabindex` fragment sections are the exception: they travel with the code unit
 *   (`dataqueue.py`'s `FRAGMENT_SECTIONS`) and are claimed in `splits.txt` with its `.text`;
 *   `.ctors`/`.dtors` are added after the split. Everything else above waits for the measured pass.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/800C9DD0_fn_800C9DD0.c`.
 */

/* The unit keeps its own scalar typedefs: `types.h` spells `u32` `unsigned long` while this unit's
 * manglings encode `unsigned int` (`fn_800C9DD0__FUiP4Vec3...` in the target object), so including
 * `types.h` here re-mangles both functions and unpairs them.  `mh3_pad/vec3.h` is the owner's header
 * for the three vector helpers and deliberately pulls in no typedefs. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef float f32;
typedef double f64;

/* nw4r::math::VEC3, the type fn_80051490/fn_80050EDC and this unit's three vector locals use. */
typedef struct {
    f32 x; /* +0x0 */
    f32 y; /* +0x4 */
    f32 z; /* +0x8 */
} Vec3; /* size: 0xC */

/* The unit's own .data pool (unclaimed - see the header): the source file name and one message per
 * checked pointer. `Panic` is `nw4r::db::Panic(const char*, int, const char*, ...)`. */
extern char lbl_80594C68[]; /* "ef_cube.cpp" */
extern char lbl_80594C74[]; /* "NW4R:Pointer Error\nem(=%p) is not valid pointer." */
extern char lbl_80594CA8[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer." */
extern char lbl_80594CDC[]; /* "NW4R:Pointer Error\nparams(=%p) is not valid pointer." */

/* The unit's own .sdata2 pool (unclaimed - see the header). */
extern f32 lbl_80796240; /* 0x00800000, FLT_MIN */
extern f32 lbl_80796244; /* 2.0f */
extern f32 lbl_80796248; /* 1.0f */
extern f32 lbl_8079624C; /* 0.0f */
extern f32 lbl_80796250; /* 0.01f */
extern f64 lbl_80796258; /* 0x4330000080000000, the u32 -> f64 magic */

/* nw4r::db::Panic. The map already carries its real C++ mangling
 * (Panic__Q24nw4r2dbFPCciPCce), and declaring that spelling as a C++ identifier re-mangles it
 * (Panic__Q24nw4r2dbFPCciPCce__FPCciPCce) - which only shows up at LINK time, so a NonMatching
 * unit hides it until it is flipped. Declare the real thing and the front-end reproduces the
 * map's spelling exactly: tools/units/mangle.py confirms it. */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }

/* nw4r::math and effect-library helpers, still `fn_*` in the symbol map.  The target object
 * references each by its plain map name, so they carry C linkage; a C++ spelling mangles the reloc
 * (fn_8009C484__FP4Vec3P4Vec3) and it no longer pairs (relocaudit). */
extern "C" {
extern void fn_8009C484(Vec3* dst, Vec3* src);
extern void fn_80051490(Vec3* dst, const Vec3* src);
extern f32 fn_80050EDC(const Vec3* v);
extern f32 fn_800A8A08(const void* p);
extern u16 fn_800A9FB0(u32 a, u16 b, f32 f, void* em);
extern void fn_800A99B4(u32 a, Vec3* b, void* em, Vec3* c, Vec3* d, Vec3* e, Vec3* f);
}

/* The "em" object this unit is handed: an effect manager. Only the fields the two functions touch are
 * named, and they are named by offset because nothing in the dump names them. */
typedef struct Em Em;
struct Em {
    u8 pad_0x00[0x67]; /* +0x00 */
    s8 field_0x67;     /* +0x67 */
    u8 pad_0x68[0x80]; /* +0x68 */
    u16 field_0xE8;    /* +0xE8 */
    u8 pad_0xEA[0x02]; /* +0xEA */
    u32 field_0xEC;    /* +0xEC */
    u8 pad_0xF0[0x08]; /* +0xF0 */
    u32 field_0xF8;    /* +0xF8 */
    u8 field_0xFC[4];  /* +0xFC */
}; /* size: 0x100 (approx: only the accessed offsets are evidenced) */

/* The "pm" object: its vtable pointer sits at +0x1C and slot 5 is the spawn call this unit makes. */
typedef struct Pm Pm;
typedef struct PmVtbl PmVtbl;
struct PmVtbl {
    u8 pad_0x00[0x14];                                                  /* +0x00 */
    void (*spawn_0x14)(Pm* self, u16 a, Vec3* b, Vec3* c, u32 d, f32 e, /* +0x14 */
                       u8* f, u32 g, u16 h);
}; /* size: 0x18 */
struct Pm {
    u8 pad_0x00[0x1C]; /* +0x00 */
    PmVtbl* vtbl;      /* +0x1C */
}; /* size: 0x20 */

#pragma peephole off
#pragma fp_contract off

/* The engine's pointer validity test: the address has to fall in one of the memory regions the game
 * allocates from. Retail materialises it as one `||` chain (`li`/`li ...,0` per region), so it must
 * stay a single expression assigned to a variable - an inline `if (!...)` compiles to branches. */
#define IS_VALID_PTR(p)                         \
    ((((u32)(p) & 0xFF000000) == 0x80000000) || \
     (((u32)(p) & 0xFF800000) == 0x81000000) || \
     (((u32)(p) & 0xF8000000) == 0x90000000) || \
     (((u32)(p) & 0xFF000000) == 0xC0000000) || \
     (((u32)(p) & 0xFF800000) == 0xC1000000) || \
     (((u32)(p) & 0xF8000000) == 0xD0000000) || \
     (((u32)(p) & 0xFFFFC000) == 0xE0000000))

/* Normalises the effect's two direction vectors, derives a spawn position from them, and hands the
 * result to the effect parameter object's spawn method. */
void fn_800C9DD0(u32 a, Vec3* b, Vec3* c, Em* em, Pm* pm, u16 d, f32 f, u32 e)
{
    Vec3 v1;
    Vec3 v2;
    Vec3 v3;
    Vec3 b_copy;
    Vec3 v3_copy;
    f32 s;
    u16 r;
    int ok;

    ok = IS_VALID_PTR(em);
    if (!ok) {
        nw4r::db::Panic(lbl_80594C68, 43, lbl_80594C74, em);
    }
    ok = IS_VALID_PTR(pm);
    if (!ok) {
        nw4r::db::Panic(lbl_80594C68, 44, lbl_80594CA8, pm);
    }

    fn_8009C484(c, c);
    fn_80051490(&v1, b);
    if (fn_80050EDC(&v1) <= lbl_80796240) {
        v1.x = fn_800A8A08(&em->field_0xEC) * lbl_80796244 - lbl_80796248;
        v1.y = fn_800A8A08(&em->field_0xEC) * lbl_80796244 - lbl_80796248;
        v1.z = fn_800A8A08(&em->field_0xEC) * lbl_80796244 - lbl_80796248;
    }
    fn_8009C484(&v1, &v1);
    fn_80051490(&v2, b);
    v2.y = lbl_8079624C;
    if (fn_80050EDC(&v2) <= lbl_80796240) {
        v2.x = fn_800A8A08(&em->field_0xEC) * lbl_80796244 - lbl_80796248;
        v2.z = fn_800A8A08(&em->field_0xEC) * lbl_80796244 - lbl_80796248;
    }
    fn_8009C484(&v2, &v2);
    VEC3_ctor(&v3);
    fn_800A99B4(a, &v3, em, b, c, &v1, &v2);
    v3_copy = v3;
    b_copy = *b;
    s = lbl_80796248 + lbl_80796250 * (f32)em->field_0x67 * fn_800A8A08(&em->field_0xEC);
    r = fn_800A9FB0(a, d, f, em);
    pm->vtbl->spawn_0x14(pm, r, &b_copy, &v3_copy, e, s, em->field_0xFC, em->field_0xF8,
                         em->field_0xE8);
}

/* Builds the effect cube's vertex grid and emits it through fn_800C9DD0 once per face.
 *
 * Only the three pointer asserts are recovered: they are the same `||` chain fn_800C9DD0 uses, at
 * lines 94/95/96, and they alone measure 12.27 %. The rest of the body is not written yet - see the
 * header for the structure that is established and for the m2c draft to start from. */
void fn_800CA200(u32 a, Em* em, Pm* pm, u32 n, u32 flags, void* params, u16 d, u32 e, f32 f)
{
    int ok;

    ok = IS_VALID_PTR(em);
    if (!ok) {
        nw4r::db::Panic(lbl_80594C68, 94, lbl_80594C74, em);
    }
    ok = IS_VALID_PTR(pm);
    if (!ok) {
        nw4r::db::Panic(lbl_80594C68, 95, lbl_80594CA8, pm);
    }
    ok = IS_VALID_PTR(params);
    if (!ok) {
        nw4r::db::Panic(lbl_80594C68, 96, lbl_80594CDC, params);
    }
}
