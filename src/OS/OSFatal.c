/*
 * OS/OSFatal.c - the OS fatal-error screen: the video/GX configuration helpers, `OSFatal` and the text drawer.
 * RANGE. .text 0x804CF350-0x804CFF80 (4 functions); .bss 0x8074D360-0x8074D640; .sdata 0x80793FA8-0x80793FB0; .sdata2
 *    0x8079D2D8-0x8079D318.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the bss run
 *    0x8074D360..0x8074D640 and the 14 float constants at .sdata2 0x8079D2D8..0x8079D318 are read only by
 *    `OSFatal` (0x804CF7A0) and `FatalScreenFiber`; the helper at 0x804CF350 calls into the font code (0x804D09E0) and
 *    the one at 0x804CF680 into VI (0x804E7F60, 0x804E8630, 0x804E8CE0); `__OSBootDol` ends the exec unit at
 *    0x804CF350.
 * FLAGS. `cflags_base` per object (configure.py) with `#pragma fp_contract off` for the colour conversion (retail keeps the
 *    separate fmuls/fadds).
 * NAMES. `OSFatal` is the map's name; the left edge (0x804CF350) is medium evidence.  `DrawFatalText` is a GUESS (draws a
 *    string into the YCbCr framebuffer), `ConfigureFatalVideo` is a GUESS (builds a render mode and hands it to VI),
 *    `FatalScreenFiber` is a GUESS (the screen task `OSFatal` switches to), `s_fatalContext`, `s_fatalParam` and `s_fatalFormat`
 *    are GUESS names; `VIInit` (0x804E7480), `VIConfigure` (0x804E7F60), `VIConfigurePan` (0x804E8630) and `GXAbortFrame`
 *    (0x804B5FA0) are GUESS names taken from their bodies and from the arguments this unit passes.
 * RESIDUALS. `DrawFatalText` (90 %): the glyph loop is a `for(;;)` with the end-of-string test on top (the target tests at the
 *    bottom and leaves by fall-through), the pixel store is `stbux` where the target uses `stbx` plus a separate pointer, and the
 *    multiplies are scheduled in the other order. `FatalScreenFiber` (98.6 %): same instructions, `param`/`len`/`font` take
 *    r31/r29/r30 where the target has r29/r30/r31. The .bss run is 0xC short of the claim (link padding) and the format string
 *    is an anonymous .sdata word here, not `s_fatalFormat`.
 * SHAPES. the RGB to YCbCr conversion (ITU-R BT.601 coefficients) is an inline helper expanded for the background and
 *    the text colour; the glyph buffer is zeroed through a local row index and read through a row pointer (both
 *    measurably better than inline index expressions).
 */

#pragma fp_contract off

#include "types.h"

#include "gx.h"
#include "gx/GXRenderModeObj.h"
#include "EXI/EXIBios.h"
#include "MSL/strlen.h"
#include "MSL_C/alloc.h"
#include "OS/OS.h"
#include "OS/OSArena.h"
#include "OS/OSAudioSystem.h"
#include "OS/OSCache.h"
#include "OS/OSContext.h"
#include "OS/OSDefaultExceptionHandler.h"
#include "OS/OSError.h"
#include "OS/OSFatal.h"
#include "OS/OSFont.h"
#include "OS/OSInterrupt.h"
#include "OS/OSReset.h"
#include "OS/OSSwitchFiber.h"
#include "OS/OSThread.h"
#include "OS/OSTime.h"
#include "OS/PPCHalt.h"
#include "RVLGX/GXAbortFrame.h"
#include "VI/vi.h"

#define OS_TIMER_CLOCK (OS_BUS_CLOCK / 4)

/* size: 0xC - the colours and the message `OSFatal` hands to the screen task */
typedef struct FatalParam {
    /* +0x00 */ GXColor fg;
    /* +0x04 */ GXColor bg;
    /* +0x08 */ const char* message;
} FatalParam;

static FatalParam s_fatalParam;
static OSContext s_fatalContext;

