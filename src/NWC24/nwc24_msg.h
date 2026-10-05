/*
 * NWC24/nwc24_msg.h - the NWC24 message-library half (`src/NWC24/nwc24_msg.c`, `.text`
 * 0x8051D710..0x8051E068).
 *
 * Rule 2: this unit owns the message-library state API and the request engine, so
 * `NWC24/nwc24_io.c` includes this header instead of declaring them.  The library's shared work
 * block and device-path literals have no owner and live in `unsplit/NWC24.h`, which this
 * file includes.
 */
#ifndef MHTRI_NWC24_NWC24_MSG_H
#define MHTRI_NWC24_NWC24_MSG_H

#include "types.h"
#include "unsplit/NWC24.h"

#ifdef __cplusplus
extern "C" {
#endif

/* the library's four open states (see `sMsgLibOpenState` in the unit's source) */
void NWC24iRegisterVersion(void);
int NWC24IsMsgLibOpened(void);
int NWC24IsMsgLibOpenedByTool(void);
int NWC24iIsMsgLibOpenBlocked(void);
int NWC24BlockOpenMsgLib(int block);

/* the scheduler pair and the two request entry points the device layer calls */
int NWC24SuspendScheduler(void);
int NWC24ResumeScheduler(void);
int NWC24iSetScriptMode(u32 mode);
int NWC24iGetUserId(u32* userId);
int NWC24iRequestGenerateUserId(u32* userId, u32* ticket);
int NWC24iRequestIoctl(u32 handle, u32 command, u32* argument);

/* the `/dev/net/kd/request` command thunks; each forwards one command number */
int NWC24iRequestCommand1(void);
int NWC24iRequestCommand3(void);
int NWC24iRequestCommand6(u32* out);
int NWC24iRequestCommand7(u32* out);
int NWC24iLockSocket(void);
int NWC24iUnlockSocket(void);

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `unsplit/NWC24.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x807958A8 - the work block's one-time-initialisation flag (bit 0). */
extern u32 sNwc24WorkInit;

/* 0x807958AC / 0x807958B0 - the scheduler pair's two counters: the depth `NWC24SuspendScheduler`
 * raises and `NWC24ResumeScheduler` lowers, and the limit both compare against. */
extern s32 sNwc24SuspendCount;

extern s32 sNwc24ResumeLimit;

/* 0x80795898 - the library's work pointer; +0x08 is the cached user id (two words). */
extern u32* sNwc24UserWork;

extern const char Nwc24SetScriptModeName[]; /* 0x8063118C "NWC24iSetScriptMode" */

extern const char Nwc24GenerateUserIdName[]; /* 0x806311A0 "NWC24iRequestGenerateUserId" */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NWC24_NWC24_MSG_H */
