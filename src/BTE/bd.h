/* BTE/bd.h - the device address type and the address helpers `BTE/bd.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BD_H
#define MHTRI_BTE_BD_H

#include "types.h"

#define BD_ADDR_LEN 6

/* A Bluetooth device address, most significant byte first. */
typedef u8 BD_ADDR[BD_ADDR_LEN];

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047ABC4 / 0x8047ABF8 - copy a device address, or compare two (0 when equal, else -1). */
void bdcpy(BD_ADDR a, const BD_ADDR b);
int bdcmp(const BD_ADDR a, const BD_ADDR b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BD_H */
