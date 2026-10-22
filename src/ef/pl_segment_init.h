/* ef/pl_segment_init.h - the declarations of `pl_segment_init`, which `ef/effect.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em_common.cpp` calls them through. */
#ifndef MHTRI_EF_PL_SEGMENT_INIT_H
#define MHTRI_EF_PL_SEGMENT_INIT_H

#include "types.h"

struct PlSegment;

#ifdef __cplusplus
extern "C" {
#endif
void pl_segment_init(struct PlSegment* seg);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_PL_SEGMENT_INIT_H */