/* Converts an RGB colour to the YCbCr triple the framebuffer stores (Y in `r`, Cb in `g`, Cr in `b`). */
static inline GXColor RGBToYCbCr(GXColor rgb) {
    GXColor yuv;
    f32 y = 0.5f + (16.0f + (0.098f * rgb.b + (0.257f * rgb.r + 0.504f * rgb.g)));
    f32 cb = 0.5f + (128.0f + (0.439f * rgb.b + (-0.148f * rgb.r - 0.291f * rgb.g)));
    f32 cr = 0.5f + (128.0f + ((0.439f * rgb.r - 0.368f * rgb.g) - 0.071f * rgb.b));

    yuv.r = (u8)(y > 235.0f ? 235.0f : (y < 16.0f ? 16.0f : y));
    yuv.g = (u8)(cb > 240.0f ? 240.0f : (cb < 16.0f ? 16.0f : cb));
    yuv.b = (u8)(cr > 240.0f ? 240.0f : (cr < 16.0f ? 16.0f : cr));
    yuv.a = 0;
    return yuv;
}

/* Waits until `count` vertical retraces have passed. */
static inline void WaitRetraces(s32 count) {
    s32 start = VIGetRetraceCount();

    while ((s32)(VIGetRetraceCount() - start) < count) {
    }
}

/* 0x804CF350 (0x328): draws `str` at (`x`, `y`) of the YCbCr framebuffer `fb`, line by line, with `color`. */
static void DrawFatalText(u8* fb, s32 width, s32 height, GXColor color, s32 x, s32 y, u16 leading, const char* str) {
    u32 glyph[72];
    s32 glyphWidth;
    s32 limit = height - 24;

    while (limit >= y) {
        s32 cx = x;
        u8* dst = fb + (x + y * width) * 2;

        for (;;) {
            u32 row;
            u32 col;

            if (*str == 0) {
                return;
            }
            if (*str == '\n') {
                y += leading;
                str++;
                break;
            }
            if (width - 48 < cx) {
                y += leading;
                break;
            }
            for (row = 0; row < 24; row++) {
                s32 base = (row & 7) + (row >> 3) * 24;

                glyph[base] = 0;
                glyph[base + 8] = 0;
                glyph[base + 16] = 0;
            }
            str = OSGetFontTexel(str, (u8*)glyph, 0, 6, &glyphWidth);
            for (row = 0; row < 24; row++) {
                u32* line = &glyph[(row & 7) + (row >> 3) * 24];

                for (col = 0; col < 24; col++) {
                    s32 texel = (line[(col >> 3) * 8] >> ((7 - (col & 7)) * 4)) & 0xF;

                    if (texel != 0) {
                        u8* pixel = dst + (col + row * width) * 2;

                        pixel[0] = color.r * texel * 239 / 255 / 15 + 16;
                        if ((cx + col) & 1) {
                            pixel[-1] = color.g;
                            pixel[1] = color.b;
                        } else {
                            pixel[-1] = color.b;
                            pixel[1] = color.g;
                        }
                    }
                }
            }
            cx += glyphWidth;
            dst += glyphWidth * 2;
        }
    }
}

/* 0x804CF680 (0x120): programs VI for a `fbWidth` x `xfbHeight` frame buffer in the console's TV format. */
static void ConfigureFatalVideo(s16 fbWidth, s16 xfbHeight) {
    GXRenderModeObj mode;

    mode.fbWidth = fbWidth;
    mode.efbHeight = 480;
    mode.xfbHeight = xfbHeight;
    mode.viXOrigin = 40;
    mode.viWidth = 640;
    mode.viHeight = xfbHeight;
    switch (VIGetTvFormat()) {
    case 0:
    case 2:
        if (*(volatile u16*)0xCC00206C & 1) {
            mode.viTVmode = 2;
            mode.viYOrigin = 0;
            mode.xfbMode = 0;
        } else {
            mode.viTVmode = 0;
            mode.viYOrigin = 0;
            mode.xfbMode = 1;
        }
        break;
    case 5:
        if (*(volatile u16*)0xCC00206C & 1) {
            mode.viTVmode = 22;
            mode.viYOrigin = 0;
            mode.xfbMode = 0;
        } else {
            mode.viTVmode = 20;
            mode.viYOrigin = 0;
            mode.xfbMode = 1;
        }
        break;
    case 1:
        mode.viTVmode = 4;
        mode.viYOrigin = 47;
        mode.xfbMode = 1;
        break;
    }
    VIConfigure(&mode);
    VIConfigurePan(0, 0, 640, 480);
}

