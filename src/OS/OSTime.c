/*
 * OS/OSTime.c - the OS time base: `OSGetTime`/`OSGetTick`, system time and the calendar conversions.
 * RANGE. .text 0x804D4D50-0x804D5420 (6 functions); .data 0x8061D678-0x8061D6D8.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the two month tables (.data 0x8061D678, 0x8061D6A8) are read only by
 *    `OSTicksToCalendarTime` and `OSCalendarTimeToTicks`.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. map names; GUESS: `YearDays` and `LeapYearDays` (the map had both month tables as `lbl_` rows: the cumulative day
 *    count before each month of a common / leap year).
 * RESIDUALS. `OSCalendarTimeToTicks` (45 % positional): the target saves r20-r31 and interleaves the usec/msec/second 64-bit products with
 *    the leap-year test; ours hoists the year product and saves fewer registers; the source tries (term grouping, evaluation order) did not
 *    reproduce it.  `OSTicksToCalendarTime` (99.8 %): the saved ticks halves take r26/r28 in the target and r28/r27 in ours.
 * SHAPES. `OSGetTime` and `OSGetTick` are asm functions (the time-base reads `mftb`/`mftbu` have no C spelling); the rest is C
 *    over 64-bit arithmetic (`__div2i`/`__mod2i` calls).
 */

#include "types.h"

#include "OS/OSInterrupt.h"
#include "OS/OS.h"
#include "OS/OSTime.h"
#include "OS/__OSGetSystemTime.h"

/* The system-time offset in low memory; a quarter of the bus clock is the time-base rate. */
#define OS_SYSTEM_TIME (*(s64*)0x800030D8)

#define OS_TIMER_CLOCK (OS_BUS_CLOCK / 4)

#define SECS_IN_DAY 86400
#define SECS_IN_YEAR 31536000
#define BIAS 0xB2575

/* The day count before each month of a common year and of a leap year. */
static s32 YearDays[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
static s32 LeapYearDays[12] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};

/* Reads the 64-bit time base, retrying across a carry into the upper half. */
asm s64 OSGetTime(void)
{
    nofralloc
    mftbu r3
    mftb r4
    mftbu r5
    cmpw r3, r5
    bne OSGetTime
    blr
}

/* Reads the low half of the time base. */
asm u32 OSGetTick(void)
{
    nofralloc
    mftb r3
    blr
}

/* Returns the time base plus the system-time offset. */
s64 __OSGetSystemTime(void)
{
    BOOL enabled = OSDisableInterrupts();
    s64 result = OSGetTime() + OS_SYSTEM_TIME;

    OSRestoreInterrupts(enabled);
    return result;
}

/* Converts a time-base value to system time by adding the offset. */
s64 __OSTimeToSystemTime(s64 time)
{
    BOOL enabled = OSDisableInterrupts();
    s64 result = OS_SYSTEM_TIME + time;

    OSRestoreInterrupts(enabled);
    return result;
}

/* Returns the number of leap days in the years before `year`. */
static inline s32 LeapDaysBefore(s32 year)
{
    if (year < 1) {
        return 0;
    }
    return (year + 3) / 4 - (year - 1) / 100 + (year - 1) / 400;
}

/* Returns the number of days from year 0 to the start of `year`. */
static inline s32 DaysBefore(s32 year)
{
    return year * 365 + LeapDaysBefore(year);
}

/* Returns whether `year` is a leap year. */
static inline BOOL IsLeapYear(s32 year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

/* Splits a time-base value into calendar fields. */
void OSTicksToCalendarTime(s64 ticks, OSCalendarTime* td)
{
    s32 days;
    s32 secs;
    s32 year;
    s32 yearDays;
    s32 yday;
    s32 mon;
    s64 d;
    const s32* table;

    d = ticks % OS_TIMER_CLOCK;
    if (d < 0) {
        d += OS_TIMER_CLOCK;
    }
    td->usec = (d * 8 / (OS_TIMER_CLOCK / 125000)) % 1000;
    td->msec = (d / (OS_TIMER_CLOCK / 1000)) % 1000;
    ticks -= d;
    ticks /= OS_TIMER_CLOCK;

    days = ticks / SECS_IN_DAY + BIAS;
    secs = ticks % SECS_IN_DAY;
    if (secs < 0) {
        days -= 1;
        secs += SECS_IN_DAY;
    }
    td->wday = (days + 6) % 7;

    for (year = days / 365; days < (yearDays = DaysBefore(year)); year--) {
    }
    yday = days - yearDays;
    td->year = year;
    td->yday = yday;

    table = IsLeapYear(year) ? LeapYearDays : YearDays;
    for (mon = 12; yday < table[--mon];) {
    }
    td->mon = mon;
    td->mday = yday - table[mon] + 1;

    td->hour = (secs / 60) / 60;
    td->min = (secs / 60) % 60;
    td->sec = secs % 60;
}

/* Combines calendar fields into a time-base value. */
s64 OSCalendarTimeToTicks(const OSCalendarTime* td)
{
    s64 secs;
    s64 ticks;
    s32 days;
    s32 year;
    s32 yearAdd;
    s32 leapDays;
    s32 mon;
    const s32* table;

    yearAdd = td->mon / 12;
    mon = td->mon % 12;
    if (mon < 0) {
        mon += 12;
        yearAdd--;
    }
    year = td->year + yearAdd;

    leapDays = LeapDaysBefore(year);
    table = IsLeapYear(year) ? LeapYearDays : YearDays;
    days = td->mday + leapDays + table[mon] - 1;

    secs = (s64)SECS_IN_YEAR * year + (s64)SECS_IN_DAY * days;
    secs += td->hour * 3600 + td->min * 60 + td->sec;
    secs -= (s64)BIAS * SECS_IN_DAY;
    return (s64)td->usec * (OS_TIMER_CLOCK / 125000) / 8 + (secs * OS_TIMER_CLOCK + (s64)td->msec * (OS_TIMER_CLOCK / 1000));
}
