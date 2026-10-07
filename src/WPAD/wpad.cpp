/* WPAD/wpad.cpp - the Wii remote driver: control block, connect/disconnect, speaker, stream data and the report queue.
 * RANGE. .text 0x804EB510-0x804F9B30 (131 functions); .rodata 0x80573C80-0x80573CD8; .data 0x8062BF30-0x8062DBF0;
 *   .bss 0x8075B2A0-0x8075EAA0; .sdata 0x80794198-0x807941C8; .sbss 0x807956A8-0x80795720; .sdata2 0x8079D3C8-0x8079D478.
 *   Edges: the "<< RVL_SDK - WPAD ... Jun 22 2009 ... (0x4302_145) >>" build string (.data 0x8062BF30) and the
 *   `WBCReadDummy`/`WBCSetZEROPointDummy`/`WBCGetTGCWeightDummy` print strings follow it before any other string, so
 *   the nine 8-byte stubs and the three dummies at 0x804EB510..0x804EB630 belong to this TU; the right edge 0x804F9B30
 *   is the first reader of the WUD state (.sbss 0x80795738, .bss 0x8075EAA0).
 * FLAGS. `cflags_base` (-O4,p, 16-byte function alignment): every function start is 16-aligned.
 * NAMES. the map's names (`WPADInit`, `WPADDisconnect`, `WPADProbe`, `WPADControlSpeaker`, `WPADSendStreamData`,
 *   `WPADiSendWriteData`, `WPADiSendSetReportType`, `WPADiClearQueue`, ...); the other functions are generated.
 *   GUESS names: `WPADiDebugPrint` (an empty varargs sink every log line calls), `WPADiNullCallbackA`, `WPADiNullCallbackB`,
 *   `WPADiGetReserved0`, `WPADiGetReserved1`, `WPADiGetReserved2`, `WPADiGetReserved3`, `WPADiReturnZeroA`, `WPADiReturnZeroB`,
 *   `WPADiReturnZeroC`, `WBCReadDummy`, `WBCSetZEROPointDummy`, `WBCGetTGCWeightDummy` (the last three from their print strings).
 * RESIDUALS. Flip blockers: .bss 0x3800, .data (0x3E of 0x1CC0), .rodata 0x58, .sdata 0x30 and .sbss (0x10 of 0x78) are claimed
 *   but the written bodies emit almost none of them.  `WPADiDebugPrint` is an empty varargs body by design (retail 0x50 bytes, the
 *   same empty body).  Written: the nine 8-byte stubs 0x804EB510..0x804EB590, the three dummies and the log sink (13 of 131); the
 *   other 118 bodies (0x804EB630 onward) are not attempted.  The TU is probably two: the .sdata2 pool holds the double 4330000080000000
 *   twice (0x8079D418 read by 0x804F3150/0x804F5050/0x804F54F0, 0x8079D460 read by 0x804F84E0/0x804F8C00/0x804F8D30) and
 *   the float 0 twice (0x8079D3E0, 0x8079D46C); the second TU starts after 0x804F5768 and no later than 0x804F8380
 *   (the first reader of 0x8079D430), and the 17 functions 0x804F5770..0x804F8380 read no pool entry, so the seam is
 *   not pinned and the unit is kept whole.
 */

#include "types.h"
#include "WPAD/wpad.h"

static u32 wpadReserved0;
static u32 wpadReserved1;
static u32 wpadReserved2;
static u32 wpadReserved3;

/* The null callback pair handed to the WUD layer when the stack starts. */
s32 WPADiNullCallbackA(void)
{
    return 0;
}

s32 WPADiNullCallbackB(void)
{
    return 0;
}

u32 WPADiGetReserved0(void)
{
    return wpadReserved0;
}

u32 WPADiGetReserved1(void)
{
    return wpadReserved1;
}

u32 WPADiGetReserved2(void)
{
    return wpadReserved2;
}

u32 WPADiGetReserved3(void)
{
    return wpadReserved3;
}

s32 WPADiReturnZeroA(void)
{
    return 0;
}

s32 WPADiReturnZeroB(void)
{
    return 0;
}

s32 WPADiReturnZeroC(void)
{
    return 0;
}

/* The balance board read stub: logs and fails. */
s32 WBCReadDummy(void)
{
    WPADiDebugPrint("WBCReadDummy\n");
    return -1;
}

s32 WBCSetZEROPointDummy(void)
{
    WPADiDebugPrint("WBCSetZEROPointDummy\n");
    return -1;
}

s32 WBCGetTGCWeightDummy(void)
{
    WPADiDebugPrint("WBCGetTGCWeightDummy\n");
    return -1;
}

/* Takes the format of a log line and discards it; every WPAD log call goes through here. */
void WPADiDebugPrint(const char* format, ...)
{
}
