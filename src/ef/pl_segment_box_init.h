/* ef/pl_segment_box_init.h - the declarations of `pl_segment_box_init`, which `ef/em_effect_ctrl.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em_common.cpp` calls them through. */
#ifndef MHTRI_EF_PL_SEGMENT_BOX_INIT_H
#define MHTRI_EF_PL_SEGMENT_BOX_INIT_H

#include "types.h"

struct PlSegmentBox;

#ifdef __cplusplus
extern "C" {
#endif
void pl_segment_box_init(struct PlSegmentBox* box);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_PL_SEGMENT_BOX_INIT_H */
