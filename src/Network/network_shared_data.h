/*
 * include/Network/network_shared_data.h - the Network band's shared small-data pool, owned by the
 * data-only unit `Network/network_shared_data.cpp` (rule 12's named owner, playbook 54's model).
 *
 * The two runs this header's words live in -
 *   `.sdata2` 0x8079C690-0x8079C758 and `.sdata` 0x80793900-0x80793930 -
 * are the MWLD merge of several Network objects' own pools: one address is read by more than one TU
 * (`callers.py 0x8079C6EC` answers the Network transport units (`Network/NetworkPeerMcs.cpp` ...) and `Network/NetworkSessionManager.cpp`;
 * `0x8079C750` answers an unsplit (Network) object as well), so no single consumer can emit the run
 * and no consumer may claim it without taking rows another registered unit reads.  Giving the run one
 * owner is what lets every consumer *include this header* instead of declaring the words into its own
 * file - which is the rule-12 finding the declarations below used to be, in `Network/NetworkSessionManager.h`.
 *
 * The declarations moved here verbatim from that header (`networkMillisecondsPerSecond` ..
 * `networkSessionPatTimeOrigin`) in the same change that registered the owner; their names and their
 * value comments are that lane's, read off the DOL.  Nothing renames them.
 *
 * The run also holds the rows no consumer has needed to name yet (0x8079C690, 0x8079C6C8, 0x8079C6D0,
 * 0x8079C6D8, ..., still `lbl_8079C6xx` in the map).  They stay unnamed here on purpose: rule 7 names
 * a row from *what it holds and where it is used*, and the bodies that use them are unwritten, so the
 * lane that writes those bodies names them then and declares them in this header.  Playbook 29's
 * shape: a claimed pool is **declared, never defined** - the source here defines nothing and the
 * original bytes stay in the binary.
 */

#ifndef NETWORK_SHARED_DATA_H
#define NETWORK_SHARED_DATA_H

#include "types.h"

/* the band's rate/timer float pool (values read off the DOL; each name is the use the range makes of
   it).  `networkRateFloor` is the one word of the set that lives in `.sdata`, not `.sdata2`. */
extern const f32 networkMcsRetryInterval;       /* 0x8079C6C8 = 0.5f - seconds the Mcs peer waits between two connection attempts */
extern const f32 networkMillisecondsPerSecond;   /* 0x8079C6EC = 1000.0f */
extern const f32 networkRateScale;               /* 0x8079C6F0 = 2.0f */
extern const f32 networkRateMax;                 /* 0x8079C6F8 = 1.0f */
extern const f32 networkRateMin;                 /* 0x8079C708 = 0.1f */
extern const f32 networkRateUpStep;              /* 0x8079C718 = 0.017f */
extern const f32 networkRateUpLerp;              /* 0x8079C730 = 0.5f */
extern const f32 networkRateDownStep;            /* 0x8079C734 = 0.008f */
extern const f32 networkRateDecay;               /* 0x8079C738 = 0.002f */
extern const f32 networkRateDownLerp;            /* 0x8079C73C = 0.25f */
extern f32 networkRateFloor;               /* 0x8079392C = 0.032f (.sdata) */
extern const f32 networkRequestZero;             /* 0x8079C740 = 0.0f */
extern const f32 networkRequestTimerIdle;        /* 0x8079C748 = 0.0f */
extern const f32 networkRequestTimerReset;       /* 0x8079C750 = 0.0f */
extern const f32 networkSessionPatTimeOrigin;    /* 0x8079C754 = -3600.0f */

/* the band's initialised `.sdata` words (GUESS on every name: each is read off the setter that writes it) */
extern u32 networkSessionNotifyValue;         /* 0x80793910 = 1 */
extern u32 networkSessionMaxHosts;            /* 0x80793914 = 0x10 */
extern u32 networkSessionMaxSubhosts;         /* 0x80793918 = 4 */
extern f32 networkSessionHostTimeout;         /* 0x8079391C = 20.0f */
extern f32 networkSessionSubhostTimeout;      /* 0x80793920 = 20.0f */
extern f32 networkSessionRateStep;            /* 0x80793924 = 0.25f - 1 / the divisor `setRate` was given */
extern s32 networkSessionRateWindow;          /* 0x80793928 = 0x200 - the quotient `setRate` stores */
extern const f32 networkSessionUnit;                /* 0x8079C6D0 = 1.0f */
extern const f32 networkNonceScale;                 /* 0x8079C6E0 = 1000.0f */
extern const f32 networkSessionDefaultDelay;        /* 0x8079C690 = 3.0f - a slot's relay delay before a route is known (GUESS) */
extern const f32 networkSessionZero;                /* 0x8079C6E8 = 0.0f */
extern const f32 networkSessionRelayCooldown;       /* 0x8079C6F4 = 10.0f - seconds a slot waits before it retries the relay (GUESS) */
extern const f32 networkSessionRelayWarnRatio;      /* 0x8079C6FC = 0.7f - share of the subhost timeout after which a missing relay is logged (GUESS) */
extern const f32 networkSessionEstablishInterval;   /* 0x8079C700 = 35.0f - seconds between two establish requests (GUESS) */
extern const f32 networkSessionCongestionKeep;      /* 0x8079C704 = 0.9f - the weight a slot's congestion average keeps (GUESS) */
extern const f32 networkSessionNever;               /* 0x8079C71C = -1.0f */
extern const f32 networkSessionPriorityScale;       /* 0x8079C728 = 256.0f (GUESS) */
extern const f32 networkSessionPriorityRange;       /* 0x8079C72C = 4096.0f (GUESS) */

#endif /* NETWORK_SHARED_DATA_H */
