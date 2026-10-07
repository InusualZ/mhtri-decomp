/*
 * Runtime.PPCEABI.H/exit.h - the declaration of `exit`, owned by `Runtime.PPCEABI.H/__init_user.c`.
 */
#ifndef RUNTIME_EXIT_H
#define RUNTIME_EXIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D8060 (0x4C): runs the exit hooks and stops the program with `status`. */
void exit(s32 status);

#ifdef __cplusplus
}
#endif

#endif
