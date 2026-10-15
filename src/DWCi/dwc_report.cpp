/*
 * DWCi/dwc_report.cpp - the DWC report printer: a category-filtered `OSReport` with a category prefix.
 *
 * RANGE. `.text` 0x805078F0..0x80507C40 (1 function); `.data` 0x8062FE10..0x8062FF90 (the category prefixes);
 *   `.sbss` 0x807957C8..0x807957D0 (the report level). The left `.data` edge is a seam: the target pads DWC_Init's
 *   strings 0x10C -> 0x110 before these prefixes, which one object does not emit.
 * FLAGS. `cflags_base` (the retail function starts 16-byte aligned); docs/network.md.
 * NAMES. `DWC_Printf` is the SDK's; GUESS on `stDwcReportLevel` and on the file name (the SDK's `dwc_report.c`;
 *   no `__FILE__` string).
 * RESIDUALS. none recorded.
 */
#include "DWCi/dwc_report.h"
#include "OS/OSVReport.h"
#include "stdarg.h"
#include "unsplit/OS.h"

extern "C" {

u32 stDwcReportLevel;

/* 0x805078F0 (0x348): prints a report line with its category prefix when the category is enabled. */
void DWC_Printf(u32 level, const char* format, ...) {
    va_list args;
    if (level & stDwcReportLevel) {
        switch ((s32)level) {
        case 0x1:
            OSReport("DWC_INFO     :");
            break;
        case 0x2:
            OSReport("++DWC_ERROR  :");
            break;
        case 0x4:
            OSReport("DWC_DEBUG    :");
            break;
        case 0x8:
            OSReport("DWC_WARN     :");
            break;
        case 0x10:
            OSReport("DWC_ACHECK   :");
            break;
        case 0x20:
            OSReport("DWC_LOGIN    :");
            break;
        case 0x40:
            OSReport("DWC_MATCH_NN :");
            break;
        case 0x80:
            OSReport("DWC_MATCH_GT2:");
            break;
        case 0x100:
            OSReport("DWC_TRANSPORT:");
            break;
        case 0x200:
            OSReport("DWC_QR2_REQ  :");
            break;
        case 0x400:
            OSReport("DWC_SB_UPDATE:");
            break;
        case 0x8000:
            OSReport("DWC_SEND     :");
            break;
        case 0x10000:
            OSReport("DWC_RECV     :");
            break;
        case 0x20000:
            OSReport("DWC_UPDATE_SV:");
            break;
        case 0x40000:
            OSReport("DWC_CONNECTINET:");
            break;
        case 0x1000000:
            OSReport("DWC_AUTH     :");
            break;
        case 0x2000000:
            OSReport("DWC_AC       :");
            break;
        case 0x4000000:
            OSReport("DWC_BM       :");
            break;
        case 0x8000000:
            OSReport("DWC_UTIL     :");
            break;
        case 0x10000000:
            OSReport("DWC_OPTION_CF:");
            break;
        case 0x20000000:
            OSReport("DWC_OPTION_CONNTEST:");
            break;
        case (s32)0x80000000:
            OSReport("DWC_GAMESPY  :");
            break;
        default:
            OSReport("DWC_UNKNOWN  :");
            break;
        }
        va_start(args, format);
        OSVReport(format, args);
        va_end(args);
    }
}

}
