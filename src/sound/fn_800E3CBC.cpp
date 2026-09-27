/*
 * sound/fn_800E3CBC.cpp - the 0x800E3CBC..0x800E46E8 band: the primitive-record pools and their GX state
 * helpers.  Registered from `proposal/800E3CBC_fn_800E3CBC.cpp` (12 functions, 2604 B).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>`: nine of the twelve addresses carry a `zz_` placeholder,
 * only prim_init_all__Fv, set_blendmode__FUcUcUc and set_zmode__FbUcb have a real dump name, and none of
 * the three is the translation unit's name).
 *
 * Module evidence.  The range has **no** `__FILE__` string: scanning every bare source-name string in
 * `orig/RMHE08/sys/main.dol` finds none in or around it, and the only Panic-carrying function in
 * 0x800E0xxx..0x800E4xxx is at 0x800E1C34.  The module therefore comes from the **placed link-neighbour**,
 * `sound/fn_800E46E8.cpp`, which starts exactly at this range's end (0x800E46E8) - the route
 * `.pi/notes/undecided-modules.report.md` used for the other `module: ?` units.  The content, however, is
 * a primitive/GX utility subsystem, not sound: the dump names `prim_init_all` / `set_blendmode` /
 * `set_zmode` suggest a `prim` subsystem whose real module may not be `sound`.  The trigger to revisit is
 * the dump showing a wider `prim_*` family elsewhere.
 *
 * What the range is, read off the disassembly:
 *   * `fn_800E3CBC` / `prim_init_all` / `fn_800E3D7C` / `fn_800E3DE0` reset and rebuild the two record
 *     pools (`.sbss` bases `lbl_807949A0`, counts `lbl_807949A8`, capacities `lbl_80791460`) and the six
 *     pointer free-lists (`.data` bases `lbl_80597D20`, counts `lbl_80597D38`).
 *   * `fn_800E3E1C` walks both record lists and drives per-record visibility, callbacks and state.
 *   * `fn_800E4148` / `fn_800E4284` / `fn_800E4390` / `fn_800E440C` / `fn_800E444C` are the draw queue:
 *     a depth metric, a sorted insertion and the per-bucket draw dispatch.
 *   * `set_blendmode` / `set_zmode` are thin GX state wrappers (GXSetBlendMode / GXSetZMode).
 *
 * `#pragma peephole off` is load-bearing, as on the sibling `sound/` units: with the peephole pass on,
 * `fn_800E444C`'s `rlwinm`+`cmpwi` zero test fuses into `rlwinm.`+`bne` and the whole unit loses ~5
 * points (fn_800E444C 74.5 -> 85.3, fn_800E4148 85.8 -> 89.7, fn_800E4284 89.9 -> 94.3).
 *
 * Measured against this range's split object with `recompile.py --measure` (the official report metric):
 *
 *   fn_800E3CBC     99.33 (132/132 B)   fn_800E3DE0   100.00   fn_800E444C     85.27 (296/288 B)
 *   prim_init_all  100.00               fn_800E3E1C    88.10 (812/820 B)   set_blendmode  100.00
 *   fn_800E3D7C     89.40 (100/100 B)   fn_800E4148    89.75 (316/324 B)   set_zmode       97.50
 *   fn_800E4284     94.33 (268/264 B)   fn_800E4390    98.06 (124/124 B)
 *   fn_800E440C    100.00
 *
 * All twelve are at or above the 80 % bar; the unit is 92.16 % fuzzy, 4 of 12 byte-identical.  Residuals:
 * `fn_800E3E1C` (+8 B) and `fn_800E4148` (+8 B) materialise an address the peephole pass would fold, and
 * `fn_800E444C` (-8 B) keeps one `lwz` fewer than retail across the `rec = (PrimRec*)local` reload;
 * `fn_800E3CBC`/`fn_800E3D7C`/`fn_800E4390`/`set_zmode` are size-exact and differ only in register
 * colouring.  `fn_800E4284` (-4 B) is the `(u32)((f32)(n - 1) * t)` conversion sequence.
 */

#include "types.h"
#include "nw4r/math.h"
#include "sound/fn_800E46E8.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

