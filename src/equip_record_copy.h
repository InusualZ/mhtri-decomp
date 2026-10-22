/* `equip_record_copy` (0x8004A20C), defined by `userdata_item.cpp` (leaf header, rule 2): the same declaration as
 * `userdata_item.h`, for a consumer that also includes `menu/menu_item_page.h` (whose `userdata_item_count_total`
 * disagrees with `userdata_item.h`'s). */
#ifndef MHTRI_EQUIP_RECORD_COPY_H
#define MHTRI_EQUIP_RECORD_COPY_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* Copies one 12-byte equipment record field by field; returns `dst`. */
struct _EQUIP* equip_record_copy(struct _EQUIP* dst, const struct _EQUIP* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EQUIP_RECORD_COPY_H */
