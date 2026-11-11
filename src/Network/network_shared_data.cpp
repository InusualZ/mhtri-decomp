/*
 * Network/network_shared_data.cpp - the Network band's shared small-data pool: the rate/timer words and floats the
 *   transport units and `Network/NetworkSessionManager.cpp` read (data-only, the model is `Pl/pl_frame_data.cpp`).
 * RANGE. no `.text`; `.sdata2` 0x8079C6C8-0x8079C758 (144 B, the rate/timer float pool) and `.sdata`
 *   0x80793900-0x80793930 (48 B, the rate/interval words, ending at `networkRateFloor`).  The `.sdata2` start is the
 *   end of `Network/NetworkConnectionStable.cpp`'s own pool 0x8079C698-0x8079C6C8, its end the first row of
 *   `Network/NetworkSessionManagerPat.cpp`'s claim; the block is linked right after
 *   `Network/NetworkConnectionStable.cpp` (the next `.sdata`/`.sdata2` claims, 0x80793930/0x8079C758, come later).
 * NAMES. The file name is descriptive (no `__FILE__` string covers the pool).  GUESS: `networkS32ToF64Bias`,
 *   `networkS32ToF64BiasAlt` (0x4330000080000000, the signed int-to-double bias) and `networkU32ToF64Bias`
 *   (0x4330000000000000, the unsigned one), read off the constants' values.
 * SHAPES. The pool has several readers - the transport units and `Network/NetworkSessionManager.cpp` share
 *   0x8079C6EC..0x8079C754 (`callers.py 0x8079C6EC`), unsplit Network code reads 0x8079C750 - so no consumer may
 *   claim it; one owner lets every consumer include `Network/network_shared_data.h` (rule 2).
 */

#include "types.h"
#include "Network/network_shared_data.h"

/* `.sdata` words, in address order. */
f32 networkConnectionDefaultInterval = 20.0f;
f32 networkConnectionDefaultTimeout = 20.0f;
f32 networkConnectionDefaultLimit = 20.0f;
u32 networkSessionNotifyValue __attribute__((aligned(8))) = 1;
u32 networkSessionMaxHosts = 0x10;
u32 networkSessionMaxSubhosts = 4;
f32 networkSessionHostTimeout = 20.0f;
f32 networkSessionSubhostTimeout = 20.0f;
f32 networkSessionRateStep = 0.25f;
s32 networkSessionRateWindow = 0x200;
f32 networkRateFloor = 0.032f;

/* `.sdata2` constants, in address order; `aligned(8)` reproduces the zero word retail leaves after a float. */
const f32 networkMcsRetryInterval = 0.5f;
const f32 networkSessionUnit __attribute__((aligned(8))) = 1.0f;
const f64 networkS32ToF64Bias = 4503601774854144.0;
const f32 networkNonceScale = 1000.0f;
const f32 networkSessionZero __attribute__((aligned(8))) = 0.0f;
const f32 networkMillisecondsPerSecond = 1000.0f;
const f32 networkRateScale = 2.0f;
const f32 networkSessionRelayCooldown = 10.0f;
const f32 networkRateMax = 1.0f;
const f32 networkSessionRelayWarnRatio = 0.7f;
const f32 networkSessionEstablishInterval = 35.0f;
const f32 networkSessionCongestionKeep = 0.9f;
const f32 networkRateMin = 0.1f;
const f64 networkS32ToF64BiasAlt = 4503601774854144.0;
const f32 networkRateUpStep = 0.017f;
const f32 networkSessionNever = -1.0f;
const f64 networkU32ToF64Bias = 4503599627370496.0;
const f32 networkSessionPriorityScale = 256.0f;
const f32 networkSessionPriorityRange = 4096.0f;
const f32 networkRateUpLerp = 0.5f;
const f32 networkRateDownStep = 0.008f;
const f32 networkRateDecay = 0.002f;
const f32 networkRateDownLerp = 0.25f;
const f32 networkRequestZero __attribute__((aligned(8))) = 0.0f;
const f32 networkRequestTimerIdle __attribute__((aligned(8))) = 0.0f;
const f32 networkRequestTimerReset __attribute__((aligned(8))) = 0.0f;
const f32 networkSessionPatTimeOrigin = -3600.0f;
