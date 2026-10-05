/*
 * Network/NetworkWiiMediator.h - the class the 0x80413450..0x80417BC0 Network band owns (the round 3 fold of
 * the mediator head, the class band and the former `Network/network_opening.cpp`, whose header folded in here).
 *
 * Reconstructed from the range's own disassembly and the runtime dump's map.  The dump spells the
 * member functions `NetworkWiiMediator::<method>`; the project's `symbols.txt` carries the same names
 * already mangled (`reflectInit__18NetworkWiiMediatorPFllllPvPv_vPv`, `getAccountBan__18NetworkWiiMediatorFPcUl`),
 * so the declarations below are chosen so MWCC mangles them back to those exact spellings.
 *
 * The two polymorphic classes are `NetworkMediator` (table 0x80602978) and `NetworkWiiMediator` (table
 * 0x80602968): the unit defines both constructors and destructors and `NetworkWiiMediator::update`, so it emits
 * both tables (rule 10).  The plain member functions are non-virtual.  `reflectInit`'s first parameter is the
 * callback the retail body stores at +0x78C - the `PFllllPvPv_v` half of the mangled name.
 */
#ifndef NETWORK_WII_MEDIATOR_H
#define NETWORK_WII_MEDIATOR_H

#include "types.h"
#include "Network/NetworkReflectService.h"   /* NetworkWiiMediatorReflectFn - the reflect service's callback type */
#include "Network/gamespy_interface_types.h"
#include "DWCi/dwc_nasfunc.h"               /* DWCSvlResult - owner DWCi/dwc_nasfunc.cpp */
#include "menu/PatTerms.h"                  /* PatTerms - the member at +0x6DD8 (owner menu/menu_plsearch.cpp) */

/* The mediator's field layout, traced from the disassembly: every offset below is one an instruction
 * in this unit addresses.  `buffer_A`/`buffer_B` are the two 0x106-byte blocks the accessors copy,
 * `reflect_page` the 0x400-byte page `getReflectPageBuffer` hands out, `line_table_a`/`line_table_b`
 * the two 1024-entry pointer arrays `parseReflectLines` clears and fills, and `line_table_c` the
 * one-entry array of its "single line" mode.  `pat_*` are the four words `initializeNetworkMediator`
 * and `updateOpeningState` keep the two `setPatRange` slots' address/length pairs in.  From +0x2BA8 on the
 * fields are the opening part's (the former `Network/network_opening.cpp`): the NAS token, the game-info copy and the
 * four transfer slots, each with its queue (the constructor 0x80413480 clears every one of them).  `pad_*`
 * is a gap no function touches. */
/* One of the mediator's four transfer slots (`openMediatorTransferSlot` fills it).  size: 0x08 */
typedef struct NetworkWiiMediatorTransferSlot {
    /* +0x00 */ u8  active;
    /* +0x01 */ u8  mode;        /* 0 drops the slot's queue (`setMediatorTransferSlotMode`) */
    /* +0x02 */ u8  flag;
    /* +0x03 */ u8  pad_03;
    /* +0x04 */ f32 level;
} NetworkWiiMediatorTransferSlot;

/* The mediator interface `sNetworkLibrary` keeps (`mpMediator`) and the per-frame Pat update drives (its +0x0C
 * slot): the table 0x80602978 holds the destructor 0x804136D0 and an empty (pure) +0x0C slot, the constructor
 * 0x80413714 only stores it.  Class name GUESSED from the library pair `sNetworkLibrary` / `sNetworkLibraryWii`:
 * the platform mediator derives from it the same way.  size: 0x04 */
class NetworkMediator {
public:
    NetworkMediator();
    /* +0x08 */ virtual ~NetworkMediator();
    /* +0x0C (GUESS: the per-frame step `updateNetworkPat` dispatches) */ virtual void update() = 0;
};


/* The mediator singleton (table 0x80602968: the destructor 0x80413724 and `update` 0x804138EC).  Its fields are
 * traced from the disassembly: every offset below is one an instruction in this unit addresses, and the
 * constructor 0x80413480 clears every one of them. */
