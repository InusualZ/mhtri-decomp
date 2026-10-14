/*
 * homebutton/hbm_kb_cursor.h - declarations of the symbols owned by `homebutton/hbm_kb_cursor.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_KB_CURSOR_H
#define HOMEBUTTON_HBM_KB_CURSOR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char lbl_80657E14[];

extern char lbl_80794728[8];

extern char lbl_80794738[8];

extern f32 lbl_8079D750;

extern f32 lbl_8079D754;

extern f32 lbl_8079D774;

extern f32 lbl_8079D784;

/* The unit's .sdata2 pool entries the scroll handlers read (declared, never defined: playbook 29). */
extern f32 hbmKbCursorScrollFrames;    /* 0x8079D760: 15.0, the scroll animation length */

extern f32 hbmKbCursorScrollRows;      /* 0x8079D764: 3.0, the rows one arrow press scrolls */

extern f32 hbmKbCursorTwoRows;         /* 0x8079D770: 2.0 */

#ifdef __cplusplus
}
#endif

#endif
