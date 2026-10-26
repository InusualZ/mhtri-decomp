/* `eft052_page_counts_get`, defined by `ef/eft052.cpp` (leaf header, rule 2). */
#ifndef MHTRI_EF_EFT052_PAGE_COUNTS_GET_H
#define MHTRI_EF_EFT052_PAGE_COUNTS_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8035902C - the held, box and pouch counts of item `id`. */
u32 eft052_page_counts_get(u16 id, u32* held, u32* box, u32* pouch);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT052_PAGE_COUNTS_GET_H */