#pragma peephole off

/* ------------------------------------------------------------------ types */

/* The 0x34-byte primitive record both record lists hold.  Layout from the field accesses in this range
 * (`+0x04` sort key, `+0x18` handle, `+0x1C` kind, `+0x1D` layer, `+0x1E` flags, `+0x20` callback).
 * size: 0x34 */
typedef struct PrimRec PrimRec;
struct PrimRec {
    /* 0x00 */ u32 next;                       /* free-list / sorted-list link */
    /* 0x04 */ f32 key;                        /* depth metric (fn_800E4284 sorts on it) */
    /* 0x08 */ u8 state;                       /* 0 idle, 1/2 the two drawn states */
    /* 0x09 */ u8 started;                     /* the "callback already fired" latch */
    /* 0x0A */ u8 pad_0x0A[2];
    /* 0x0C */ VEC3 pos;                       /* +0x0C..+0x17; pos.y is read at +0x10 */
    /* 0x18 */ void* handle;                   /* engine object the record wraps */
    /* 0x1C */ u8 kind;                        /* 0/1/2: the three draw kinds */
    /* 0x1D */ u8 layer;                       /* 0..5: which free-list / sort direction */
    /* 0x1E */ u16 flags;                      /* bit field; see the masks below */
    /* 0x20 */ void (*callback)(PrimRec*);
    /* 0x24 */ f32 limit_lo;                   /* lower cull bound (fnegated) */
    /* 0x28 */ f32 limit_hi;                   /* upper cull bound */
    /* 0x2C */ f32 f_0x2C;
    /* 0x30 */ u8 pad_0x30[4];
};

/* The working buffer `MTX34_ctor` initialises; fn_800E4148 reads +0x20..+0x2C, fn_800E3E1C reads +0x1C.
 * Only the read fields are named; the rest is untouched padding.  size: 0x30 (approximate) */
typedef struct WorkBuf {
    /* 0x00 */ u8 pad_0x00[0x1C];
    /* 0x1C */ f32 f_0x1C;                     /* fn_80081714 writes it; read back as f1 */
    /* 0x20 */ VEC3 v_0x20;                    /* the projected point fn_80052214 reads */
    /* 0x2C */ f32 f_0x2C;
};

/* The object `fn_80047234` returns.  Only +0xB4 / +0xB8 are read.  size: 0xBC (approximate) */
typedef struct DrawCtx {
    /* 0x00 */ u8 pad_0x00[0xB4];
    /* 0xB4 */ f32 f_0xB4;
    /* 0xB8 */ f32 f_0xB8;
    /* 0xBC */ u8 pad_0xBC[0x00];
};

/* The leading vtable of a draw handle (`fn_800E3E1C` calls slot 3 = +0xC).  size: 0x10 */
typedef struct Obj Obj;
typedef struct ObjVtbl {
    /* 0x00 */ void (*slot0)();
    /* 0x04 */ void (*slot1)();
    /* 0x08 */ void (*slot2)();
    /* 0x0C */ void (*slot3)(Obj* self, u32 a, u32 b, WorkBuf* c);
} ObjVtbl;

/* The secondary interface `fn_800E444C` reaches through +0x1C and whose slot at +0x1C it calls.
 * size: 0x20 (approximate) */
typedef struct ObjSub {
    /* 0x00 */ u8 pad_0x00[0x1C];
    /* 0x1C */ void (*slot7)(Obj* self, void* m);
} ObjSub;

/* A draw handle.  size: 0x20 */
struct Obj {
    /* 0x00 */ ObjVtbl* vtbl;
    /* 0x04 */ u8 pad_0x04[0x18];
    /* 0x1C */ ObjSub* sub;
};

/* The manager `lbl_80794970` points at; only +0x29084 is read (the trailing struct size is therefore
 * approximate - the real record is longer).  size: 0x29088 (approximate) */
typedef struct PrimMgr {
    /* 0x00000 */ u8 pad_0x00000[0x29084];
    /* 0x29084 */ void* field_0x29084;
} PrimMgr;

/* ------------------------------------------------------------------ externs */

