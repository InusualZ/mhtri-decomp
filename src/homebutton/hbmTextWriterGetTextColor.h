/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/hbm_text_panel.cpp`'s `hbmTextWriterGetTextColor`, the one
 * declaration `homebutton/hbm_value.cpp` needs. */
#ifndef MHTRI_HOMEBUTTON_HBMTEXTWRITERGETTEXTCOLOR_H
#define MHTRI_HOMEBUTTON_HBMTEXTWRITERGETTEXTCOLOR_H

#include "types.h"
#include "nw4r/ut_CharWriter.h"

extern "C" {

/* 0x80547B34 (0x24): Returns the text writer's text colour (CharWriter::mTextColor.start). */
nw4r::ut::Color hbmTextWriterGetTextColor(const nw4r::ut::CharWriter* writer);
}

#endif /* MHTRI_HOMEBUTTON_HBMTEXTWRITERGETTEXTCOLOR_H */
