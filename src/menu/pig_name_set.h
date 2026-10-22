/* menu/pig_name_set.h - the declarations of `pig_name_set`, which `menu/menu_placeinfo.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em020_prog.cpp` calls them through. */
#ifndef MHTRI_MENU_PIG_NAME_SET_H
#define MHTRI_MENU_PIG_NAME_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void pig_name_set(const char* name);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PIG_NAME_SET_H */