/* Not-yet-reconstructed `sound`-band symbols (the bracketing registered units both name `sound`); the
 * declarations belong in `include/unsplit/sound.h` and are requested there in the outbox. */
extern "C" {
extern u32 lbl_807949A0[2];     /* .sbss record-list bases */
extern u32 lbl_807949A8[2];     /* .sbss record-list counts */
extern u16 lbl_80791460[2];     /* .sdata record-list capacities */
extern void* lbl_80597D20[6];   /* .data free-list bases */
extern u16 lbl_80597D38[6];     /* .data free-list counts */
extern void* lbl_80794970;      /* .sbss manager pointer */
extern f32 lbl_80796458;
extern f32 lbl_8079645C;
extern f32 lbl_80796460;
extern f32 lbl_80796464;
extern f32 lbl_80796468;
extern f64 lbl_80796470;
void fn_800E3C90(void* p);
void fn_80075394(void* a, WorkBuf* out);
void* fn_80047234(void* a);
s32 fn_802AFF38(void);
void fn_80074AA8(void* a, u32 b, f32* out, u32 d);
s32 fn_802AFF58(void);
s32 fn_800D0724(void);
s32 fn_802BE39C(void);
s32 camera_work_ck(void);
s32 my_player_no(void);
void fn_8007F77C(void* p);
void* fn_800A60C0(void* p);
void fn_80081714(void* a, u32 b, WorkBuf* out);
f32 fn_80052214(const VEC3* a, const VEC3* b);
void* fn_8007B544(void* a, u32 b);
void fn_80049728(void* p, s32 n);
void fn_80088590(s32 n);
void GXInvalidateVtxCache(void);
void GXSetBlendMode(u32 a, u32 b, u32 c, u32 d);
void GXSetZMode(u32 a, u32 b, u32 c);
}

/* `sound/fn_800E46E8.cpp` owns fn_800E46E8; its declaration is in include/sound/fn_800E46E8.h. */

/* Forward declarations for the mutually recursive bodies below. */
extern "C" s32 fn_800E4148(PrimRec* self, const WorkBuf* arg);
extern "C" void fn_800E4284(PrimRec* self, u8 layer, f32 f1, f32 f2);
extern "C" void fn_800E4390(u32* link, PrimRec* node, u32 flag);
extern "C" u32 fn_800E440C(u32* p);

/* ------------------------------------------------------------------ functions */

/* Reset record list `idx`: clear its count and run the per-slot destructor over the whole capacity. */
extern "C" void fn_800E3CBC(u8 idx)
{
    PrimRec* p = (PrimRec*)lbl_807949A0[idx];

    lbl_807949A8[idx] = 0;
    for (u32 i = 0; i < lbl_80791460[idx]; i++) {
        fn_800E3C90(p);
        p++;
    }
}

/* Rebuild both record lists. */
void prim_init_all(void)
{
    for (u32 i = 0; i < 2; i++) {
        fn_800E3CBC(i);
    }
}

/* Rebuild free-list `idx`: every slot points at the next one, the last is null. */
extern "C" void fn_800E3D7C(u8 idx)
{
    u32* base = (u32*)lbl_80597D20[idx];
    u32* p = base;
    u32 i;

    for (i = 0; i < lbl_80597D38[idx] - 1; i++) {
        *p = (u32)base + (i + 1) * 4;
        p++;
    }
    base[i] = 0;
}

/* Rebuild all six free-lists. */
extern "C" void fn_800E3DE0(void)
{
    for (u32 i = 0; i < 6; i++) {
        fn_800E3D7C(i);
    }
}

/* The per-frame driver: for both record lists, run each live record's callback, state and draw setup.
 * `mode` selects the state filter (`1` keeps only the flag-0x2 records). */
