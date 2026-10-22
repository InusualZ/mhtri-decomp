/*
 * menu/PatTerms.h - leaf header (rule 2) for the terms object `menu/menu_plsearch.cpp` defines: its constructor
 *   0x804504F4 (stores the table 0x80607E40, publishes the object in `.sbss` 0x80794D58, clears its state), its
 *   destructor 0x80450534 (unpublishes it) and its table.  `NetworkWiiMediator` embeds it at +0x6DD8.
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

    /* +0x04 */ u8* buffer_0x04;            /* the P-Mic work area `initPatTerms` hands over (PMICInit) */
    /* +0x08 */ u32 buffer_size_0x08;       /* its size; 0x3E00 of it is the P-Mic library's */
    /* +0x0C */ u8  state_0x0C;
    /* +0x0D */ u8  ready_0x0D;
    /* +0x0E */ u8  check_request_0x0E;     /* raised by `requestPatTermsCheck` */
    /* +0x0F */ u8  update_request_0x0F;    /* raised by `requestPatTermsUpdate` */
    /* +0x10 */ u8  cancel_request_0x10;    /* raised by `cancelPatTermsUpdate` */
    /* +0x11 */ u8  pad_0x11[0x3];
    /* +0x14 */ s32 async_result_0x14;      /* the last P-Mic request's result (1 while it is in flight) */
    /* +0x18 */ s16 voice_out_0x18[0x60];   /* the playback samples `writePatTermsVoice` hands to PMICWrite */
    /* +0xD8 */ s32 reserved_0xD8;          /* set to 1 by the state reset; nothing reads it */
    /* +0xDC */ s32 echo_reset_0xDC;        /* the echo suppressor's reset argument (1 until it first succeeds) */
    /* +0xE0 */ u16 progress_0xE0;          /* the high half of the P-Mic parameter word PMICGetParam fills */
    /* +0xE2 */ u8  flag_0xE2;
    /* +0xE3 */ u8  pad_0xE3[0x5];
    /* +0xE8 */ u8  thread_0xE8[0x318];     /* the `OSThread` that runs PMICEnd */
    /* +0x400 */ u8 stack_0x400[0x8000];    /* that thread's stack */
};

#endif /* MHTRI_MENU_PATTERMS_H */
