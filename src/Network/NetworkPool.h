/*
 * Network/NetworkPool.h - the declarations of `Network/NetworkPool.cpp`: the `NetworkPool` singleton, its C callbacks
 *   and `NetworkRandom`.
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
 * (0x4001 startup, 0x4002 cleanup, 0x4005 purchase, 0x4006 download), three values and the 12-byte error record,
 * plus that argument.  The mediator's `mediatorEventCallback` has the same shape. */
struct NetworkErrorInfo;
/* untyped: caller-owned payload - the argument the installer passed to `init` */
typedef void (*NetworkPoolCallback)(u32 code, s32 a, s32 b, s32 c, const struct NetworkErrorInfo* error, void* arg);

/* The NHTTP command callbacks the startup step registers (`NHTTPi_RegisterCallbacks`). */
typedef void (*NetworkPoolCommandFn)(u32 command);

/* The 0x38-byte setup record `init` copies: two strings, the timed handler's 64-bit period and the four NHTTP words.
 * Field names GUESSED from where `init` stores them.  size: 0x38 */
typedef struct NetworkPoolConfig {
    /* +0x00 */ char name[0x10];               /* copied to +0x0C, NUL-terminated there */
    /* +0x10 */ u8   pad_10;
    /* +0x11 */ char region[0x0A];             /* copied to +0x1D, NUL-terminated there */
    /* +0x1B */ u8   pad_1B[0x05];
    /* +0x20 */ u64  period;                   /* the timed handler's period (`NetworkTimedHandler::init`) */
    /* +0x28 */ NetworkPoolCommandFn commandCallback;
    /* +0x2C */ NetworkPoolCommandFn commandCallbackEx;
    /* +0x30 */ u32  option_30;
    /* +0x34 */ u32  option_34;
} NetworkPoolConfig;

/* One row of the purchase catalogue (`purchase` looks the item id up in it).  size: 0x18 */
typedef struct NetworkPoolItem {
    /* +0x00 */ s32 id;
    /* +0x04 */ s32 price;                     /* compared against `getPointBalance` */
    /* +0x08 */ u8  pad_08[0x10];
} NetworkPoolItem;

class NetworkTimedHandler;                     /* Network/GameSpyInterfaceThread.h */

/* The 0x3A20-byte network singleton `getNetworkPool` returns (.sbss 0x80794CB8): the constructor 0x80412528
 * stores the table 0x80602490 and publishes itself, and the mediator's forwarding wrappers create it with `new`.
 * It is the game's EC (Wii Shop) client: its startup reads and deletes `<home>/ec.cfg` through the NAND API and
 * brings NHTTP up ("NHTTPStartup(%d)"), its cleanup tears NHTTP down ("NHTTPCleanup"), and its purchase step checks
 * the parental-control shop restriction and the NAND quota; the download, ticket and transfer entry points are
 * empty in retail.  Class name GUESSED from the runtime dump's `GetPool` on the accessor; every method name below
 * is a GUESS from its body.  The table is emitted by `Network/NetworkPool.cpp` (rule 10).  size: 0x3A20 */
class NetworkPool {
public:
    /* 0x80412528 */
    NetworkPool();
    /* +0x08 - 0x80412570 */ virtual ~NetworkPool();

    /* 0x804125E4 / 0x804125DC - reset, then keep the event sink and its argument and copy the setup record; with
     * `timed` set, also create the timed handler at +0x50 from the record's period.  The second passes 0. */
    /* untyped: caller-owned payload - the sink's argument */
    void init(NetworkPoolCallback callback, void* arg, const NetworkPoolConfig* config, s32 timed);
    /* untyped: caller-owned payload - the sink's argument */
    void init(NetworkPoolCallback callback, void* arg, const NetworkPoolConfig* config);
    void reset();
    void clearState();
    void destroySession();
    void update();
    void start();
    void startCleanup();
    void advanceCleanup();
    void handleCheckResult(s32 result);
    void handleOpenResult(s32 result);
    void handleReadResult(s32 result);
    void handleCloseResult(s32 result);
    void handleDeleteResult(s32 result);
    void syncTickets();
    void deleteTicket(s32 itemId);
    void purchase(s32 itemId);
    u8   isConfigRead();
    s32  isShopAvailable();
    s32  setAccount(const char* userId, const char* password);
    s32  getPointBalance(struct NetworkErrorInfo* error);
    s32  isPurchaseRestricted();
    u8   isRestrictionBypassed(s32 kind);
    void launchShopHelp();
    void setTransferTotal(u64 total);
    void finishTransfer();
    void setTransferOption(s32 option, s32 value);
    void selectTransferMode(s32 mode);
    void startDownload();
    void postEvent(u32 code, s32 a, s32 b, s32 c, const struct NetworkErrorInfo* error);
    void stepStartup();
    void stepCleanup();
    void stepPurchase();
    void stepDownload();
    void stepPending();
    /* 0x8041793C (`Network/NetworkWiiMediator.cpp`) - whether NHTTP is up (+0x44). */
    BOOL isECStarted();