class NetworkWiiMediator : public NetworkMediator {
public:
    NetworkWiiMediator();
    virtual ~NetworkWiiMediator();
    virtual void update();

    void reflectInit(NetworkWiiMediatorReflectFn callback, void* arg);
    void reflectStart();
    void reflectStop();
    void reflectFinal();
    s32  getOpeningProgress();
    s32  getOpeningTermsVersion();
    s32  isOpeningMaintenanceTerms();
    s32  isOpeningMaintenanceServer();
    s32  isOpeningAnnounce();
    char* getAccountBan(char* out, u32 size);
    char* getAccountWarning(char* out, u32 size);
    char* getAccountWaitQueue(char* out, u32 size);
    void getReflectPage(u8 page);
    void agreeReflect();

    /* the opening part, 0x804155D4..0x80417BC0 (names GUESSED from the bodies) */
    void deleteNetworkPool();
    void ECStart();
    char* getReflectName(char* out, u32 size);
    void openTransferSlot(s8 slot, u8 mode, f32 level);
    void closeTransferSlot(s8 slot);
    void closeTransferSlots();
    void clearTransferQueue(s8 slot);
    s32  pushTransferRecord(s8 slot, const u8* data, s32 size);
    s32  popTransferRecord(s8 slot, u8* out, s32 max);
    /* 0x80416DB0 / 0x804171A4 / 0x80417294 - the microphone side the layer's `move` drives (GUESS names): read up
     * to `size` bytes of samples from the terms (P-Mic) object while the transfer mode is on, whether `size` bytes
     * of samples average below the silence floor, and whether the terms object's update has finished. */
    s32  readVoice(u8* out, s32 size);
    s32  isVoiceSilent(const s16* samples, s32 size);
    u8   isVoiceReady();
    u8   getTransferFlag6DD1();
    u8   getTransferFlag6DD2();
    f32  getTransferLevel();
    void setTransferSlotMode(s8 slot, u8 mode);
    BOOL getTransferSlotMode(s8 slot);
    void setTransferSlotFlag(s8 slot, u8 flag);
    u8   getTransferSlotFlag(s8 slot);
    BOOL isTransferSlotReady(s8 slot);
    void openingStart();
    void openingStop();
    void applyEvent(s32 code, s32 a, s32 b, s32 c, const GameSpyEventMsg* msg);

