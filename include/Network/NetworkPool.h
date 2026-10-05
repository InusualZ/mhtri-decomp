/*
 * include/Network/NetworkPool.h - the declarations `src/Network/NetworkPool.cpp` owns (`.text` 0x804123F8..0x80413450:
 * the `NetworkPool` singleton, the NHTTP wrappers and `NetworkRandom`).  Moved here from
 * `include/Network/network_layer_io.h` when the round 4 fold gave the tail its own unit (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_NETWORK_NETWORKPOOL_H
#define MHTRI_NETWORK_NETWORKPOOL_H

#include "types.h"

/* The network library's linear-congruential random generator (`sNetworkLibrary::mpRandom`): the constructor
 * 0x80413384 stores the table 0x806024A0 and the classic `rand` constants (seed 1, multiplier 0x41C64E6D,
 * increment 12345, result shift 16, mask 0x7FFF); 0x80413424 steps it.  Class name GUESSED.  size: 0x18 */
class NetworkRandom {
public:
    NetworkRandom();
    /* +0x08 */ virtual ~NetworkRandom();

    /* +0x04 */ u32 seed;
    /* +0x08 */ u32 multiplier;
    /* +0x0C */ u32 increment;
    /* +0x10 */ u32 shift;
    /* +0x14 */ u32 mask;
};

/* The 0x3A20-byte network singleton `getNetworkPool` returns (.sbss 0x80794CB8): the constructor 0x80412528
 * stores the table 0x80602490 and publishes itself, 0x8041275C clears its state (the +0x6B progress byte,
 * the +0x3A10 timestamp among it), and the mediator's forwarding wrappers create it with `new` (0x3A20).
 * Class name GUESSED from the runtime dump's `GetPool` on the accessor.  The virtual is declared and not
 * defined here, so no table is emitted by a consumer (rule 10).  size: 0x3A20 */
class NetworkPool {
public:
    /* +0x08 */ virtual ~NetworkPool();

    /* 0x804128C4 - resets the state and starts the EC (shop) sequence (GUESS name). */
    void start();
    /* 0x8041793C (`Network/NetworkWiiMediator.cpp`) - whether the EC sequence is running (+0x44). */
    BOOL isECStarted();
    /* 0x804127E8 - deletes the session object at +0x50 through its deleting destructor and clears the pointer
     * (GUESS name; the mediator's `deleteNetworkPool` calls it before deleting the pool). */
    void destroySession();
    /* 0x8041283C - the per-frame step: dispatches on the +0x69/+0x68 bytes and the +0x60 mode to the step helpers
     * (GUESS name; the mediator's `update` calls it once per frame). */
    void update();

    /* +0x0004 */ u8  pad_0004[0x40];
    /* +0x0044 */ u8  ec_started;
    /* +0x0045 */ u8  pad_0045[0x26];
    /* +0x006B */ u8  progress;        /* the opening progress the mediator adds to 90 while step 6 runs */
    /* +0x006C */ u8  pad_006C[0x39A4];
    /* +0x3A10 */ u64 timestamp;       /* the server timestamp the mediator stamps the account with */
    /* +0x3A18 */ u8  pad_3A18[0x08];
};

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8041241C - the pool singleton the mediator band forwards to. */
NetworkPool* getNetworkPool(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORKPOOL_H */
