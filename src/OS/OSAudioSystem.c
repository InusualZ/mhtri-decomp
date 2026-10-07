/*
 * OS/OSAudioSystem.c - the OS audio-system bring-up: AI clock init, audio system init and stop.
 * RANGE. .text 0x804CC130-0x804CC5F0 (3 functions); .data 0x8061C0D8-0x8061C158.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: `DSPInitCode` (.data 0x8061C0D8, 0x80 B) is read only by
 *    `__OSInitAudioSystem`; the three functions call the time base and the AI registers.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. map names throughout; the `DSP_*` and `HW_*` register macros are GUESSes named from how the bring-up sequence uses each
 *    halfword/word (mailboxes, control/status, ARAM DMA triple); `DelayMicroseconds` is a GUESS for the inline time-base wait.
 * RESIDUALS. none recorded yet.
 * SHAPES. plain C over volatile hardware registers; the microsecond wait is a static inline helper expanded in place.
 */

#include "types.h"

#include "OS/OS.h"
#include "OS/OSAudioSystem.h"
#include "OS/OSArena.h"
#include "OS/OSCache.h"
#include "OS/OSTime.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* The DSP interface registers (0xCC005000 block). */
#define DSP_MAILBOX_TO_HI (*(volatile u16*)0xCC005000)
#define DSP_MAILBOX_FROM_HI (*(volatile u16*)0xCC005004)
#define DSP_MAILBOX_FROM_LO (*(volatile u16*)0xCC005006)
#define DSP_CONTROL (*(volatile u16*)0xCC00500A)
#define DSP_ARAM_SIZE (*(volatile u16*)0xCC005012)
#define DSP_DMA_MAIN_ADDRESS (*(volatile u32*)0xCC005020)
#define DSP_DMA_ARAM_ADDRESS (*(volatile u32*)0xCC005024)
#define DSP_DMA_LENGTH (*(volatile u32*)0xCC005028)
#define DSP_AI_DMA_CONTROL (*(volatile u16*)0xCC005036)

/* The two clock-control words of the Hollywood block (0xCD800000) the AI clock bring-up programs. */
#define HW_CLOCK_SELECT (*(volatile u32*)0xCD800180)
#define HW_AI_CLOCK_CONFIG (*(volatile u32*)0xCD8001CC)
#define HW_AI_CLOCK_CONTROL (*(volatile u32*)0xCD8001D0)

#define DSP_INIT_CODE_ADDRESS ((void*)0x81000000)