static void FatalScreenFiber(void);

/* 0x804CF7A0 (0x1E8): shuts the system down and switches to the fatal screen. */
void OSFatal(GXColor fg, GXColor bg, const char* message) {
    s64 start;
    void* arenaHi;

    OSDisableInterrupts();
    OSDisableScheduler();
    OSClearContext(&s_fatalContext);
    OSSetCurrentContext(&s_fatalContext);
    __OSStopAudioSystem();
    VIInit();
    __OSUnmaskInterrupts(0x80);
    VISetBlack(TRUE);
    VIFlush();
    VISetPreRetraceCallback(NULL);
    VISetPostRetraceCallback(NULL);
    OSEnableInterrupts();
    WaitRetraces(1);
    start = OSGetTime();
    do {
        if (__OSCallShutdownFunctions(FALSE, 0)) {
            break;
        }
    } while (OSGetTime() - start < OS_TIMER_CLOCK / 1000 * 1000);
    OSDisableInterrupts();
    __OSCallShutdownFunctions(TRUE, 0);
    EXISetExiCallback(EXI_CHAN_0, NULL);
    EXISetExiCallback(EXI_CHAN_2, NULL);
    while (!EXILock(EXI_CHAN_0, EXI_DEV_INT, NULL)) {
        EXISync(EXI_CHAN_0);
        EXIDeselect(EXI_CHAN_0);
        EXIUnlock(EXI_CHAN_0);
    }
    EXIUnlock(EXI_CHAN_0);
    while ((*(volatile u32*)0xCD00680C & 1) == 1) {
    }
    __OSSetExceptionHandler(8, OSDefaultExceptionHandler);
    GXAbortFrame();
    OSSetArenaLo((void*)0x81400000);
    arenaHi = *(void**)0x80000038;
    if (arenaHi == NULL) {
        OSSetArenaHi(*(void**)0x80003110);
    } else {
        OSSetArenaHi(arenaHi);
    }
    s_fatalParam.fg = fg;
    s_fatalParam.bg = bg;
    s_fatalParam.message = message;
    OSSwitchFiber(FatalScreenFiber, OSGetArenaHi());
}

/* 0x804CF990 (0x5E8): draws the fatal screen (fills the frame buffer, prints the message) and halts. */
static void FatalScreenFiber(void) {
    GXColor background;
    GXColor foreground;
    OSFontHeader* font;
    u8* frameBuffer;
    FatalParam* param;
    const char* message;
    s32 y;
    s32 x;
    u32 len;
    u8* p;

    OSEnableInterrupts();
    param = &s_fatalParam;
    message = param->message;
    len = strlen(message) + 1;
    param->message = memmove(OSAllocFromMEM1ArenaLo(len, 32), message, len);
    font = OSAllocFromMEM1ArenaLo(0xA1004, 32);
    OSInitFont(font, OSGetArenaLo());
    frameBuffer = OSAllocFromMEM1ArenaLo(0x96000, 32);
    background = RGBToYCbCr(param->bg);
    p = frameBuffer;
    for (y = 0; y < 480; y++) {
        for (x = 0; x < 320; x++) {
            p[0] = background.r;
            p[1] = background.g;
            p[2] = background.r;
            p[3] = background.b;
            p += 4;
        }
    }
    VISetNextFrameBuffer(frameBuffer);
    ConfigureFatalVideo(640, 480);
    VIFlush();
    WaitRetraces(2);
    foreground = RGBToYCbCr(param->fg);
    DrawFatalText(frameBuffer, 640, 480, foreground, 48, 100, font->leading, param->message);
    DCFlushRange(frameBuffer, 0x96000);
    VISetBlack(FALSE);
    VIFlush();
    WaitRetraces(1);
    OSDisableInterrupts();
    OSReport("%s\n", param->message);
    PPCHalt();
}