    /* +0x004 */ u8  cleared_004[4];          /* +0x04..+0x17: five words only the constructor touches (GUESS names) */
    /* +0x008 */ u8  cleared_008[4];
    /* +0x00C */ u8  cleared_00C[4];
    /* +0x010 */ u8  cleared_010[4];
    /* +0x014 */ u8  cleared_014[4];
    /* +0x018 */ u32 event_flags;             /* `applyEvent` ORs the Pat events in: 1 error, 2 refused, 8 shut, 0x10 done */
    /* +0x01C */ u8  flag_1C;
    /* +0x01D */ u8  flag_1D;
    /* +0x01E */ u8  flag_1E;
    /* +0x01F */ u8  flag_1F;
    /* +0x020 */ u8  library_started;         /* `openingStart` sets it once the library's `start` succeeded */
    /* +0x021 */ u8  flag_21;
    /* +0x022 */ u8  pad_022[0x02];
    /* +0x024 */ u32 field_24;
    /* +0x028 */ u32 field_28;
    /* +0x02C */ u32 field_2C;
    /* +0x030 */ u32 field_30;
    /* +0x034 */ u32 field_34;
    /* +0x038 */ u32 field_38;
    /* +0x03C */ u8  reflect_name_3C[0x20];
    /* +0x05C */ u8  reflect_name_5C[0x20];
    /* +0x07C */ u8  buffer_A[0x106];
    /* +0x182 */ u8  buffer_B[0x106];
    /* +0x288 */ u8  field_288[0x402];        /* the block `getMediatorField288` hands out (the constructor clears 0x402) */
    /* +0x68A */ u8  flag_68A;
    /* +0x68B */ u8  name_buffer_68B[0x100];
    /* +0x78B */ u8  flag_78B;
    /* +0x78C */ u8  flag_78C;
    /* +0x78D */ u8  reflect_page[0x400];
    /* +0xB8D */ u8  pad_B8D[0x03];
    /* +0xB90 */ u32 pat_terms_ptr;           /* the constructor points it at `terms_default` */
    /* +0xB94 */ u32 pat_maintenance_ptr;     /* the constructor points it at `maintenance_default` */
    /* +0xB98 */ u32 pat_terms_size;
    /* +0xB9C */ u32 pat_maintenance_size;
    /* +0xBA0 */ u8  terms_default;           /* the one-byte (empty) terms text the pointers start at */
    /* +0xBA1 */ u8  maintenance_default;     /* the one-byte (empty) maintenance text */
    /* +0xBA2 */ u8  single_line_buffer[0x02];
    /* +0xBA4 */ char* line_table_a[0x400];
    /* +0x1BA4 */ char* line_table_b[0x400];
    /* +0x2BA4 */ char* line_table_c[0x01];
    /* +0x2BA8 */ DWCSvlResult svl_result;      /* the NAS service-locator result `openingStart` waits for */
    /* +0x2D1C */ u32 game_info[9];              /* the DWC game-info words `setGameInfo2d1c` keeps */
    /* +0x2D40 */ char game_info_name[0x11];     /* game_info[3] points here (16 chars + NUL) */
    /* +0x2D51 */ char game_info_secret[0x11];   /* game_info[6] points here (16 chars + NUL) */
    /* +0x2D62 */ u16 game_info_title[0x1A];     /* game_info[7] points here (25 UTF-16 units + NUL) */
    /* +0x2D96 */ u8  transfer_queue[4 * 0x1000]; /* 0x1000 per slot: records of a u16 length and its bytes */
    /* +0x6D96 */ u8  pad_6D96[0x02];
    /* +0x6D98 */ s32 transfer_queued[4];        /* bytes held in each slot's queue */
    /* +0x6DA8 */ u8  transfer_activity[4];      /* per slot: polls with data (high nibble) and the ready flag */
    /* +0x6DAC */ s8  transfer_timer[4];         /* per slot: polls left before the activity is re-judged */
    /* +0x6DB0 */ NetworkWiiMediatorTransferSlot transfer_slots[4];
    /* +0x6DD0 */ u8  transfer_mode;             /* `setMediatorTransferMode`; 0 disables the terms queries */
    /* +0x6DD1 */ u8  transfer_flag_6DD1;
    /* +0x6DD2 */ u8  transfer_flag_6DD2;
    /* +0x6DD3 */ u8  pad_6DD3;
    /* +0x6DD4 */ f32 transfer_level;
    /* +0x6DD8 */ PatTerms terms;                /* the terms object (`getPatTerms` publishes it) */
};   /* size: 0xF1D8 (the allocation `sNetworkLibraryWii::init` makes) */



