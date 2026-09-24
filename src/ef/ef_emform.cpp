/*
 * auto/800CCCF8_fn_800CCCF8.c - the effect-emitter shape registry: seven shape constructors, one static
 * initializer and one lookup.  `.text` 0x800CCCF8-0x800CCFB0 (10 functions, 0x2B8 B) and the unit's
 * `.ctors` word at 0x8056F2E8.  All four sections (`.text`, `.ctors`, `extab`, `extabindex`) are
 * byte-identical to the target object.
 *
 * The unit owns one registered instance per emitter shape.  `fn_800CCDA8` (the `.ctors` entry) constructs
 * the seven instances `lbl_80794938`..`lbl_80794950`; `fn_800CCCF8` maps an id to one of them.  `fn_800CCE38`
 * is the shared base constructor (stores the abstract base vtable `lbl_80594ED0`); the seven
 * `fn_800CCDFC`..`fn_800CCF74` are the derived constructors, each calling the base and then overwriting the
 * vtable pointer at +0 with its shape's own vtable.  An instance is a bare vtable pointer, so the
 * reconstructed type is 4 bytes with one field.
 *
 * The vtable, instance and string symbols are owned by other units (the seven instances by the `.sbss`
 * data unit); they are declared `extern` here and never defined (playbook 29), which is how the target
 * object holds them - all undefined.
 *
 * Flags: the unit needs `cflags_main` **plus `#pragma peephole off`**.  The peephole is the only lever:
 * with it on every constructor computes the vtable address into the `lis` register (`addi r4, r4, sym@l`),
 * where retail computes into `r0` (`addi r0, r4, sym@l`) - 97.5 % on the base constructor and 99.33 % on
 * each derived one; with it off, 100 %.  The pragma covers the whole unit, matching the original TU's one
 * flag set.
 *
 * Two source shapes are load-bearing: the lookup's id parameter is **signed** (`cmplwi` for every case but
 * 0 otherwise) and its cases are written in retail's unsorted order (0, 1, 7, 8, 10, 5, 9), which is the
 * `cmpwi` chain MWCC emits; and the `.ctors` word is placed with `__declspec(section ".ctors")` alone,
 * because the `#pragma section const_type` pair answers 33041 for the plain `.ctors` name.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/800CCCF8_fn_800CCCF8.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object names
 * the original source file.
 */

#include "types.h"

#pragma peephole off

/* One registered shape: the object itself is nothing but its vtable pointer. */
typedef struct EmForm {
    /* +0x00 */ void* vtable;
} EmForm; /* size: 0x04 */

/* Shape vtables.  Each is { offset-to-top, typeinfo, virtual fn, [virtual fn] }, owned by the shape's own
 * unit; declared here, never defined.  The comment is the `__FILE__` string the unit pools after it. */
extern u32 lbl_80594ED0[];   /* base (ef_line.cpp) */
extern u32 lbl_80594C58[];   /* ef_cube.cpp */
extern u32 lbl_80594D14[];   /* ef_cylinder.cpp */
extern u32 lbl_80594DD0[];   /* ef_disc.cpp */
extern u32 lbl_80594E8C[];   /* ef_emform.cpp */
extern u32 lbl_80594F8C[];   /* ef_point.cpp */
extern u32 lbl_80595048[];   /* ef_sphere.cpp */
extern u32 lbl_80595108[];   /* no pooled file string */

/* The seven registered instances (owned by the .sbss data unit). */
extern EmForm lbl_80794938;
extern EmForm lbl_8079493C;
extern EmForm lbl_80794940;
extern EmForm lbl_80794944;
extern EmForm lbl_80794948;
extern EmForm lbl_8079494C;
extern EmForm lbl_80794950;

/* nw4r::db::Panic's file/format strings and the variadic entry point. */
extern const char lbl_80594E98[];
extern const char lbl_80594EA8[];
/* nw4r::db::Panic. The map already carries its real C++ mangling
 * (Panic__Q24nw4r2dbFPCciPCce), and declaring that spelling as a C++ identifier re-mangles it
 * (Panic__Q24nw4r2dbFPCciPCce__FPCciPCce) - which only shows up at LINK time, so a NonMatching
 * unit hides it until it is flipped. Declare the real thing and the front-end reproduces the
 * map's spelling exactly: tools/units/mangle.py confirms it. */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }
void fn_800CCE38(EmForm* self);
EmForm* fn_800CCDFC(EmForm* self);
EmForm* fn_800CCE48(EmForm* self);
EmForm* fn_800CCE84(EmForm* self);
EmForm* fn_800CCEC0(EmForm* self);
EmForm* fn_800CCEFC(EmForm* self);
EmForm* fn_800CCF38(EmForm* self);
EmForm* fn_800CCF74(EmForm* self);

/* Maps a shape id to its registered instance, or asserts and returns null.  The first parameter is the
 * virtual dispatch's `this`, which the function never reads. */
EmForm* fn_800CCCF8(void* self, int id)
{
    switch (id) {
    case 0:  return &lbl_80794938;
    case 1:  return &lbl_8079493C;
    case 7:  return &lbl_80794940;
    case 8:  return &lbl_80794944;
    case 10: return &lbl_80794948;
    case 5:  return &lbl_8079494C;
    case 9:  return &lbl_80794950;
    default:
        nw4r::db::Panic(lbl_80594E98, 65, lbl_80594EA8);
        return 0;
    }
}

/* Static initializer: constructs the seven registered shape instances. */
void fn_800CCDA8(void)
{
    fn_800CCF74(&lbl_80794938);
    fn_800CCF38(&lbl_8079493C);
    fn_800CCEFC(&lbl_80794940);
    fn_800CCEC0(&lbl_80794944);
    fn_800CCE84(&lbl_80794948);
    fn_800CCE48(&lbl_8079494C);
    fn_800CCDFC(&lbl_80794950);
}

/* Derived constructors: run the base constructor, then install the shape's own vtable.  They are written
 * in the target's `.text` order, so `fn_800CCDFC` comes before the base constructor it calls. */
EmForm* fn_800CCDFC(EmForm* self)
{
    fn_800CCE38(self);
    self->vtable = lbl_80595048;
    return self;
}

/* Base constructor: installs the abstract base's vtable. */
void fn_800CCE38(EmForm* self)
{
    self->vtable = lbl_80594ED0;
}

EmForm* fn_800CCE48(EmForm* self)
{
    fn_800CCE38(self);
    self->vtable = lbl_80594D14;
    return self;
}

EmForm* fn_800CCE84(EmForm* self)
{
    fn_800CCE38(self);
    self->vtable = lbl_80594C58;
    return self;
}

EmForm* fn_800CCEC0(EmForm* self)
{
    fn_800CCE38(self);
    self->vtable = lbl_80595108;
    return self;
}

EmForm* fn_800CCEFC(EmForm* self)
{
    fn_800CCE38(self);
    self->vtable = lbl_80594DD0;
    return self;
}

EmForm* fn_800CCF38(EmForm* self)
{
    fn_800CCE38(self);
    self->vtable = lbl_80594F8C;
    return self;
}

EmForm* fn_800CCF74(EmForm* self)
{
    fn_800CCE38(self);
    self->vtable = lbl_80594E8C;
    return self;
}

#pragma peephole reset

/* The `.ctors` word the linker walks: this unit's static initializer reference. */
__declspec(section ".ctors") void* const lbl_8056F2E8 = (void*)fn_800CCDA8;
