/*
 * Network/network_shared_data.h - the words of the Network band's shared small-data pool (owner: the data-only unit
 *   `Network/network_shared_data.cpp`; every consumer includes this header instead of declaring them, rule 12).
 * SHAPES. The pool is declared, never defined (playbook 29: the original bytes stay); rows no body reads yet stay
 *   `lbl_8079C6xx` in the map until a body that uses them names them here.
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
