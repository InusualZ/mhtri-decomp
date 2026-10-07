/*
 * VI/i2c.h - the entry point of `VI/i2c.cpp` that the video encoder programming calls.
 */
#ifndef VI_I2C_H
#define VI_I2C_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void WaitMicroTime(s32 us);
BOOL sendSlaveAddr(u8 addr);
BOOL __VISendI2CData(u8 slaveAddr, const u8* data, s32 size);

#ifdef __cplusplus
}
#endif

#endif
