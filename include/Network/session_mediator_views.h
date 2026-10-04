/*
 * include/Network/session_mediator_views.h - the Pat session units' interim views of three mediator-side
 * records: `PatInterface`, `GameSpyInterfaceThread` and `NetworkErrorInfo`, plus the two no-argument
 * `PatInterface_*` spellings those units call.
 *
 * Included only by the units that use them (`Network/NetworkSessionManager.cpp`,
 * `Network/NetworkSessionManagerPat.cpp`, `Network/NetworkLayerPatStep.cpp`), so the transport and
 * game-control units that include `Network/NetworkSessionManager.h` no longer see mediator types.  Moved
 * here unchanged from that header (2026-10-03).
 *
 * EACH VIEW DISAGREES WITH ITS OWNER'S DEFINITION, and unifying it changes these units' code (measured
 * against the target, so it is the bodies' pass, not a header move):
 *   - `PatInterface` (owner header `Network/PatInterface.h`, 0xD640 bytes): this view has no fields, so
 *     `new PatInterface()` in `NetworkSessionManagerPat::NetworkSessionManagerPat`/`::init` allocates 4
 *     where the target allocates 0xD640 (`lis r3,0x1` / `subi r3,r3,0x29c0`); its virtual is named
 *     `destroy(u32)` where the owner's is `finalize(s32)` (the same +0x08 slot), and it adds
 *     `static getInstance()`.
 *   - `GameSpyInterfaceThread` (owner header `Network/gamespy_interface_types.h`, 0x44A0 bytes): no
 *     fields, so `new GameSpyInterfaceThread()` allocates 4 where the target allocates 0x44A0
 *     (`li r3,0x44a0`); `destroy(u32)` stands for the owner's virtual destructor, and `canClose`
 *     (void vs s32) and `requestClose` (bool vs u8) return different types.
 *   - `NetworkErrorInfo` (owner `Network/gamespy_interface_types.h`: `s32 code_00, param1_04, param2_08,
 *     reported_0C`): this view's words are `u32` and named `value_00, code_04, extra_08, pad_0C`, so
 *     `NetworkSessionManagerPat::move` compares +0x04 with `cmplwi` where the target has `cmpwi r0,75`.
 *   - `PatInterface_clear`/`PatInterface_isReady` take the singleton in r3 (`Network/PatInterface.h`);
 *     `NetworkSessionManagerPat::release` calls them with no argument, where the target passes
 *     `getInstance_()` to each (`bl getInstance_` / `bl PatInterface_clear`).
 */
#ifndef MHTRI_NETWORK_SESSION_MEDIATOR_VIEWS_H
#define MHTRI_NETWORK_SESSION_MEDIATOR_VIEWS_H

#include "types.h"
#include "Network/sGameSpyInterfaceThread.h"   /* sGameSpyInterfaceThread - owner Network/GameSpyInterfaceThread.cpp */

/* the Pat interface singletons the Pat methods build on demand (another band) */
class PatInterface {
public:
    virtual void destroy(u32 flags);   /* +0x08 - the key function, defined in the Pat band */
    PatInterface();
    static PatInterface* getInstance();
};
/* The error record `GameSpyInterfaceThread::getErrorStruct` fills and `NetworkInstance::postError`
 * (declared in `include/unsplit/Network.h`, which only forward-declares this type) takes back.  It is
 * declared here beside that handshake, from the Pat band's `move`: it reads +0x04 as the error
 * **code** (it forwards the record only when it is 0x4B) and copies the three words
 * +0x00/+0x04/+0x08 into its own copy, so only those three are named; the tail is untouched anywhere
 * and is padding.  size: 0x10 (approximate - only +0x00..+0x0B is evidenced). */
struct NetworkErrorInfo {
    /* +0x00 */ s32 value_00;
    /* +0x04 */ s32 code_04;
    /* +0x08 */ s32 extra_08;
};   /* size: 0xC - `NetworkSessionManagerPat::move`'s frame gives the record 12 bytes (0x14..0x20) and compares
        the code signed (`cmpwi r0,75`) */

class GameSpyInterfaceThread {
public:
    virtual void destroy(u32 flags);   /* +0x08 - the key function, defined in the Pat band */
    GameSpyInterfaceThread();
    /* the live worker thread (`.sbss` 0x80794CE4); defined in `Network/NetworkSessionManager.cpp` */
    static GameSpyInterfaceThread* getInstance();
    void canClose();
    s32 initialize();
    void armCancel();
    bool requestClose();
    /* The error handshake `move` runs (all three are plain members - the target calls them by their
     * own mangling, not through the table): the result the thread finished with (negative = error),
     * the error record it filled in, and the acknowledgement that clears it. */
    s32 getResult();
    void getErrorStruct(NetworkErrorInfo* info);
    void clearError();
};

/* the Pat accessors keep their plain (unmangled) map names.

   The singleton accessor at 0x803768F0 and `memset` are deliberately **not** declared here: both are
   owned by another registered unit (`getInstance_` sits inside `enemy/em020_ai.cpp`'s range,
   `memset` is `Runtime.PPCEABI.H/memset.c`), and rule 2 puts the declaration in the owner's header -
   which the consumer includes (`enemy/em020_ai.h`, `Runtime.PPCEABI.H/memset.h`). */
extern "C" void PatInterface_clear(void);
extern "C" int PatInterface_isReady(void);

#endif /* MHTRI_NETWORK_SESSION_MEDIATOR_VIEWS_H */
