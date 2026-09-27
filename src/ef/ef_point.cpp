/*
 * ef_point.cpp - the point-emitter spawn routine of the "ef" (effect) library.
 *
 * The file is named by the `__FILE__` string its own asserts reference: `.data:lbl_80594F98` reads
 * "ef_point.cpp", and the three sibling strings `lbl_80594FA8`/`lbl_80594FDC`/`lbl_80595010` are the
 * "NW4R:Pointer Error\n{em,pm,params}(=%p) is not valid pointer." messages. The unit holds exactly one
 * function, `.text 0x800CD584..0x800CDB2C` (`fn_800CD584`); the surrounding seams are ef_line.cpp ->
 * ef_point.cpp below it and ef_point.cpp -> ef_sphere.cpp above it.
 *
 * What the function does: validates its emitter (`em`), parameter-manager (`pm`) and parameter-block
 * (`params`) pointers, then spawns `count` points. Each point takes a normalised rate from `em`'s
 * `+0xEC` sub-object (`fn_800A8A08`), maps it through `0.66 +/- 0.34 * t` onto a circle, builds a
 * position/rotation pair (`fn_800A99B4`), and hands it to `pm`'s virtual method at vtable `+0x14`
 * together with the id `fn_800A9FB0` resolves.
 *
 * Load-bearing source shapes (each one is what the target's bytes require):
 *   - `#pragma fp_contract off` (scoped to this file): the target has **no** fused float op anywhere
 *     (`fmuls`/`fadds`/`fsubs` only), while `-fp_contract on` fuses `2.0f * rate - 1.0f`,
 *     `0.66f + 0.34f * t` and `1.0f - s * s` into `fmsubs`/`fmadds`/`fnmsubs`. Per-library flag
 *     evidence lives in `configure.py`; this is a one-unit deviation (docs/plan.md 8.2).
 *   - the pool constants are **declared, never defined** (`lbl_80796300..lbl_80796320`, playbook 29):
 *     writing `0.0f`/`2.0f`/... literals makes MWCC pool them under its own names and the `lfs`
 *     relocations stop pairing with the target's.
 *   - the pointer asserts are the `if (!okN && !test) okN-1 = FALSE;` chain (six materialised BOOLs,
 *     the first `if` carrying two tests) - that exact shape is what the target emits; a plain
 *     `||`-chain short-circuits and a single accumulator produces one `li` instead of six.
 *   - the masked top bits are precomputed *after* the six BOOL declarations (`u32 top_ = (u32)ptr &
 *     0xFF000000`). Declaring it before them, or inlining the mask in each test, colours the mask web
 *     r5 and shifts all six BOOLs up one register; the target's r11 + r5..r10 only appears with the
 *     declaration after the BOOLs.
 *   - the three assert call sites are at lines 42/43/44 of the original file; `#line` reproduces the
 *     `li r4,{42,43,44}` immediates.
 *
 * Residual - `fn_800CD584` measures 93.69 % (`build/RMHE08/report.json`; ours 0x580 = 1408 B vs the
 * target's 0x5A8 = 1448 B, so ten instructions are still missing):
 *   - the whole size gap is the FPR-restore idiom: the target does `li r0,<off>; psq_lx fN,r1,r0,0,0;
 *     lfd` per saved register, while every Wii compiler here emits `psq_l fN,<off>(r1); lfd`
 *     (`-use_lmw_stmw on|off`, `-schedule off`, `-O4,p` and Wii 1.0/1.1/1.5/1.6/1.7 all emit the
 *     latter). Not source-reachable.
 *   - FPR colouring: the target keeps pi/2^52/0.01 in f22/f23/f24 and the sqrt result in f25; this
 *     build gives f22 to the sqrt result and f23..f25 to the constants (8 instructions).
 *   - `(...) * t` is emitted `fmuls f0,f1,f0` where the target has `fmuls f0,f0,f1` (2 instructions);
 *     the literal-constant variant gets the order right but drops the `lbl_8079630*` pool
 *     relocations, which measures 93.74 % - objdiff's partial credit for nine wrong relocations, so
 *     the extern form stays.
 *   - the emitter's fields keep `field_0xNN` names: their roles are not evidenced beyond their use in
 *     this function (opaque arguments to the spawn call), so no context name was invented.
 */

