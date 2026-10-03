/* Declarations owned by `src/Network/net_session_close.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_NETWORK_NET_SESSION_CLOSE_H
#define MHTRI_NETWORK_NET_SESSION_CLOSE_H

#include "types.h"
#include "Network/network_transport.h"

/* Declarations moved here from `include/unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
typedef struct NetId NetId;                 /* include/Network/NetworkLayerPat.h */
struct NetRosterSync;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804343C4 - starts the network session's close-down sequence (0 when there is no session manager or one
 * is already closing, 1 once started); 0x80434668 - its progress: -1 on error, 1 when done or when there is
 * no manager, else 0.  No registered unit owns the addresses, so the band is their home (rule 2).  Added
 * with `quest/arenatask.cpp`; the names are GUESSES from the bodies. */
s32 net_session_close_start(void);

s32 net_session_close_state_get(void);

/* 0x80437040 - renders a network id as text into `out` (a `%` in the id prints as `*`). */
void formatNetId(char* out, const NetId* id);

/* 0x804370BC - imports a network id from `src` into `dst`. */
void importNetId(NetId* dst, const NetId* src);

/* 0x804344DC.. - the handlers of the actions queued in the work record's action byte (1, 4, 5, 6, 7, 8). */
void runPendingAction1(void);

void runPendingAction5(void);

void runPendingAction6(void);

void runPendingAction7(void);

void runPendingAction8(void);

/* 0x804353A4 / 0x804356D0 / 0x80435BAC / 0x80435D1C - the friend roster sync helpers. */
void buildRosterSync(struct NetRosterSync* sync);

void flushRosterSync(void);

void refreshRosterCache(void);

void startRosterFetch(s32 mode);

/* 0x80433384 / 0x8043339C / 0x80434FB4 / 0x80433B0C - the shutdown phase slots and the friend slot lookup. */
void clearPhaseSlot(s32 slot);

u32 isPhaseSlotDone(s32 slot);

s8 lookupFriendSlot(const u8* id);

void startShutdownTimer(void);

/* 0x80433870 - reads the link state; 1 = the network link is up. */
u32 getLinkStatus(void);

/* 0x804344A0 - whether the session-start request has completed (`net_ctrl_wk` +0xC153, set by the start
 * request's completion callback and cleared by `net_session_close_start`).  GUESS name from those writers. */
u32 isSessionStartDone(void);

/* 0x80434800 - drops the session after a link loss: clears `net_ctrl_wk` +0x98 and raises pending action 8
 * (`runPendingAction8`); returns 0 when there is no work record.  GUESS name from its callers, which are the
 * link-loss exits of the arena and the lobby. */
s32 net_session_abort_start(void);

/* 0x804333B0 - resets the link state the reconnect path relies on. */
void resetLinkState(void);

/* 0x804370CC - whether two network ids are equal (1). */
u32 isSameNetId(const NetId* left, const NetId* right);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NET_SESSION_CLOSE_H */
