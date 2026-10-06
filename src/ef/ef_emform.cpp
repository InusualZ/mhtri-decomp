/*
 * ef/ef_emform.cpp - the emitter-form registry: the shared base constructor (stores the abstract table
 *   `lbl_80594ED0`), seven derived constructors (each installs its shape's table over the base's), the static
 *   initializer `fn_800CCDA8` that builds the seven instances `lbl_80794938`-`lbl_80794950`, and the id lookup
 *   `fn_800CCCF8`.
 * RANGE. .text 0x800CCCF8-0x800CCFB0 (10 functions); extab 0x8000A584-0x8000A5CC, extabindex 0x80023BEC-0x80023C58,
 *   .ctors 0x8056F2E8-0x8056F2EC, .data 0x80594E8C-0x80594EE0, .sbss 0x80794938-0x80794958 (the seven instances).
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail computes each table address into r0, `addi r0,
 *   r4, sym@l`, where the peephole pass reuses the `lis` register).
 * NAMES. The file name is a GUESS: nothing in the object names the source file.
 * RESIDUALS. none in `.text`, `.ctors`, extab or extabindex (byte-identical).
 *   flipcheck: `.data` and `.sbss` claimed, not emitted (the tables and instances are declared, never defined).
 *   relocdiff: our `.ctors` word carries the symbol `lbl_8056F2E8`, retail's none; both relocate to
 *   `fn_800CCDA8__Fv`.
 * SHAPES. The lookup's id is signed, and its cases are in retail's order (0, 1, 7, 8, 10, 5, 9): that is the
 *   `cmpwi` chain MWCC emits.
 * SHAPES. The `.ctors` word is placed with `__declspec(section ".ctors")` alone (the `#pragma section const_type`
 *   pair answers error 33041 for the plain `.ctors` name).
 */

#include "types.h"

#pragma peephole off

/* One registered shape: the object itself is nothing but its vtable pointer. */
typedef struct EmForm {
    /* +0x00 */ void* vtable;
} EmForm; /* size: 0x04 */

/* Shape tables, each { offset-to-top, typeinfo, virtual fn, [virtual fn] }, declared, never defined.  The
 * comment is the `__FILE__` string pooled after it. */
extern u32 lbl_80594ED0[];   /* base (ef_line.cpp) */
extern u32 lbl_80594C58[];   /* ef_cube.cpp */
extern u32 lbl_80594D14[];   /* ef_cylinder.cpp */
extern u32 lbl_80594DD0[];   /* ef_disc.cpp */
extern u32 lbl_80594E8C[];   /* ef_emform.cpp */
extern u32 lbl_80594F8C[];   /* ef_point.cpp */
extern u32 lbl_80595048[];   /* ef_sphere.cpp */
extern u32 lbl_80595108[];   /* no pooled file string */

/* The seven registered instances (this unit's claimed `.sbss`). */
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
/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
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

/* Derived constructors: run the base constructor, then install the shape's own table.  In retail's
 * `.text` order the first one precedes the base constructor it calls. */
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