#ifdef __cplusplus
extern "C" {
#endif

/* The mediator's plain-named entry points (each takes the singleton `getInstance` returns, as its field
 * record); the parameter lists are this unit's own definitions'. */
void resetMediatorState(NetworkWiiMediator* self);
void resetMediatorFlags(NetworkWiiMediator* self);
void resetMediatorFlag1D(NetworkWiiMediator* self);
void updateOpeningState(NetworkWiiMediator* self, u32 slot, u32 address, u32 size);
void updateTermVersion(NetworkWiiMediator* self, u32 value);
void updatePatInterface180(NetworkWiiMediator* self, u32 a, u32 b, u32 c);
void loadPatInterfaceBuffers();
s32 queryOpeningFlag278(NetworkWiiMediator* self);
s32 queryOpeningFlag2A8(NetworkWiiMediator* self);
s32 queryOpeningFlag2C0(NetworkWiiMediator* self);
void setReflectPageRange(NetworkWiiMediator* self, u32 address, u32 size);
void setReflectField30(NetworkWiiMediator* self, u32 value);
void setReflectField34(NetworkWiiMediator* self, u32 value);
void setReflectField38(NetworkWiiMediator* self, u32 value);
void setReflectName3C(NetworkWiiMediator* self, char* name);
void setReflectName5C(NetworkWiiMediator* self, char* name);
s32 dispatchReflectEvent();
s32 getWarningUInt(NetworkWiiMediator* self);
char* getAccountName(NetworkWiiMediator* self, char* out, u32 size);
/* the mediator state byte at +0x68A (the session state machine sets and reads it) */
void setMediatorState68A(NetworkWiiMediator* self, u8 value);
/* 0x8041517C - splits reflect text source `source` (0..2) into the line tables */
void parseReflectLines(NetworkWiiMediator* self, s32 source);
void getMediatorState68A(NetworkWiiMediator* self, u8* out);

/* The mediator's mirrors of the `PatInterface` state: the singleton's constructor reads them (the reflect page range,
 * fields and names, the reply buffer, the two server blocks, the flags and the ticket) and its destructor writes the
 * server blocks and the flags back.  Moved here from `src/Network/NetworkWiiMediator.cpp` when the constructor was
 * written (docs/plan.md 6.5 rule 2: the owner declares).  `getReflectPageBuffer` is the retired
 * `NetworkWiiMediator.c` symbol, carried across verbatim. */
void getReflectPageBuffer(char* self, char** subobject, unsigned int* limit);
void setMediatorBufferA(NetworkWiiMediator* self, const u8* src);
void getMediatorBufferA(NetworkWiiMediator* self, u8* dst);
void setMediatorBufferB(NetworkWiiMediator* self, const u8* src);
void getMediatorBufferB(NetworkWiiMediator* self, u8* dst);
void getMediatorNameBuffer(NetworkWiiMediator* self, u32* out1, u8* out2);
void setMediatorFlag78B(NetworkWiiMediator* self, u8 value);
void getMediatorField288(NetworkWiiMediator* self, u32* out);
void setMediatorFlag78C(NetworkWiiMediator* self, u8 value);
void getMediatorFlag78C(NetworkWiiMediator* self, u8* out);
void getReflectPageRange(NetworkWiiMediator* self, u32* out1, u32* out2);
void getReflectField30(NetworkWiiMediator* self, u32* out);
void getReflectField34(NetworkWiiMediator* self, u32* out);
void getReflectField38(NetworkWiiMediator* self, u32* out);
void getReflectName3C(NetworkWiiMediator* self, char* out, u32 size);
void getReflectName5C(NetworkWiiMediator* self, char* out, u32 size);

#ifdef __cplusplus
}
#endif


/* The opening part's and the head's free functions (moved here from `Network/network_opening.h` and
 * `Network/network_layer_io.h` by the round 3 fold; docs/plan.md 6.5 rule 2: the owner declares). */
struct PatTerms;
class PatInterface;                               /* Network/PatInterface.h */
typedef PatInterface NetworkInstance;             /* unsplit/Network.h: the band's alias */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80413450 - the Pat interface callback the mediator installs (slot 1): forwards the event to the
 * mediator passed as the callback argument (GUESS name). */
void mediatorEventCallback(u32 code, s32 a, s32 b, s32 c, const union GameSpyEventMsg* msg,
                           NetworkWiiMediator* mediator);
/* The reflect service singleton the mediator band forwards to and the language/event queries it reads. */
NetworkReflectService* getReflectService(void);
/* 0x80413AEC - whether the console's parental controls restrict messaging (`SCCheckPCMessageRestriction` reads 1);
 * the network control aborts its start when it does.  Renamed from `isMaintenanceMode` (GUESS, from that callee). */
s32 isMessageRestricted(NetworkWiiMediator* self);
/* 0x80413B18 - the link check: fills `error` with 0x80000008/93 while `flag_21` forces a link error
 * (`enableMediatorLinkError`), else reads the interface's link state (`SOGetInterfaceOpt`, level 0xFFFE, option
 * 0x1005) and fills `error` when that read fails; 1 when the link is up.  NAME (a GUESS from the body; the Pat
 * interface's step calls it before each send). */
s32 checkMediatorLink(NetworkWiiMediator* self, struct NetworkErrorInfo* error);
/* 0x80413BF8 - the console's country code (`SCGetCountryCode`, IPL.SADR's first byte), 0 without one; renamed from
 * `getReflectEventId` (GUESS) - `dispatchReflectEvent` maps the codes it returns (18 Canada, 65.. Europe) to a
 * region index. */
