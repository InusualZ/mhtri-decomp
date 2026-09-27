/*
 * include/Network/NetworkWiiMediator.h - the class the 0x80413C64..0x804155D4 Network band owns.
 *
 * Reconstructed from the range's own disassembly and the runtime dump's map.  The dump spells the
 * member functions `NetworkWiiMediator::<method>`; the project's `symbols.txt` carries the same names
 * already mangled (`reflectInit__18NetworkWiiMediatorPFllllPvPv_vPv`, `getAccountBan__18NetworkWiiMediatorFPcUl`),
 * so the declarations below are chosen so MWCC mangles them back to those exact spellings.
 *
 * Only the methods the range defines are declared.  They are declared **non-virtual** on purpose:
 * the range's object stores no `.data` vtable (the class's vtable lives in another translation unit),
 * and a class whose virtuals are all defined in its own TU gains a 20-byte `.data` vtable (the same
 * finding the GameSpy interface unit's header records).  `reflectInit`'s first parameter is the callback
 * the retail body stores at +0x78C - the `PFllllPvPv_v` half of the mangled name.
 */
#ifndef NETWORK_WII_MEDIATOR_H
#define NETWORK_WII_MEDIATOR_H

#include "types.h"

typedef void (*NetworkWiiMediatorReflectFn)(s32, s32, s32, s32, void*, void*);

class NetworkWiiMediator {
public:
    /* the class body is 0x800 bytes; every field the range addresses is reached through a documented
     * offset at the call site (the class is only *used*, never constructed, in this band) */
    u8 pad_00[0x800];

    void reflectInit(NetworkWiiMediatorReflectFn callback, void* arg);
    void reflectStart();
    void reflectStop();
    void reflectFinal();
    s32  getOpeningProgress();
    s32  getOpeningTermsVersion();
    s32  isOpeningMaintenanceTerms();
    s32  isOpeningMaintenanceServer();
    s32  isOpeningAnnounce();
    void getAccountBan(char* out, u32 size);
    void getAccountWarning(char* out, u32 size);
    void getAccountWaitQueue(char* out, u32 size);
    void getReflectPage(u8 page);
    void agreeReflect();
};

#endif /* NETWORK_WII_MEDIATOR_H */
