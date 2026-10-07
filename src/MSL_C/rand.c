/*
 * MSL_C/rand.c - `rand` and `srand` over the 8-byte seed.
 *
 * RANGE. .text 0x8045DFA0..0x8045DFC8 (2 functions in the map, 0x28 B); .sdata 0x80793CE0..0x80793CE8.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `rand`); `srand` is the map's name and a GUESS (the dump labels its address with a neighbour); `rand_next` is GUESS (the seed word the two functions share).
 * EVIDENCE. `.sdata` 0x80793CE0 (8 B) is read only by these two functions and sits between the printf unit's
 *    `@wstringBase0` (0x80793CD8) and the string unit's `strtok` statics (0x80793CE8).
 * RESIDUALS. none measured.
 * SHAPES. linear congruential generator, multiplier 1103515245, increment 12345, result bits 16..30.
 */
#include "types.h"

static u32 rand_next = 1;

int rand(void)
{
    rand_next = rand_next * 1103515245 + 12345;
    return (rand_next >> 16) & 0x7FFF;
}

void srand(u32 seed)
{
    rand_next = seed;
}