s32 getCountryCode(void);
s32 getLanguage(void);
/* 0x80414C54 - copies the Pat interface's media version string (up to `size` bytes) into `out`. */
void getMediaVersionString(NetworkWiiMediator* self, char* out, u32 size);

u32 isTermsUpdateFinished(struct PatTerms* terms);

/* 0x804166E0 / 0x80416620 - copy the mediator's nine DWC game-info words out / in (strings are copied
 * into the mediator's own buffers on the way in). */
void getGameInfo2d1c(NetworkWiiMediator* self, u32* out);
void setGameInfo2d1c(NetworkWiiMediator* self, u32* info);

/* 0x80416890 / 0x8041690C / 0x8041693C - start the terms check / start and cancel the terms update on the terms
 * object, when there is one. */
void startTermsCheck(NetworkWiiMediator* self);
void startTermsUpdate(NetworkWiiMediator* self);
void cancelTermsUpdate(NetworkWiiMediator* self);

/* 0x80416A2C / 0x80416A30 - the terms progress graded against the 17-step threshold table (0..16; 0 with no
 * terms object), the first a forwarding thunk; 0x80416B58 the raw progress count. */
s32 getTermsProgressLevel(NetworkWiiMediator* self);
s32 getMediatorTermsProgressLevel(NetworkWiiMediator* self);
u16 getMediatorTermsProgress(NetworkWiiMediator* self);
/* 0x80416B90 / 0x80416BD8 - store / read the terms object's +0xE2 byte (0 with no terms object). */
void setMediatorTermsFlag(NetworkWiiMediator* self, u32 flag);
u8 getMediatorTermsFlag(NetworkWiiMediator* self);

/* 0x804168F8 / 0x80416BD0 / 0x80416C10 - the terms object's ready byte (as 0/1) and its +0xE2 byte. */
u32 isPatTermsReady(struct PatTerms* terms);
void setPatTermsFlag(struct PatTerms* terms, u32 flag);
u8 getPatTermsFlag(struct PatTerms* terms);

/* The mediator's terms and transfer-state entry points the network pat control drives (GUESS names from the
 * bodies; the fields are the mediator's +0x6DD0 mode byte, the +0x6DD1/+0x6DD2 flag bytes and the +0x6DD4
 * level float):
 *  - 0x80416800 hands the terms object its buffer, resets the per-slot state and sets mode 1, flags 0 and the
 *    default level;
 *  - 0x804168C0 the terms object's status (0 when there is none);
 *  - 0x8041696C stores the transfer mode, resetting the four slots when it changes;
 *  - 0x804172CC / 0x804172DC / 0x804172EC store the +0x6DD1 flag, the +0x6DD2 flag and the level;
 *  - 0x80415FAC posts the community profile's record through the opening step (modes 0 and 2), nonzero
 *    when either accepted it. */
/* untyped: byte range - the MEM2 buffer handed to the terms object */
void initMediatorTerms(NetworkWiiMediator* self, void* buffer, u32 size);
s32 getMediatorTermsStatus(NetworkWiiMediator* self);
void setMediatorTransferMode(NetworkWiiMediator* self, u32 mode);
/* 0x804169D4 - 0 while the transfer mode is 0 or no terms object exists, else `isTermsUpdateFinished` on it
 * (GUESS name). */
s32 isMediatorTermsUpdateFinished(NetworkWiiMediator* self);
void setMediatorTransferFlag6DD1(NetworkWiiMediator* self, u8 flag);
void setMediatorTransferFlag6DD2(NetworkWiiMediator* self, u8 flag);
void setMediatorTransferLevel(NetworkWiiMediator* self, f32 level);
s32 postMediatorRecord(NetworkWiiMediator* self, u8* record);

/* The NAS login token and the user id/password paths the session state machine hands the opening
 * (`Network/network_state.cpp` passes the mediator singleton). */
char* getNASToken(NetworkWiiMediator* self);
void setConnectionPaths(NetworkInstance* connection, const char* userId, const char* password);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_WII_MEDIATOR_H */
