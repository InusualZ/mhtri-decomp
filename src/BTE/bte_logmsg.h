/* BTE/bte_logmsg.h - the trace entry points `BTE/bte_logmsg.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTE_LOGMSG_H
#define MHTRI_BTE_BTE_LOGMSG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047A470 - formats a trace message, appends a newline and prints it. */
void LogMsg(u32 trace_set_mask, const char* fmt_str, ...);

/* 0x8047A530 / 0x8047A550 / 0x8047A570 / 0x8047A590 / 0x8047A5B0 - print a trace message with 0, 1, 2, 3 or 4
 * arguments; the mask carries the layer (bits 16-23) and the type. */
void LogMsg_0(u32 trace_set_mask, const char* fmt_str);
void LogMsg_1(u32 trace_set_mask, const char* fmt_str, u32 p1);
void LogMsg_2(u32 trace_set_mask, const char* fmt_str, u32 p1, u32 p2);
void LogMsg_3(u32 trace_set_mask, const char* fmt_str, u32 p1, u32 p2, u32 p3);
void LogMsg_4(u32 trace_set_mask, const char* fmt_str, u32 p1, u32 p2, u32 p3, u32 p4);

/* 0x8047A5D0 / 0x8047A5F0 - print a trace message with 5 or 6 arguments. */
void LogMsg_5(u32 trace_set_mask, const char* fmt_str, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5);
void LogMsg_6(u32 trace_set_mask, const char* fmt_str, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTE_LOGMSG_H */
