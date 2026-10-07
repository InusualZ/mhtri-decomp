/*
 * TRK/trk_report.h - how the MetroTRK units call `OSReport`: through fixed-argument prototypes, so the call carries no
 *    variadic `crclr` (the retail MetroTRK code declared the report function without a variable argument list).
 */
#ifndef TRK_TRK_REPORT_H
#define TRK_TRK_REPORT_H

#include "types.h"
#include "OS/OSError.h"

/* Reports a fixed message. */
#define TRK_REPORT(message) (((void (*)(const char*))OSReport)(message))

/* Reports a message with one integer argument. */
#define TRK_REPORT_S32(format, value) (((void (*)(const char*, s32))OSReport)((format), (value)))

#endif
