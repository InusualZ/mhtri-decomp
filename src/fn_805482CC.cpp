/* Monster Hunter Tri (RMHE08) - un-moduled game-UI band, `.text` 0x805482CC-0x8054E894
 * (71 functions / 26056 B), reconstructed from the split target object.
 *
 * Registration: the range is proposal `805482CC_fn_805482CC` from discovery's attribution queue; its
 * edges are a `--max-bytes` cut, not a translation-unit boundary (`python tools/splits/tudiscover.py
 * at 0x805482CC` reports only weak signals at both ends, no must-link anchor at all), so the run is
 * worked as one unit.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked: `tudiscover.py at
 * 0x805482CC` gives no name evidence; `python tools/symbols/dumpmap.py lookup` answers `zz_<addr>_`
 * for most addresses and, for the rest, a repeated and clearly misattributed set of SDK names -
 * `DBClose`, `gdev_cc_shutdown`, `J3DAnonVisibilityFull::~...` - at dozens of addresses across the
 * band, so it is no evidence; and the range's `.data` refs are UI part-name strings only, no
 * `__FILE__` emitter).  The module is likewise un-evidenced, so the file keeps the map's stem at the
 * repository root, like the neighbouring un-moduled game files `fn_80056F24.cpp`/`Network/network_pat_control.cpp`.
 *
 * Language C++ and lib, from the range's own structure: vptr dispatch, adjustor thunks
 * (`subi r3, r3, 0x10`), deleting destructors calling `__dl__FPv`, and the target object's flags.
 * It takes the game-root `main` lib with cflags_main, the same as its link neighbours
 * `homebutton/keyboard_ui.cpp` above and `homebutton/fn_80555374.cpp` below.
 *
 * What the range is: the panel class whose constructor is `fn_80542D8C` (0x80542D8C, in the band
 * below this one) - its field stores at +0x0, +0x5C, +0x118, +0x18F8..+0x1A00 pin the layout used
 * here.  The panel owns three heap helper objects at +0x193C / +0x1940 / +0x1944 that share one
 * dispatch table, a scroll record embedded at +0x1964 (vtable from `lbl_8064F338`), and a
 * sub-object at +0x10 whose dispatch table pointer sits at its own +0x4C (`fn_80542D8C` stores
 * `lbl_80651708` there).  The vocabulary it drives is the UI text-box pool: `lbl_8064E760`
 * "P_txtScrll_UP", `lbl_8064EC08` "T_As_TextBox_00", `lbl_8064EC18` "W_TextBox_02".
 *
 * Sections this unit owns: `.text` 0x805482CC..0x8054E894 only.  The target object carries no data
 * section at all, so every constant the range loads is another unit's pool entry and is declared
 * `extern` here (playbook 29/58), never defined - see `include/unsplit/unknown.h`.
 *
 * Sections: our object emits `.text` only, the same section list as the target object - `datagap.py
 * --unit fn_805482CC --mode both` reports an empty `ours-extra`, and the `extab`/`extabindex` the lib's
 * `-Cpp_exceptions on` would add are suppressed with the scoped `#pragma exceptions off` below.  The
 * unit claims `.text` and nothing else.
 *
 * Residuals (measured with `python tools/units/recompile.py fn_805482CC --measure <symbol>`; the unit
 * reads 10.01 % fuzzy at this commit):
 *  - 38 of the range's 71 functions are reconstructed; 35 (22796 B) are not, the largest being
 *    fn_80549D9C (2880 B), fn_8054D0AC (1328 B), fn_8054C414 (1104 B) and fn_8054BFE4 (1072 B).
 *    They keep their map names and have no body here.
 *  - fn_80548394 (13.75 %): the store schedule differs - retail materialises the u8 `1` in r6 and
 *    keeps the argument pointer in r4, ours copies the pointer to r6 and materialises the `1` in r0,
 *    which shifts every following instruction (positional metric).  The record layout and the tail
 *    call through slot 0x08 are right.
 *  - fn_805482CC (65.6 %): retail keeps the sub-object address computation and its table load
 *    separate (`addi r3, r3, 0x1964` after the two loads); our `#pragma peephole off` removes the
 *    `lwzu` fusion but places the `addi` first.  The relative branch's float loads are also ordered
 *    the other way (retail loads 0x195C before 0x1960).
 *  - fn_80548B10 (77.8 %) / fn_80548950 (80.5 %): the argument-to-callee-saved-register choice is
 *    inverted (`self` in r31 where retail has `key`, and vice versa), which rotates the prologue.
 *  - fn_80548E40 (76.2 %) / fn_80548FE4 (not written): retail carries a `cmplw r10, r10` self-compare
 *    (a comparison the optimiser CSE'd to one load) that the source cannot express without a `goto`;
 *    omitting it costs the two instructions and shifts the block.  Rule 8.
 *  - fn_80549C50 (69.9 %): the `fn_8054F788(&self->field_0x118)` argument is materialised earlier in
 *    ours than in retail.
 */