#include "types.h"
#include "nw4r/math.h" /* nw4r::math::VEC3 - the vector record these bodies work on (rule 11) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* --------------------------------------------------------------------------------------------- */
/* Types                                                                                          */
/* --------------------------------------------------------------------------------------------- */

/* The three-float vector the ef emitter calls pass around: the shared record the three vector
 * helpers take, reached here as the local spelling `EfVec3` (the name the ef band's own code
 * uses). */
typedef nw4r::math::VEC3 EfVec3; /* size: 0x0C */

/* The emitter sub-object at `em + 0xEC` whose normalised progress `fn_800A8A08` returns. Only its
 * address is used here; `fn_800A8A08` reads the u32 at +0x00. */
typedef struct EfRate {
    /* +0x00 */ u32 counter;
    /* +0x04 */ u8 pad_0x04[0x08];
} EfRate; /* size: 0x0C */

/* The emitter the point is spawned from. Only the fields `fn_800CD584` touches are evidenced, so the
 * size is a lower bound. */
typedef struct EfEmitter {
    /* +0x00 */ u8 pad_0x00[0x67];
    /* +0x67 */ s8 field_0x67;             /* scaled by 0.01 and folded into the point's scale */
    /* +0x68 */ u8 pad_0x68[0xE8 - 0x68];
    /* +0xE8 */ u16 field_0xE8;            /* passed to the parameter manager */
    /* +0xEA */ u8 pad_0xEA[0xEC - 0xEA];
    /* +0xEC */ EfRate rate;
    /* +0xF8 */ s32 field_0xF8;            /* passed to the parameter manager */
    /* +0xFC */ u8 field_0xFC;             /* address passed to the parameter manager */
} EfEmitter; /* size: 0x100 (lower bound) */

/* The parameter manager's spawn method, reached through `pm->iface->fn_0x14`. */
typedef void (*EfPmSpawn)(void *self, u16 id, EfVec3 *a, EfVec3 *b, s32 arg7, u8 *p0, s32 p1,
                          u16 p2, f32 scale);

typedef struct EfPmIface {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ EfPmSpawn fn_0x14;
} EfPmIface; /* size: 0x18 (lower bound) */

typedef struct EfPm {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ EfPmIface *iface;
} EfPm; /* size: 0x20 (lower bound) */

/* --------------------------------------------------------------------------------------------- */
/* Referenced symbols. The pool labels and the assert strings belong to ranges this unit does not
 * own (docs/plan.md 8.4), so they stay undefined externs here. */
/* --------------------------------------------------------------------------------------------- */

/* nw4r::db::Panic. The map already carries its real C++ mangling
 * (Panic__Q24nw4r2dbFPCciPCce), and declaring that spelling as a C++ identifier re-mangles it
 * (Panic__Q24nw4r2dbFPCciPCce__FPCciPCce) - which only shows up at LINK time, so a NonMatching
 * unit hides it until it is flipped. Declare the real thing and the front-end reproduces the
 * map's spelling exactly: tools/units/mangle.py confirms it. */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }
