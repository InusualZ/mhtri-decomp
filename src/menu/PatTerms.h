/*
 * menu/PatTerms.h - leaf header (docs/plan.md 6.5 rule 2) for the terms object `menu/menu_plsearch.cpp`
 * defines: its constructor 0x804504F4 (stores the table 0x80607E40, publishes the object in `.sbss` 0x80794D58 and
 * clears its state), its destructor 0x80450534 (unpublishes it) and its table.  Moved here from
 * `Network/network_pat_control.h` when `NetworkWiiMediator` embedded it as a member (+0x6DD8), so the
 * mediator's header can see the complete type without the pat-control band's.
 */
#ifndef MHTRI_MENU_PATTERMS_H
#define MHTRI_MENU_PATTERMS_H

#include "types.h"

/* The terms object `getPatTerms` hands out: the state byte the band compares (20 = check finished, 11 = update
 * finished), the ready byte `initPatTerms` sets and `requestPatTermsCheck` clears (`isPatTermsReady`, and the
 * update requests act only while it is set), the progress count the mediator grades against its threshold table
 * (`getPatTermsProgress`) and the byte the mediator's `setMediatorTermsFlag`/`getMediatorTermsFlag` pass through.
 * The names past `state_0x0C` are GUESSes from those bodies (`Network/NetworkWiiMediator.cpp`, `menu/menu_plsearch.cpp`).
 * A polymorphic class: the constructor stores its table (a virtual destructor alone) at +0x00.  Its members are
 * declared, never defined here, so no consumer emits the table (rule 10).
 * size: 0x8400 (the mediator reserves 0x6DD8..0xF1D8 for its member) */
class PatTerms {
public:
    PatTerms();
    virtual ~PatTerms();

    /* +0x04 */ u8  pad_0x04[0x8];
    /* +0x0C */ u8  state_0x0C;
    /* +0x0D */ u8  ready_0x0D;
    /* +0x0E */ u8  pad_0x0E[0xD2];
    /* +0xE0 */ u16 progress_0xE0;
    /* +0xE2 */ u8  flag_0xE2;
    /* +0xE3 */ u8  pad_0xE3[0x831D];
};

#endif /* MHTRI_MENU_PATTERMS_H */