extern "C" void fn_800E3E1C(void* a, u8 mode)
{
    WorkBuf b40;
    WorkBuf b10;
    f32 scalef;
    f32 f31;

    MTX34_ctor(&b40);
    MTX34_ctor(&b10);
    GXInvalidateVtxCache();
    fn_80075394(a, &b40);
    DrawCtx* ctx = (DrawCtx*)fn_80047234(a);
    s32 sel = fn_802AFF38();
    if (sel == 1) {
        f31 = lbl_80796458;
        fn_80074AA8(a, 0, &scalef, 0);
    }
    s32 sub = fn_802AFF58();
    for (u32 i = 0; i < 2; i++) {
        PrimRec* p = (PrimRec*)lbl_807949A0[i];
        for (u32 j = 0; j < lbl_807949A8[i]; j++, p++) {
            if (!(p->flags & 0x200)) {
                if (fn_800D0724()) {
                    continue;
                }
            }
            if (p->flags & 0xC08) {
                if (fn_802BE39C() == 1) {
                    if ((p->flags & 0x400) && (s8)my_player_no() == 0) {
                        continue;
                    }
                    if ((p->flags & 0x800) && (s8)my_player_no() == 1) {
                        continue;
                    }
                    if (p->flags & 0x8) {
                        continue;
                    }
                }
            }
            if (p->flags & 0x20) {
                if (camera_work_ck() == 1) {
                    continue;
                }
            }
            p->state = 0;
            if (mode == 1) {
                if (!(p->flags & 0x2)) {
                    continue;
                }
            }
            if (p->kind <= 1) {
                Obj* h = (Obj*)p->handle;
                if (p->started == 0) {
                    if ((p->flags & 0x40) && p->callback != 0) {
                        p->callback(p);
                    }
                    fn_8007F77C(h);
                    p->started = 1;
                }
                h->vtbl->slot3(h, 4, 0, &b40);
            }
            if (!fn_800E4148(p, &b40)) {
                continue;
            }
            switch (p->layer) {
            case 1:
            case 2: {
                f32 f1;
                if (p->flags & 1) {
                    f1 = p->pos.y;
                } else if (p->kind > 1) {
                    if (p->kind == 2) {
                        f1 = ((f32*)fn_800A60C0(p->handle))[7];
                    } else {
                        f1 = lbl_80796458;
                    }
                } else {
                    fn_80081714((Obj*)p->handle, 0, &b10);
                    f1 = b10.f_0x1C;
                }
                if (sel == 1) {
                    if (f31 < scalef) {
                        p->state = (f1 < f31) ? 2 : 1;
                    } else {
                        p->state = (f1 < f31) ? 1 : 2;
                    }
                }
                break;
            }
            case 0:
                if (sub == 1) {
                    p->state = (p->flags & 0x4) ? 2 : 1;
                }
                break;
            default:
                break;
            }
            fn_800E4284(p, p->layer, ctx->f_0xB4, ctx->f_0xB8);
        }
    }
}

/* Project the record against `arg` and decide whether it survives this frame's culling. */
extern "C" s32 fn_800E4148(PrimRec* self, const WorkBuf* arg)
{
    VEC3 v;

    VEC3_ctor(&v);
    if (self->flags & 0x1) {
        fn_80052214(&self->pos, &arg->v_0x20);
        self->key = -(arg->f_0x2C + fn_80052214(&self->pos, &arg->v_0x20));
    } else if (self->kind <= 1) {
        f32* m = (f32*)fn_8007B544(self->handle, 2);
        self->key = -m[0xB];
    } else if (self->kind == 2) {
        f32* m = (f32*)fn_800A60C0(self->handle);
        setVector3(&v, m[3], m[7], m[11]);
        self->key = -(arg->f_0x2C + fn_80052214(&v, &arg->v_0x20));
    } else {
        return 0;
    }
    if (self->flags & 0x80) {
        if (self->key < -self->limit_lo) {
            return 0;
        }
    }
    if (self->flags & 0x100) {
        if (self->key > self->limit_hi) {
            return 0;
        }
    }
    return 1;
}

/* Insert `self` into free-list `layer`'s sorted chain at the position the depth metric picks. */
extern "C" void fn_800E4284(PrimRec* self, u8 layer, f32 f1, f32 f2)
{
    u32* base = (u32*)lbl_80597D20[layer];
    u32 n = lbl_80597D38[layer];
    f32 t = (self->key - f1) / f2;

    if (t < lbl_80796460) {
        t = lbl_8079645C * t / lbl_80796460;
    } else {
        t = lbl_8079645C + lbl_80796464 * (lbl_80796468 - t);
    }
    if (t < lbl_80796458) {
        t = lbl_80796458;
    } else if (t > lbl_80796468) {
        t = lbl_80796468;
    }
    u32 k = (u32)((f32)(n - 1) * t);
    fn_800E4390(&base[n - 1 - k], self, layer);
}

