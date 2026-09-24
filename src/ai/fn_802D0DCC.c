/*
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 * auto/802D0DCC_fn_802D0DCC.c - one state dispatch over a shared .data table, .text 0x802D0DCC-0x802D0F34.
 *
 * One function, `fn_802D0DCC`, the only symbol in the range.  It switches on the state byte at +0x420 of
 * its argument and tail-calls `fn_802D3398(self, entry)` with one of two adjacent entries of the
 * 16-pointer table `lbl_805D4150` (`.data` 0x805D4150, 0x40 B), chosen by the byte at +0x170.  States 1-6
 * use table slots 2-13, the default uses 14/15.  It calls nothing else and touches no other field of
 * `self`.
 *
 * Flags: `cflags_main` (`auto`, `Wii/1.3`, `-O3 -inline noauto`) reproduces the object exactly - 360 B,
 * 90/90 rows, every relocation equal - so no flag or pragma deviation is needed.
 *
 * The `default` arm is written **first** because that is the layout MWCC emits: the default body sits
 * directly after the `cmpwi`/`beq` chain, while a `default` written last is placed after the case bodies
 * and needs an extra branch to reach it (364 B, 93.73 %).
 *
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object, the
 * symbol map or the runtime dump (`zz_02d0dcc_`, `zz_02d3398_`) names the original file or function, and
 * the neighbours carry no naming scheme.  Same for the callee and the table.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/802D0DCC_fn_802D0DCC.c`.
 */

#include "types.h"

/* The dispatch state object.  Only the two bytes the function reads are named; everything else is
 * padding the disassembly does not describe. */
typedef struct DispatchState {
    u8 pad_0x000[0x170];    /* +0x000 */
    u8 variant;             /* +0x170: selects the odd/even table entry */
    u8 pad_0x171[0x2AF];    /* +0x171 */
    u8 state;               /* +0x420: the switch value */
} DispatchState;            /* size: 0x421 */

/* 16 entries, each handed to the tail-called handler.  Both the table and the handler live outside this
 * unit and have no name in any source, so they are declared here and never defined (playbook 29). */
extern void* const lbl_805D4150[16];
extern void fn_802D3398(DispatchState* self, void* entry);

void fn_802D0DCC(DispatchState* self)
{
    switch (self->state) {
    default:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[15]);
        } else {
            fn_802D3398(self, lbl_805D4150[14]);
        }
        break;
    case 1:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[3]);
        } else {
            fn_802D3398(self, lbl_805D4150[2]);
        }
        break;
    case 2:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[5]);
        } else {
            fn_802D3398(self, lbl_805D4150[4]);
        }
        break;
    case 3:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[7]);
        } else {
            fn_802D3398(self, lbl_805D4150[6]);
        }
        break;
    case 4:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[9]);
        } else {
            fn_802D3398(self, lbl_805D4150[8]);
        }
        break;
    case 5:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[11]);
        } else {
            fn_802D3398(self, lbl_805D4150[10]);
        }
        break;
    case 6:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[13]);
        } else {
            fn_802D3398(self, lbl_805D4150[12]);
        }
        break;
    }
}
