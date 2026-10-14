/*
 * BTE/bte_logmsg.c - the trace output of the Broadcom BTE Bluetooth stack: a formatter that prints through OSReport
 *   and the LogMsg_0..LogMsg_6 entry points that call it unless tracing is off.
 *
 * RANGE. .text 0x8047A470-0x8047A610 (8 functions); .bss 0x80737960-0x80738140; .sdata 0x80793D68-0x80793D70;
 *   .sbss 0x80795028-0x8079502C. Strong start: 0x8047A470 formats into the 0x7E0 buffer 0x80737960 with vsprintf,
 *   appends "%s\n" (.sdata 0x80793D68) and prints it; LogMsg_0..LogMsg_6 tail-call it and read the trace switch
 *   0x80795028. Its .bss follows BTE/uusb_ppc.c's buffers, its .sdata BTE/bta_sys_cfg.c's pointer and its .sbss
 *   BTE/bte_hcisu.c's. Medium end: 0x8047A610, the alarm handler whose address only BTA_Init takes, and its fiber
 *   stack 0x80738140 go with BTE/bta_init.c as a static pair; the 4-byte .sdata hole 0x80793D6C before BTA_Init's
 *   0x80793D70 is alignment of that 8-byte object.
 * FLAGS. Wii/1.3 with `cflags_base` (-O4,p, 16-byte function alignment), as BTE/gki_time.c measured; the lib default
 *   `cflags_os` packs functions to 4 bytes.
 * NAMES. the map's LogMsg_0..LogMsg_6. GUESS: 0x8047A470 LogMsg (Broadcom's formatter of bte_logmsg.c),
 *   0x80737960 bte_logmsg_buffer (the 0x7E0-byte line buffer), 0x80795028 bte_logmsg_mute (a byte that suppresses
 *   every trace when non-zero; nothing in the DOL sets it).
 * RESIDUALS. none known in the bodies.
 * SHAPES. plain C, variadic; "%s\n" is a string literal MWCC places in .sdata. The LogMsg_* declarations live in
 *   BTE/bte_logmsg.h.
 */

#include "BTE/bte_logmsg.h"
#include "MSL_C/printf.h"
#include "OS/OSError.h"

/* .bss 0x80737960 - the line the trace is formatted into. */
char bte_logmsg_buffer[0x7E0];

/* .sbss 0x80795028 - suppresses every trace while non-zero. */
u8 bte_logmsg_mute;

/* 0x8047A470 (0xB4): Formats a trace message, appends a newline and prints it. */
void LogMsg(u32 trace_set_mask, const char* fmt_str, ...) {
    va_list ap;

    va_start(ap, fmt_str);
    vsprintf(bte_logmsg_buffer, fmt_str, ap);
    va_end(ap);
    sprintf(bte_logmsg_buffer, "%s\n", bte_logmsg_buffer);
    OSReport(bte_logmsg_buffer);
}

/* 0x8047A530 (0x18): Prints a trace message with no argument. */
void LogMsg_0(u32 trace_set_mask, const char* fmt_str) {
    if (bte_logmsg_mute == 0) {
        LogMsg(trace_set_mask, fmt_str);
    }
}

/* 0x8047A550 (0x18): Prints a trace message with one argument. */
void LogMsg_1(u32 trace_set_mask, const char* fmt_str, u32 p1) {
    if (bte_logmsg_mute == 0) {
        LogMsg(trace_set_mask, fmt_str, p1);
    }
}

/* 0x8047A570 (0x18): Prints a trace message with two arguments. */
void LogMsg_2(u32 trace_set_mask, const char* fmt_str, u32 p1, u32 p2) {
    if (bte_logmsg_mute == 0) {
        LogMsg(trace_set_mask, fmt_str, p1, p2);
    }
}

/* 0x8047A590 (0x18): Prints a trace message with three arguments. */
void LogMsg_3(u32 trace_set_mask, const char* fmt_str, u32 p1, u32 p2, u32 p3) {
    if (bte_logmsg_mute == 0) {
        LogMsg(trace_set_mask, fmt_str, p1, p2, p3);
    }
}

/* 0x8047A5B0 (0x18): Prints a trace message with four arguments. */
void LogMsg_4(u32 trace_set_mask, const char* fmt_str, u32 p1, u32 p2, u32 p3, u32 p4) {
    if (bte_logmsg_mute == 0) {
        LogMsg(trace_set_mask, fmt_str, p1, p2, p3, p4);
    }
}

/* 0x8047A5D0 (0x18): Prints a trace message with five arguments. */
void LogMsg_5(u32 trace_set_mask, const char* fmt_str, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5) {
    if (bte_logmsg_mute == 0) {
        LogMsg(trace_set_mask, fmt_str, p1, p2, p3, p4, p5);
    }
}

/* 0x8047A5F0 (0x18): Prints a trace message with six arguments. */
void LogMsg_6(u32 trace_set_mask, const char* fmt_str, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6) {
    if (bte_logmsg_mute == 0) {
        LogMsg(trace_set_mask, fmt_str, p1, p2, p3, p4, p5, p6);
    }
}