/* Sorted-list insert.  `link` is the slot the chain hangs off; the low bit of a stored link marks the
 * chain's tail, so it is masked off when a node is walked.  `flag == 0` inserts at the head. */
extern "C" void fn_800E4390(u32* link, PrimRec* node, u32 flag)
{
    u32 f = flag & 0xFF;

    for (;;) {
        u32 raw = *link;
        if (raw == 0 || (raw & 1) == 0) {
            *link = (u32)node | 1;
            node->next = raw;
            return;
        }
        PrimRec* cur = (PrimRec*)(raw & ~1u);
        u32 cond;
        if (f == 0) {
            cond = 1;
        } else {
            cond = (u32)(cur->key <= node->key);
        }
        if (cond) {
            *link = (u32)node | 1;
            node->next = raw;
            return;
        }
        link = &cur->next;
    }
}

/* Advance the chain head past the tag and return the node now at the head (the caller draws it). */
extern "C" u32 fn_800E440C(u32* p)
{
    u32 v = *p;

    if (v == 0) {
        return 0;
    }
    u32 n = *(u32*)v;
    if (n & 1) {
        u32 r = n & ~1u;
        *p = r;
        return r;
    }
    *p = n;
    return fn_800E440C(p);
}

/* Draw one free-list bucket.  `mode` filters on the record state. */
extern "C" void fn_800E444C(u8 idx, u8 mode)
{
    u32 local = (u32)lbl_80597D20[idx];
    PrimRec* rec;

    for (;;) {
        rec = (PrimRec*)fn_800E440C(&local);
        local = (u32)rec;
        if (rec == 0) {
            return;
        }
        if (mode == 2) {
            if (rec->state != 2) {
                continue;
            }
        } else if (mode == 3) {
            if (rec->state == 2) {
                continue;
            }
        }
        if ((rec->flags & 0x40) == 0 && rec->callback != 0) {
            rec->callback(rec);
        }
        rec = (PrimRec*)local;
        if (rec->kind <= 1) {
            if (mode == 1) {
                fn_80049728(rec->handle, 4);
            } else {
                fn_80049728(rec->handle, -1);
            }
            continue;
        }
        if (rec->kind == 2) {
            Obj* h = (Obj*)rec->handle;
            void* m = ((PrimMgr*)lbl_80794970)->field_0x29084;
            h->sub->slot7(h, m);
            fn_80088590(0x7FF);
            continue;
        }
    }
}

/* Map the three arguments through their GX enums and tail-call GXSetBlendMode. */
void set_blendmode(u8 mode, u8 src, u8 dst)
{
    u32 m;
    u32 s;

    switch (mode) {
    case 0: m = 0; break;
    case 1: m = 1; break;
    case 2: m = 2; break;
    case 3: m = 3; break;
    case 4: m = 4; break;
    case 5: m = 5; break;
    case 6: m = 6; break;
    case 7: m = 7; break;
    default: m = 4; break;
    }
    switch (src) {
    case 0: s = 0; break;
    case 1: s = 1; break;
    case 2: s = 2; break;
    case 3: s = 3; break;
    case 4: s = 4; break;
    case 5: s = 5; break;
    case 6: s = 6; break;
    case 7: s = 7; break;
    }
    u32 d;
    switch (dst) {
    case 0: d = 0; break;
    case 1: d = 1; break;
    case 3: d = 3; break;
    default: d = 1; break;
    }
    GXSetBlendMode(d, m, s, 0);
}

/* Map the depth test through the GX compare table and tail-call GXSetZMode. */
void set_zmode(bool enable, u8 func, bool write)
{
    u32 zf;
    u32 on;

    if (enable) {
        on = 1;
        zf = fn_800E46E8(func);
    } else {
        on = 0;
        zf = 3;
    }
    GXSetZMode(on, zf, write);
}