#include "types.h"

#include "fn_805482CC.h"
#include "homebutton/fn_80555374.h"
#include "unsplit/unknown.h"

/* The lib's cflags turn C++ exceptions on (`cflags_main`), but this unit's target object carries no
 * `extab`/`extabindex` at all, so the original translation unit was built with them off - the scoped
 * pragma pair puts our object back on the target's section list (playbook 30). */
#pragma exceptions off

/* This unit's own later entry point, called by fn_805486B0 before its definition. */
void fn_805492A8(Panel805482CC* self, u32* before, u32* after);
void fn_80549570(Panel805482CC* self, Panel805482CC_Cell* cell, u32 count);
void fn_8054E064(Panel805482CC* self, s32 b, s32 c);

#ifdef __cplusplus
extern "C" {
#endif

/* --- this unit's own entry points, in address order (so the bodies read in the object's order) --- */
void fn_805482CC(Panel805482CC* self, Panel805482CC_Change* change);
void fn_80548394(Panel805482CC_Set* set, Panel805482CC_Item* item, u32 id, f32 a, f32 b, f32 c);
void fn_805483E4(Panel805482CC* self);
void fn_80548680(Panel805482CC* self);
void fn_80548694(void);
void fn_80548698(Panel805482CC* self);
void fn_805486AC(void);
void fn_805486B0(Panel805482CC* self);
s32 fn_8054893C(void);
s32 fn_80548944(void);
void fn_8054894C(void);
void fn_80548950(Panel805482CC* self, u32 key);
void fn_80548B04(void);
void fn_80548B08(Panel805482CC* self, u16 value);

/*
 * Apply a text-position change to the panel, then tell the panel about it.
 *
 * `change->field_0x00` selects absolute (the record's two floats are the new position) from relative
 * (`+0x04` / `+0x08` are deltas added to the current position) handling; either way the panel reports
 * the move to itself through its own dispatch slot 0x17C.
 *
 * `#pragma peephole off`: retail keeps the sub-object address computation and its table load separate
 * (`addi r3, r3, 0x1964` + `lwz r12, 0x1964(r3)`); with the pass on MWCC fuses them into one
 * `lwzu r12, 0x1964(r3)`, which moves every later instruction.  Playbook 39.
 */
#pragma peephole off
void fn_805482CC(Panel805482CC* self, Panel805482CC_Change* change)
{
    if (self->scroll.vtbl->fn_0x14(&self->scroll) != 0) {
        return;
    }
    if (change->field_0x00 != 0) {
        self->field_0x195C = change->field_0x04;
        self->field_0x1960 = change->field_0x08;
        self->sub.field_0xF0 = change->field_0x08;
    } else {
        self->field_0x195C += change->field_0x04;
        self->scroll.vtbl->fn_0x08(&self->scroll, 0, 0, self->field_0x1960,
                                   self->field_0x1960 + change->field_0x08, lbl_8079D698);
    }
    self->vtbl->fn_0x17C(self, 11);
}
#pragma peephole on

/*
 * Initialise a text-position record: the three positions, the 640.0f row bound, the "active" byte and
 * the object/id it belongs to, then hand the record to its owning object's dispatch slot 0x08.
 */
void fn_80548394(Panel805482CC_Set* set, Panel805482CC_Item* item, u32 id, f32 a, f32 b, f32 c)
{
    set->field_0x04 = a;
    set->field_0x08 = b;
    set->field_0x10 = c;
    set->field_0x0C = lbl_8079D628;
    set->field_0x14 = 1;
    set->field_0x18 = item;
    set->field_0x1C = id;
    set->field_0x15 = 0;
    if (item != NULL) {
        (*(Panel805482CC_ItemVtbl**)item)->fn_0x08(item, 0);
    }
}

/* Dispatch the panel's object at +0x1940 through its slot 0x110 (a tail call in retail). */
void fn_80548680(Panel805482CC* self)
{
    Panel805482CC_Item* item = self->field_0x1940;

    (*(Panel805482CC_ItemVtbl**)item)->fn_0x110(item);
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_80548694(void)
{
}

/* Dispatch the panel's object at +0x1940 through its slot 0x114 (a tail call in retail). */
void fn_80548698(Panel805482CC* self)
{
    Panel805482CC_Item* item = self->field_0x1940;

    (*(Panel805482CC_ItemVtbl**)item)->fn_0x114(item);
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_805486AC(void)
{
}

/*
 * Advance the panel one frame: with no input latched, pull the current position from the object at
 * +0x193C and push the delta into the text record and the panel's own cursor; with input latched,
 * either accept the object at +0x1940's pending selection or the one at +0x1944.
 */
void fn_805486B0(Panel805482CC* self)
{
    Panel805482CC_Item* item;

    if (self->field_0x1950 == 0) {
        s32 before;
        s32 after;

        item = self->field_0x193C;
        if ((*(Panel805482CC_ItemVtbl**)item)->fn_0xCC(item) != 0) {
            return;
        }
        before = (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x88(self->field_0x193C);
        item = self->field_0x193C;
        (*(Panel805482CC_ItemVtbl**)item)->fn_0xD4(item);
        item = self->field_0x193C;
        after = (*(Panel805482CC_ItemVtbl**)item)->fn_0x88(item);
        self->field_0x19F0->field_0x04 += (u16)(after - before);
        self->field_0x19F8 += (u16)(after - before);
        self->sub.mpVtbl->fn_0x90(&self->sub);
        self->sub.mpVtbl->fn_0x94(&self->sub);
    } else if (self->field_0x194C == 1) {
        u32 value;

        item = self->field_0x1940;
        if ((*(Panel805482CC_ItemVtbl**)item)->fn_0xFC(item) != 0) {
            item = self->field_0x1940;
            value = (*(Panel805482CC_ItemVtbl**)item)->fn_0x100(item);
            self->vtbl->fn_0x18(self, 21, &value);
            return;
        }
        item = self->field_0x1940;
        (*(Panel805482CC_ItemVtbl**)item)->fn_0x6C(item, 0);
        {
            s32 selected = (*(Panel805482CC_ItemVtbl**)self->field_0x1940)->fn_0xE8(
                self->field_0x1940);
            u32 before = (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x88(
                self->field_0x193C);
            u32 after;

            item = self->field_0x193C;
            (*(Panel805482CC_ItemVtbl**)item)->fn_0x6C(item, selected);
            after = (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x88(self->field_0x193C);
            if (before != after) {
                fn_805492A8(self, &before, &after);
            }
        }
        item = self->field_0x1940;
        (*(Panel805482CC_ItemVtbl**)item)->fn_0x11C(item, 0);
    } else {
        s32 selected = (*(Panel805482CC_ItemVtbl**)self->field_0x1944)->fn_0xE8(
            self->field_0x1944);
        u32 before = (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x88(
            self->field_0x193C);
        u32 after;

        item = self->field_0x193C;
        (*(Panel805482CC_ItemVtbl**)item)->fn_0x60(item, selected);
        after = (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x88(self->field_0x193C);
        if (before != after) {
            fn_805492A8(self, &before, &after);
        }
        item = self->field_0x1944;
        (*(Panel805482CC_ItemVtbl**)item)->fn_0xD8(item);
        self->field_0x19E0 = 0;
    }
}

/* Constant-0 accessor (retail has no body beyond `return 0`). */
s32 fn_8054893C(void)
{
    return 0;
}

/* Constant-0 accessor (retail has no body beyond `return 0`). */
s32 fn_80548944(void)
{
    return 0;
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_8054894C(void)
{
}

/*
 * Route a key to the object the panel currently has selected, then run its own key handler.
 *
 * The three helper objects are tried in turn (the panel's input byte at +0x1950 and its mode word at
 * +0x194C pick which), and the key is special-cased for the two ids at 0x309B/0x309C.
 */
void fn_80548950(Panel805482CC* self, u32 key)
{
    Panel805482CC_Item* item;

    if (self->field_0x1950 == 0) {
        item = self->field_0x193C;
    } else if (self->field_0x194C != 1) {
        item = self->field_0x1944;
        if ((*(Panel805482CC_ItemVtbl**)item)->fn_0xE4(item) == 0) {
            item = self->field_0x1944;
        } else {
            item = self->field_0x193C;
        }
    } else {
        item = self->field_0x1940;
        if ((*(Panel805482CC_ItemVtbl**)item)->fn_0xD8(item) == 0) {
            item = self->field_0x1940;
        } else {
            item = self->field_0x193C;
        }
    }

    if (self->field_0x1950 != 0 && self->field_0x194C == 1) {
        if (key - 0x309B <= 1) {
            self->vtbl->fn_0x18(self, 27, NULL);
        } else {
            Panel805482CC_Item* selected = self->field_0x1940;

            if ((*(Panel805482CC_ItemVtbl**)selected)->fn_0xFC(selected) != 0) {
                self->vtbl->fn_0x18(self, 6, NULL);
            }
            selected = self->field_0x1940;
            (*(Panel805482CC_ItemVtbl**)selected)->fn_0x108(selected, key);
        }
    } else if (key - 0x309B <= 1) {
        self->vtbl->fn_0x18(self, 27, NULL);
    } else {
        (*(Panel805482CC_ItemVtbl**)item)->fn_0x4C(item, key);
    }

    if (key != 0) {
        self->vtbl->fn_0x17C(self, 10);
    }
    self->field_0x1988 = 1;
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_80548B04(void)
{
}

/* Store the incoming u16 into the sub-object's +0x04 field. */
void fn_80548B08(Panel805482CC* self, u16 value)
{
    self->sub.field_0x04 = value;
}

/*
 * The panel's key handler: pick the object the current mode selects, run the mode-specific key
 * handling, then push the key into the text cursor and report the move.
 *
 * `a` is the key, `b` a flag that selects the "cursor moved" report only, `c`/`d`/`e` the mode
 * parameters the selected object's slots 0xEC / 0x54 / 0x100 take.
 */
void fn_80548B10(Panel805482CC* self, u32 a, s32 b, s32 c, s32 d, s32 e)
{
    Panel805482CC_Item* item;

    if (self->field_0x1950 == 0) {
        item = self->field_0x193C;
    } else if (self->field_0x194C == 1) {
        (*(Panel805482CC_ItemVtbl**)self->field_0x1940)->fn_0xD8(self->field_0x1940);
        item = self->field_0x1940;
    } else {
        (*(Panel805482CC_ItemVtbl**)self->field_0x1944)->fn_0xE4(self->field_0x1944);
        item = self->field_0x1944;
    }

    if (a == 0) {
        return;
    }
    if (b != 0) {
        u32 before = (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x88(
            self->field_0x193C);
        u32 after;

        (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x5C(self->field_0x193C, a);
        after = (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x88(self->field_0x193C);
        if (before != after) {
            fn_805492A8(self, &before, &after);
        }
        return;
    }

    if (item == self->field_0x1944) {
        if ((u16)(*(Panel805482CC_ItemVtbl**)item)->fn_0xF8(item) == 0) {
            (*(Panel805482CC_ItemVtbl**)item)->fn_0xEC(item, c);
            (*(Panel805482CC_ItemVtbl**)item)->fn_0x100_arg(item, e);
        }
        if ((u16)(*(Panel805482CC_ItemVtbl**)item)->fn_0xF8(item) >= 32) {
            self->vtbl->fn_0x18(self, 6, NULL);
            (*(Panel805482CC_ItemVtbl**)item)->fn_0xEC(item, c);
            (*(Panel805482CC_ItemVtbl**)item)->fn_0x5C(item, a);
            self->vtbl->fn_0x17C(self, 9);
            self->vtbl->fn_0xD8(self);
            return;
        }
    } else if (item == self->field_0x1940) {
        if (d != 0 && (*(Panel805482CC_ItemVtbl**)item)->fn_0x54(item) == 0) {
            return;
        }
    } else if (self->field_0x1950 != 0 && self->field_0x194C == 1
               && item != self->field_0x1940 && a == 32) {
        (*(Panel805482CC_ItemVtbl**)item)->fn_0x4C(item, 0);
        a = 12288;
    }

    if (item == self->field_0x193C) {
        u32 before = (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x88(
            self->field_0x193C);
        u32 after;

        (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x5C(self->field_0x193C, a);
        after = (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x88(self->field_0x193C);
        if (before != after) {
            fn_805492A8(self, &before, &after);
        }
    } else {
        (*(Panel805482CC_ItemVtbl**)item)->fn_0x5C(item, a);
    }
    self->vtbl->fn_0x17C(self, 10);
}

/* Whether the sub-object's +0x04 field is non-zero (`(x | -x) >> 31`, retail's zero test). */
s32 fn_80548E2C(Panel805482CC* self)
{
    return self->sub.field_0x04 != 0;
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_80548E20(void)
{
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_80548E24(void)
{
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_80548E28(void)
{
}

/*
 * Re-sync the text box's node list after the object at +0x193C moved its cursor: walk the 8-byte
 * node list at +0x19E4, unlink the node the panel currently points at, then hand the new node and
 * the remaining count to fn_80549570 and refresh the sub-object's two layout slots.
 */
void fn_80548E40(Panel805482CC* self)
{
    u32 before = (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x88(self->field_0x193C);
    u32 after;

    (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x68(self->field_0x193C);
    after = (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x88(self->field_0x193C);
    if (before == after || before == 0) {
        return;
    }

    if (self->field_0x19F8 == 0) {
        Panel805482CC_Cell* cell = self->field_0x19F0;
        Panel805482CC_Cell* node = &self->field_0x19E4[cell->field_0x00];
        u16 spare;
        Panel805482CC_Cell* tail;
        u16 tail_next;

        node->field_0x04 = node->field_0x04 - 1;
        self->field_0x19F8 = node->field_0x04;
        node->field_0x04 = node->field_0x04 + cell->field_0x04;

        spare = self->field_0x19E4[cell->field_0x02].field_0x00;
        self->field_0x19E4[cell->field_0x02].field_0x00 = cell->field_0x00;
        self->field_0x19E4[cell->field_0x00].field_0x02 = cell->field_0x02;
        cell->field_0x00 = spare;
        cell->field_0x02 = spare;

        tail = &self->field_0x19E4[self->field_0x19E4[self->field_0x19E8].field_0x00];
        tail_next = tail->field_0x02;
        cell->field_0x00 = self->field_0x19E4[tail_next].field_0x00;
        cell->field_0x02 = tail_next;
        tail->field_0x02 = spare;
        self->field_0x19E4[tail_next].field_0x00 = spare;

        self->field_0x19F0 = node;
    } else {
        Panel805482CC_Cell* cell = self->field_0x19F0;

        cell->field_0x04 = cell->field_0x04 - 1;
        self->field_0x19F8 = self->field_0x19F8 - 1;
    }

    {
        u32 length = (*(Panel805482CC_ItemVtbl**)self->field_0x193C)->fn_0x88(
            self->field_0x193C);

        fn_80549570(self, self->field_0x19F0, length - self->field_0x19F8);
    }
    self->sub.mpVtbl->fn_0x90(&self->sub);
    self->sub.mpVtbl->fn_0x94(&self->sub);
}

/* Hand the sub-object's layout pass at +0x118 to 0x8054F788, then refresh the sub-object. */
void fn_80549C50(Panel805482CC* self)
{
    self->vtbl->fn_0x18(self, 6, NULL);
    fn_8054F788(&self->field_0x118);
    self->sub.mpVtbl->fn_0x90(&self->sub);
    self->sub.mpVtbl->fn_0x94(&self->sub);
}

/* The object the panel's current mode selects; a non-zero `flag` falls back to +0x193C when the
 * selected object reports itself busy (`fn_0xD8` / `fn_0xE4`). */
Panel805482CC_Item* fn_80549CC0(Panel805482CC* self, s32 flag)
{
    Panel805482CC_Item* item;

    if (self->field_0x1950 == 0) {
        return self->field_0x193C;
    }
    if (self->field_0x194C == 1) {
        item = self->field_0x1940;
        if ((*(Panel805482CC_ItemVtbl**)item)->fn_0xD8(item) != 0 && flag != 0) {
            return self->field_0x193C;
        }
        return self->field_0x1940;
    }
    item = self->field_0x1944;
    if ((*(Panel805482CC_ItemVtbl**)item)->fn_0xE4(item) != 0 && flag != 0) {
        return self->field_0x193C;
    }
    return self->field_0x1944;
}

/* Whether the panel is in mode 1 (`field_0x194C == 1`) while its input byte is set. */
s32 fn_80549D74(Panel805482CC* self)
{
    if (self->field_0x1950 == 0) {
        return 0;
    }
    return self->field_0x194C == 1;
}

/* Forward the +0x18B8 record to 0x80553670 (a tail call in retail). */
void fn_8054A8DC(Panel805482CC* self)
{
    fn_80553670(&self->field_0x18B8);
}

/* Constant-0 accessor. */
s32 fn_8054CA28(void)
{
    return 0;
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_8054CA30(void)
{
}

/* Constant-0 accessor. */
s32 fn_8054CA34(void)
{
    return 0;
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_8054CA3C(void)
{
}

/* Store the incoming word into the panel's +0x1988 field. */
void fn_8054CDBC(Panel805482CC* self, s32 value)
{
    self->field_0x1988 = value;
}

/* Forward the +0x19C4 record to 0x80526D00 (a tail call in retail). */
void fn_8054CDC4(Panel805482CC* self)
{
    fn_80526D00(&self->field_0x19C4);
}

/* Store the incoming byte into the panel's +0x1AB6 field. */
void fn_8054D810(Panel805482CC* self, u8 value)
{
    self->field_0x1AB6 = value;
}

/* Drop the second argument and forward the rest to fn_8054E064 (a tail call in retail). */
void fn_8054DDCC(Panel805482CC* self, s32 a, s32 b, s32 c)
{
    fn_8054E064(self, b, c);
}

/* Forward the +0x1A04 record to 0x8055BEF0 (a tail call in retail). */
void fn_8054E05C(Panel805482CC* self)
{
    fn_8055BEF0(&self->field_0x1A04);
}

/* The u16 at +0x008. */
u16 fn_8054E524(Panel805482CC* self)
{
    return self->field_0x008;
}

/* Dispatch the object at +0x1A18 through its slot 0x14 (a tail call in retail). */
void fn_8054E808(Panel805482CC* self)
{
    Panel805482CC_Item* item = self->field_0x1A18;

    (*(Panel805482CC_ItemVtbl**)item)->fn_0x14(item);
}

/* Reset the +0x1A04 record and then the sub-object's layout at mode 0. */
void fn_8054E81C(Panel805482CC* self)
{
    fn_8055C1D4(&self->field_0x1A04);
    fn_8055A3E0(&self->sub, 0);
}

/* Close the +0x1A04 record and then the sub-object's layout at mode 1. */
void fn_8054E858(Panel805482CC* self)
{
    fn_8055C2CC(&self->field_0x1A04);
    fn_8055A3E0(&self->sub, 1);
}

#ifdef __cplusplus
}
#endif
