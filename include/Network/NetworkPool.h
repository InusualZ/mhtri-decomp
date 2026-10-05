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
    /* +0x08 - 0x804133C0 */ virtual ~NetworkRandom();
    /* +0x0C - 0x80413404 - stores all five generator words (GUESS name). */
    virtual void setParameters(u32 seed, u32 multiplier, u32 increment, u32 shift, u32 mask);
    /* +0x10 - 0x8041341C - stores the seed (GUESS name). */
    virtual void setSeed(u32 seed);
    /* +0x14 - 0x80413424 - steps the generator (`seed = seed * multiplier + increment`) and returns
     * `(seed >> shift) & mask` (GUESS name). */
    virtual u32 next();

    /* +0x04 */ u32 seed;
    /* +0x08 */ u32 multiplier;
    /* +0x0C */ u32 increment;
    /* +0x10 */ u32 shift;
    /* +0x14 */ u32 mask;
};

/* The event sink `NetworkPool::init` installs (+0x04, its argument at +0x08): `postEvent` hands it the event code
 * (0x4005 / 0x4006 for the two operations' results), three values and the 12-byte error record, plus that argument.
 * The mediator's `mediatorEventCallback` has the same shape. */
struct NetworkErrorInfo;
/* untyped: caller-owned payload - the argument the installer passed to `init` */
typedef void (*NetworkPoolCallback)(u32 code, s32 a, s32 b, s32 c, const struct NetworkErrorInfo* error, void* arg);

/* The 0x3A20-byte network singleton `getNetworkPool` returns (.sbss 0x80794CB8): the constructor 0x80412528
 * stores the table 0x80602490 and publishes itself, 0x8041275C clears its state (the +0x6B progress byte,
 * the +0x3A10 timestamp among it), and the mediator's forwarding wrappers create it with `new` (0x3A20).
 * It is the game's EC (Wii Shop) client: its startup reads and deletes `<home>/ec.cfg` through the NAND API and
 * brings NHTTP up ("NHTTPStartup(%d)"), its cleanup tears NHTTP down ("NHTTPCleanup"), and its purchase step checks
 * the parental-control shop restriction and the NAND quota.  Class name GUESSED from the runtime dump's `GetPool`
 * on the accessor; every method name below is a GUESS from its body.  The virtual is declared and not defined
 * here, so no table is emitted by a consumer (rule 10).  size: 0x3A20 */
class NetworkPool {
public:
    /* 0x80412528 */
    NetworkPool();
    /* +0x08 - 0x80412570 */ virtual ~NetworkPool();

    /* 0x804125E4 / 0x804125DC - reset, then keep the event sink and its argument and copy the 0x38-byte setup record
     * (`config`: a 16-character and a 10-character string and six words); with `timed` set, also create the timed
     * handler at +0x50 from the record's +0x20/+0x24 words.  The second passes `timed` = 0. */
    /* untyped: caller-owned payload - the sink's argument */
    void init(NetworkPoolCallback callback, void* arg, const u8* config, s32 timed);
    /* untyped: caller-owned payload - the sink's argument */
    void init(NetworkPoolCallback callback, void* arg, const u8* config);
    /* 0x804126D8 / 0x8041275C - clear the setup record, then the state (`clearState` alone is what `start` runs). */
    void reset();
    void clearState();
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
    /* 0x80412DC8 / 0x804130EC / 0x804131F0 - the steps `update` runs: the startup sequence (+0x68: NHTTP, then the
     * `ec.cfg` open/read/close/delete), the cleanup sequence (+0x69: `NHTTPDestroy`) and mode 1's purchase step. */
    void stepStartup();
    void stepCleanup();
    void stepPurchase();
    /* 0x80412904 / 0x80412918 - arm the cleanup sequence (+0x69 = 1, +0x68 = 0) / advance it (`onNHTTPDestroyed`). */
    void startCleanup();
    void advanceCleanup();
    /* 0x80412928 / 0x804129D4 / 0x804129F8 / 0x80412A34 / 0x80412A70 - the NAND results the callbacks below forward:
     * the quota check (a nonzero answer posts error 0x80000007/85 and ends the operation), the `ec.cfg` open, read
     * (success raises +0x3A19), close and delete (each advances or ends the startup step). */
    void handleCheckResult(s32 result);
    void handleOpenResult(s32 result);
    void handleReadResult(s32 result);
    void handleCloseResult(s32 result);
    void handleDeleteResult(s32 result);
    /* 0x80412A98 - starts the purchase of catalogue entry `itemId` (the 24-byte rows at +0x32DC): posts 0x4005 with
     * error 0x80000006 while an operation runs, 0x80000003 for an unknown id, 0x80000007/83 when the row's +0x08 word
     * exceeds what the stub 0x80412C18 returns (0); else keeps the row index (+0x6C) and enters mode 1 (+0x60). */
    void purchase(s32 itemId);
    /* 0x80412C00 - whether the `ec.cfg` read succeeded (+0x3A19). */
    u8 isConfigRead();
    /* 0x80412C20 - 0 while +0x3A18 is set, else whether `SCCheckPCShopRestriction` reads 1. */
    s32 isPurchaseRestricted();
    /* 0x80412C68 - a tail call into `OSLaunchShopChannelHelp`. */
    void launchShopHelp();
    /* 0x80412DA0 - calls the event sink with the code, the three values, the error record and the sink's argument. */
    void postEvent(u32 code, s32 a, s32 b, s32 c, const struct NetworkErrorInfo* error);

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

/* The pool's C callbacks, each forwarding to the singleton (GUESS names): 0x804123F8 is the `NHTTPDestroy`
 * completion (`advanceCleanup`); 0x80412424 / 0x80412458 / 0x8041248C / 0x804124C0 / 0x804124F4 are the
 * completions of `NANDCheckAsync`, `NANDOpenAsync`, `NANDReadAsync`, `NANDCloseAsync` and `NANDDeleteAsync`
 * (`handleCheckResult` .. `handleDeleteResult` with the result). */
struct NANDCommandBlock;
void onNHTTPDestroyed(void);
void onNANDCheckDone(s32 result, struct NANDCommandBlock* block);
void onConfigOpened(s32 result, struct NANDCommandBlock* block);
void onConfigRead(s32 result, struct NANDCommandBlock* block);
void onConfigClosed(s32 result, struct NANDCommandBlock* block);
void onConfigDeleted(s32 result, struct NANDCommandBlock* block);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORKPOOL_H */