extern const char lbl_80594F98[]; /* "ef_point.cpp" */
extern const char lbl_80594FA8[]; /* "NW4R:Pointer Error\nem(=%p) is not valid pointer." */
extern const char lbl_80594FDC[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer." */
extern const char lbl_80595010[]; /* "NW4R:Pointer Error\nparams(=%p) is not valid pointer." */

extern const f32 lbl_80796300; /* 0.0f  */
extern const f32 lbl_80796304; /* 2.0f  */
extern const f32 lbl_80796308; /* 1.0f  */
extern const f32 lbl_8079630C; /* 0.66f */
extern const f32 lbl_80796310; /* 0.34f */
extern const f32 lbl_80796314; /* 3.14159274f */
extern const f32 lbl_80796318; /* 0.01f */

/* ef/nw4r math helpers.  The target object references each by its plain `fn_XXXXXXXX` map name,
 * so they carry C linkage; a C++ spelling mangles the reloc (fn_80041E8C__FP6EfVec3fff) and it no
 * longer pairs (relocaudit). */
extern "C" {
extern f32 fn_800A8A08(struct EfRate *rate);
extern u16 fn_800A9FB0(void *self, u16 id, struct EfEmitter *em, f32 f);
extern void fn_800A99B4(void *self, nw4r::math::VEC3 *out, struct EfEmitter *em, nw4r::math::VEC3 *a,
                        nw4r::math::VEC3 *b, nw4r::math::VEC3 *c, nw4r::math::VEC3 *d);
extern f32 fn_80050BC0(f32 x);
extern void fn_8009C760(f32 *a, f32 *b, f32 angle);
extern void fn_8009C484(nw4r::math::VEC3 *a, nw4r::math::VEC3 *b);
}

/* --------------------------------------------------------------------------------------------- */
/* Assert                                                                                         */
/* --------------------------------------------------------------------------------------------- */

/* The library's pointer assert. `addr` must fall in one of the seven mapped memory ranges; the six
 * materialised BOOLs and the two-test first `if` are the target's exact shape. */
#define NW4R_POINTER_ASSERT(ptr, msg)                                                        \
    {                                                                                        \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;   \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                 \
        if (!(top_ == 0x80000000u) &&                                                        \
            !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))                                    \
            ok6_ = FALSE;                                                                    \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                           \
            ok5_ = FALSE;                                                                    \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                 \
            ok4_ = FALSE;                                                                    \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                           \
            ok3_ = FALSE;                                                                    \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                           \
            ok2_ = FALSE;                                                                    \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                           \
            ok1_ = FALSE;                                                                    \
        if (!ok1_)                                                                           \
            nw4r::db::Panic(lbl_80594F98, __LINE__, msg, (ptr));                   \
    }

/* --------------------------------------------------------------------------------------------- */

#pragma fp_contract off

void fn_800CD584(void *self, EfEmitter *em, EfPm *pm, s32 count, void *unused, void *params,
                 s32 id, s32 arg7, f32 f) {
    EfVec3 v_44, v_38, v_2C, v_20, v_a, v_b;
    s32 i;

#line 42
    NW4R_POINTER_ASSERT(em, lbl_80594FA8);
    NW4R_POINTER_ASSERT(pm, lbl_80594FDC);
    NW4R_POINTER_ASSERT(params, lbl_80595010);

    if (count >= 1) {
        for (i = 0; i < count; i++) {
            f32 rate, t, s, r, scale;

            setVec3(&v_44, lbl_80796300, lbl_80796300, lbl_80796300);
            VEC3_ctor(&v_38);
            rate = fn_800A8A08(&em->rate);
            t = lbl_80796304 * rate - lbl_80796308;
            if (t >= lbl_80796300)
                s = (lbl_8079630C + lbl_80796310 * t) * t;
            else
                s = (lbl_8079630C - lbl_80796310 * t) * t;
            v_38.x = s;
            r = fn_80050BC0(lbl_80796308 - v_38.x * v_38.x);
            fn_8009C760(&v_38.z, &v_38.y,
                        lbl_80796304 * (lbl_80796314 * fn_800A8A08(&em->rate)));
            v_38.y = v_38.y * r;
            v_38.z = v_38.z * r;
            setVec3(&v_2C, v_38.x, lbl_80796300, v_38.z);
            fn_8009C484(&v_2C, &v_2C);
            VEC3_ctor(&v_20);
            fn_800A99B4(self, &v_20, em, &v_44, &v_38, &v_38, &v_2C);
            v_b = v_20;
            v_a = v_44;
            scale = lbl_80796308 +
                    (lbl_80796318 * (f32)em->field_0x67) * fn_800A8A08(&em->rate);
            pm->iface->fn_0x14(pm, fn_800A9FB0(self, (u16)id, em, f), &v_a, &v_b, arg7,
                               &em->field_0xFC, em->field_0xF8, em->field_0xE8, scale);
        }
    }
}
