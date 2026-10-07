/* VI/i2c.cpp - the bit-banged I2C writer the video encoder is programmed through (`WaitMicroTime`, `sendSlaveAddr`, `__VISendI2CData`).
 * RANGE. .text 0x804E91C0-0x804E9AE0 (3 functions); .sdata 0x80794180-0x80794188; .sbss 0x80795688-0x80795690.
 *   Edges: the 8-byte .sdata object 0x80794180 and the .sbss word 0x80795688 are read only here; the callers are the
 *   vi3in1 functions (0x804E9AE0..) and none of vi.c.
 * FLAGS. `cflags_base` (-O4,p, 16-byte function alignment): every start is 16-aligned.
 * NAMES. all three functions are the map's names; `i2cSdaActiveHigh` and `i2cStarted` are GUESSES (the former selects
 *   which output level a logical 1 is driven with and starts at 1, the latter is set by the first send); both are
 *   two-word arrays because the target objects are 8 bytes (the second word is never read).
 * RESIDUALS. Relocations: the two-word state arrays carry offset 0 of `i2cSdaActiveHigh` where retail names the word itself.
 *   Flip blocker: .text object 0x910 vs claimed 0x920 (the two register-numbering rows below).  `sendSlaveAddr` (0x804E9250): the target loads the polarity word twice after the ack check, ours once
 *   (one lwz/cmp pair). `__VISendI2CData` (0x804E95A0): register numbering only - the target keeps the arguments in
 *   r29/r26/r27 and the interrupt level in r28, ours in r24-r26; `permdecl` over the two locals does not move it.
 */
#include "types.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"
#include "OS/__OSGetSystemTime.h"
#include "VI/i2c.h"

/* The GPIO output, direction and input registers that carry the I2C clock (0x4000) and data (0x8000) lines. */
#define I2C_GPIO_OUT (*(volatile u32*)0xCD8000C0)
#define I2C_GPIO_DIR (*(volatile u32*)0xCD8000C4)
#define I2C_GPIO_IN (*(volatile u32*)0xCD8000C8)

#define I2C_SCL 0x4000
#define I2C_SDA 0x8000

static u32 i2cSdaActiveHigh[2] = {1, 0};
static u32 i2cStarted[2];

void WaitMicroTime(s32 us)
{
    s64 start = __OSGetSystemTime();
    while ((__OSGetSystemTime() - start) * 8 / 486 < us) {
    }
}

#define I2C_OUT_SET(bit) (I2C_GPIO_OUT = (I2C_GPIO_OUT & ~(bit)) | (bit))
#define I2C_OUT_CLR(bit) (I2C_GPIO_OUT = I2C_GPIO_OUT & ~(bit))

/* Drives the data line to logic 1 (0 when the polarity flag is clear). */
#define I2C_SDA_HIGH()                 \
    if (i2cSdaActiveHigh[0] == 0) {       \
        I2C_OUT_CLR(I2C_SDA);          \
    } else {                           \
        I2C_OUT_SET(I2C_SDA);          \
    }

#define I2C_SDA_LOW()                  \
    if (i2cSdaActiveHigh[0] == 0) {       \
        I2C_OUT_SET(I2C_SDA);          \
    } else {                           \
        I2C_OUT_CLR(I2C_SDA);          \
    }

BOOL sendSlaveAddr(u8 addr)
{
    s32 i;

    I2C_SDA_LOW();
    WaitMicroTime(2);
    I2C_OUT_CLR(I2C_SCL);
    for (i = 0; i < 8; i++) {
        if (addr & 0x80) {
            I2C_SDA_HIGH();
        } else {
            I2C_SDA_LOW();
        }
        WaitMicroTime(2);
        I2C_OUT_SET(I2C_SCL);
        WaitMicroTime(2);
        I2C_OUT_CLR(I2C_SCL);
        addr <<= 1;
    }
    I2C_GPIO_DIR = (I2C_GPIO_DIR & ~I2C_SDA) | I2C_SCL;
    WaitMicroTime(2);
    I2C_OUT_SET(I2C_SCL);
    WaitMicroTime(2);
    if (i2cSdaActiveHigh[0] == 1 && ((I2C_GPIO_IN >> 15) & 1)) {
        return FALSE;
    } else {
        I2C_SDA_LOW();
        I2C_GPIO_DIR = (I2C_GPIO_DIR & ~I2C_SDA) | (I2C_SCL | I2C_SDA);
        I2C_OUT_CLR(I2C_SCL);
        return TRUE;
    }
}

BOOL __VISendI2CData(u8 slaveAddr, const u8* data, s32 size)
{
    BOOL level;
    s32 i;

    if (i2cStarted[0] == 0) {
        i2cSdaActiveHigh[0] = 1;
        i2cStarted[0] = 1;
    }
    level = OSDisableInterrupts();
    I2C_GPIO_DIR = (I2C_GPIO_DIR & ~I2C_SDA) | (I2C_SCL | I2C_SDA);
    I2C_GPIO_OUT = (I2C_GPIO_OUT & ~I2C_SCL) | I2C_SCL;
    if (i2cSdaActiveHigh[0] == 0) {
        I2C_OUT_CLR(I2C_SDA);
    } else {
        I2C_OUT_SET(I2C_SDA);
    }
    WaitMicroTime(2);
    WaitMicroTime(2);
    if (!sendSlaveAddr(slaveAddr)) {
        OSRestoreInterrupts(level);
        return FALSE;
    }
    I2C_GPIO_DIR = (I2C_GPIO_DIR & ~I2C_SDA) | (I2C_SCL | I2C_SDA);
    while (size != 0) {
        slaveAddr = *data++;
        for (i = 0; i < 8; i++) {
            if (slaveAddr & 0x80) {
                I2C_SDA_HIGH();
            } else {
                I2C_SDA_LOW();
            }
            WaitMicroTime(2);
            I2C_OUT_SET(I2C_SCL);
            WaitMicroTime(2);
            I2C_OUT_CLR(I2C_SCL);
            slaveAddr <<= 1;
        }
        I2C_GPIO_DIR = (I2C_GPIO_DIR & ~I2C_SDA) | I2C_SCL;
        WaitMicroTime(2);
        I2C_OUT_SET(I2C_SCL);
        WaitMicroTime(2);
        if (i2cSdaActiveHigh[0] == 1 && ((I2C_GPIO_IN >> 15) & 1)) {
            OSRestoreInterrupts(level);
            return FALSE;
        }
        I2C_SDA_LOW();
        I2C_GPIO_DIR = (I2C_GPIO_DIR & ~I2C_SDA) | (I2C_SCL | I2C_SDA);
        I2C_OUT_CLR(I2C_SCL);
        size--;
    }
    I2C_GPIO_DIR = (I2C_GPIO_DIR & ~I2C_SDA) | (I2C_SCL | I2C_SDA);
    I2C_SDA_LOW();
    WaitMicroTime(2);
    I2C_OUT_SET(I2C_SCL);
    WaitMicroTime(2);
    I2C_SDA_HIGH();
    OSRestoreInterrupts(level);
    return TRUE;
}