u16 DSPInitCode[64] = {
    0x029F, 0x0010, 0x029F, 0x0033, 0x029F, 0x0034, 0x029F, 0x0035,
    0x029F, 0x0036, 0x029F, 0x0037, 0x029F, 0x0038, 0x029F, 0x0039,
    0x1206, 0x1203, 0x1204, 0x1205, 0x0080, 0x8000, 0x0088, 0xFFFF,
    0x0084, 0x1000, 0x0064, 0x001D, 0x0218, 0x0000, 0x8100, 0x1C1E,
    0x0044, 0x1B1E, 0x0084, 0x0800, 0x0064, 0x0027, 0x191E, 0x0000,
    0x00DE, 0xFFFC, 0x02A0, 0x8000, 0x029C, 0x0028, 0x16FC, 0x0054,
    0x16FD, 0x4348, 0x0021, 0x02FF, 0x02FF, 0x02FF, 0x02FF, 0x02FF,
    0x02FF, 0x02FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

/* Busy-waits for `us` microseconds of the time base. */
static inline void DelayMicroseconds(u32 us)
{
    u32 start = OSGetTick();

    while ((u32)((OSGetTick() - start) * 8) / ((OS_BUS_CLOCK / 4) / 125000) < us) {
    }
}

/* Programs the audio clock; `source` selects the clock source bit. */
void __AIClockInit(int source)
{
    u32 value;

    value = HW_CLOCK_SELECT;
    value &= ~0x100;
    value |= source << 8;
    HW_CLOCK_SELECT = value & ~0x80;
    HW_AI_CLOCK_CONTROL &= 0x3FFFFFFF;
    DelayMicroseconds(100);
    if (source == 0) {
        value = HW_AI_CLOCK_CONFIG;
        value = (value & 0xFFFC003F) | 0xFC0;
        value &= 0xF803FFC0;
        HW_AI_CLOCK_CONFIG = value | 0x04640000;
    } else {
        value = HW_AI_CLOCK_CONFIG;
        value = (value & 0xFFFC003F) | 0xFFC0;
        value &= ~0x3F;
        value |= 0xE;
        value &= 0xF803FFFF;
        HW_AI_CLOCK_CONFIG = value | 0x04B00000;
    }
    DelayMicroseconds(100);
    HW_AI_CLOCK_CONTROL &= 0xEFFFFFFF;
    DelayMicroseconds(1000);
    HW_AI_CLOCK_CONTROL = (HW_AI_CLOCK_CONTROL & 0xBFFFFFFF) | 0x40000000;
    DelayMicroseconds(1000);
    HW_AI_CLOCK_CONTROL = (HW_AI_CLOCK_CONTROL & 0x7FFFFFFF) | 0x80000000;
    DelayMicroseconds(1000);
}

/* Starts the DSP, runs the 0x80-byte init code from the top of MEM1 and restores the memory it borrowed. */
void __OSInitAudioSystem(void)
{
    u16 status;
    u32 start;

    if (!__OSInIPL) {
        __AIClockInit(1);
    }
    memcpy((u8*)OSGetArenaHi() - 0x80, DSP_INIT_CODE_ADDRESS, 0x80);
    memcpy(DSP_INIT_CODE_ADDRESS, DSPInitCode, 0x80);
    DCFlushRange(DSP_INIT_CODE_ADDRESS, 0x80);
    DSP_ARAM_SIZE = 0x43;
    DSP_CONTROL = 0x8AC;
    DSP_CONTROL |= 1;
    while (DSP_CONTROL & 1) {
    }
    DSP_MAILBOX_TO_HI = 0;
    while (((DSP_MAILBOX_FROM_HI << 16) | DSP_MAILBOX_FROM_LO) & 0x80000000) {
    }
    DSP_DMA_MAIN_ADDRESS = 0x01000000;
    DSP_DMA_ARAM_ADDRESS = 0;
    DSP_DMA_LENGTH = 0x20;
    status = DSP_CONTROL;
    while (!(status & 0x20)) {
        status = DSP_CONTROL;
    }
    DSP_CONTROL = status;
    start = OSGetTick();
    while ((s32)(OSGetTick() - start) < 0x892) {
    }
    DSP_DMA_MAIN_ADDRESS = 0x01000000;
    DSP_DMA_ARAM_ADDRESS = 0;
    DSP_DMA_LENGTH = 0x20;
    status = DSP_CONTROL;
    while (!(status & 0x20)) {
        status = DSP_CONTROL;
    }
    DSP_CONTROL = status;
    DSP_CONTROL = (u32)DSP_CONTROL & 0xFFFFF7FF;
    while (DSP_CONTROL & 0x400) {
    }
    DSP_CONTROL = (u32)DSP_CONTROL & 0xFFFFFFFB;
    status = DSP_MAILBOX_FROM_HI;
    while (!(status & 0x8000)) {
        status = DSP_MAILBOX_FROM_HI;
    }
    (void)DSP_MAILBOX_FROM_LO;
    DSP_CONTROL |= 4;
    DSP_CONTROL = 0x8AC;
    DSP_CONTROL |= 1;
    while (DSP_CONTROL & 1) {
    }
    memcpy(DSP_INIT_CODE_ADDRESS, (u8*)OSGetArenaHi() - 0x80, 0x80);
}

/* Halts the DSP audio engine and waits for it to stop. */
void __OSStopAudioSystem(void)
{
    u32 start;
    u16 status;

    DSP_CONTROL = 0x804;
    DSP_AI_DMA_CONTROL &= 0x7FFF;
    status = DSP_CONTROL;
    while (status & 0x400) {
        status = DSP_CONTROL;
    }
    status = DSP_CONTROL;
    while (status & 0x200) {
        status = DSP_CONTROL;
    }
    DSP_CONTROL = 0x8AC;
    DSP_MAILBOX_TO_HI = 0;
    while (((DSP_MAILBOX_FROM_HI << 16) | DSP_MAILBOX_FROM_LO) & 0x80000000) {
    }
    start = OSGetTick();
    while ((s32)(OSGetTick() - start) < 44) {
    }
    DSP_CONTROL |= 1;
    status = DSP_CONTROL;
    while (status & 1) {
        status = DSP_CONTROL;
    }
}