    /* +0x0004 */ NetworkPoolCallback callback;
    /* +0x0008 */ void* callbackArg;             /* the sink's argument */
    /* +0x000C */ char  name[0x11];
    /* +0x001D */ char  region[0x0B];
    /* +0x0028 */ u64   period;
    /* +0x0030 */ u8    pad_0030[0x04];
    /* +0x0034 */ NetworkPoolCommandFn commandCallback;
    /* +0x0038 */ NetworkPoolCommandFn commandCallbackEx;
    /* +0x003C */ u32   option_3C;
    /* +0x0040 */ u32   option_40;
    /* +0x0044 */ u8    ec_started;              /* NHTTP is up (the startup's step 1 sets it) */
    /* +0x0045 */ u8    flag_45;
    /* +0x0046 */ u8    flag_46;
    /* +0x0047 */ u8    pad_0047;
    /* +0x0048 */ f32   time_48;
    /* +0x004C */ s32   timed;                   /* `init`'s last argument: skip NHTTP and the config file */
    /* +0x0050 */ NetworkTimedHandler* session;
    /* +0x0054 */ u8    pad_0054[0x04];
    /* +0x0058 */ u64   transfer_total;
    /* +0x0060 */ s32   mode;                    /* 1 purchase, 2 download */
    /* +0x0064 */ s32   pending;                 /* the purchase step leaves 3 here */
    /* +0x0068 */ u8    startup_step;
    /* +0x0069 */ u8    cleanup_step;
    /* +0x006A */ u8    operation_step;
    /* +0x006B */ u8    progress;                /* the opening progress the mediator adds to 90 while step 6 runs */
    /* +0x006C */ s32   item_index;              /* the catalogue row `purchase` accepted */
    /* +0x0070 */ u32   quota_answer;            /* `NANDCheckAsync`'s answer bits */
    /* +0x0074 */ char  path[0x40];              /* the home directory, then "<home>/ec.cfg" */
    /* +0x00B4 */ u8    file_info[0x8C];         /* the SDK's NANDFileInfo */
    /* +0x0140 */ u8    delete_config;           /* the read reported a corrupt file: delete it after closing */
    /* +0x0141 */ u8    pad_0141[0x1F];
    /* +0x0160 */ u8    read_buffer[0x20];
    /* +0x0180 */ u8    command_block[0xB8];     /* the SDK's NANDCommandBlock */
    /* +0x0238 */ u8    pad_0238[0x1A0];
    /* +0x03D8 */ u8    catalog_data[0x2BD8];    /* cleared by `clearState` */
    /* +0x2FB0 */ s32   catalog_status;
    /* +0x2FB4 */ u8    pad_2FB4[0x324];
    /* +0x32D8 */ s32   item_count;
    /* +0x32DC */ NetworkPoolItem items[0x4C];
    /* +0x39FC */ u8    pad_39FC[0x14];
    /* +0x3A10 */ u64   timestamp;               /* the server timestamp the mediator stamps the account with */
    /* +0x3A18 */ u8    restriction_bypassed;    /* skips the parental-control shop check */
    /* +0x3A19 */ u8    config_read;             /* the startup finished, or `ec.cfg` was read */
    /* +0x3A1A */ u8    pad_3A1A[0x06];
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
