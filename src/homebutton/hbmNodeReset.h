/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/hbm_text_panel.cpp`'s `hbmNodeReset`, the one declaration
 * `homebutton/hbm_kb_cursor.cpp` needs from it.  GUESS name (registry, lane hbm-kb-cursor): the body clears the node's
 * argument and hands it to its target's slot 0x30. */
#ifndef MHTRI_HOMEBUTTON_HBMNODERESET_H
#define MHTRI_HOMEBUTTON_HBMNODERESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8054F788 (0x34): clears the node's argument and hands it to the node target's reset. */
/* untyped: opaque node record, typed by the callers' views */
void hbmNodeReset(void* record);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HOMEBUTTON_HBMNODERESET_H */
