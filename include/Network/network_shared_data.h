/*
 * include/Network/network_shared_data.h - the Network band's shared small-data pool, owned by the
 * data-only unit `Network/network_shared_data.cpp` (rule 12's named owner, playbook 54's model).
 *
 * The two runs this header's words live in -
 *   `.sdata2` 0x8079C690-0x8079C758 and `.sdata` 0x80793900-0x80793930 -
 * are the MWLD merge of several Network objects' own pools: one address is read by more than one TU
 * (`callers.py 0x8079C6EC` answers `Network/network_transport.cpp` and `Network/fn_803D3CE8.cpp`;
 * `0x8079C750` answers an unsplit (Network) object as well), so no single consumer can emit the run
 * and no consumer may claim it without taking rows another registered unit reads.  Giving the run one
 * owner is what lets every consumer *include this header* instead of declaring the words into its own
 * file - which is the rule-12 finding the declarations below used to be, in `Network/fn_803D3CE8.h`.
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
extern f32 networkMcsRetryInterval;       /* 0x8079C6C8 = 0.5f - seconds the Mcs peer waits between two connection attempts */
extern f32 networkMillisecondsPerSecond;   /* 0x8079C6EC = 1000.0f */
extern f32 networkRateScale;               /* 0x8079C6F0 = 2.0f */
extern f32 networkRateMax;                 /* 0x8079C6F8 = 1.0f */
extern f32 networkRateMin;                 /* 0x8079C708 = 0.1f */
extern f32 networkRateUpStep;              /* 0x8079C718 = 0.017f */
extern f32 networkRateUpLerp;              /* 0x8079C730 = 0.5f */
extern f32 networkRateDownStep;            /* 0x8079C734 = 0.008f */
extern f32 networkRateDecay;               /* 0x8079C738 = 0.002f */
extern f32 networkRateDownLerp;            /* 0x8079C73C = 0.25f */
extern f32 networkRateFloor;               /* 0x8079392C = 0.032f (.sdata) */
extern f32 networkRequestZero;             /* 0x8079C740 = 0.0f */
extern f32 networkRequestTimerIdle;        /* 0x8079C748 = 0.0f */
extern f32 networkRequestTimerReset;       /* 0x8079C750 = 0.0f */
extern f32 networkSessionPatTimeOrigin;    /* 0x8079C754 = -3600.0f */

/* the band's initialised `.sdata` words (GUESS on every name: each is read off the setter that writes it) */
extern u32 networkSessionNotifyValue;         /* 0x80793910 = 1 */
extern u32 networkSessionMaxHosts;            /* 0x80793914 = 0x10 */
extern u32 networkSessionMaxSubhosts;         /* 0x80793918 = 4 */
extern f32 networkSessionHostTimeout;         /* 0x8079391C = 20.0f */
extern f32 networkSessionSubhostTimeout;      /* 0x80793920 = 20.0f */
extern f32 networkSessionRateStep;            /* 0x80793924 = 0.25f - 1 / the divisor `setRate` was given */
extern s32 networkSessionRateWindow;          /* 0x80793928 = 0x200 - the quotient `setRate` stores */
extern f32 networkSessionUnit;                /* 0x8079C6D0 = 1.0f */
extern f32 networkNonceScale;                 /* 0x8079C6E0 = 1000.0f */

#endif /* NETWORK_SHARED_DATA_H */
