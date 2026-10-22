/* `word_read`, defined by `pad_connect.cpp` (leaf header, rule 2). */
#ifndef MHTRI_WORD_READ_H
#define MHTRI_WORD_READ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80047234 (0x8): returns the word at src. */
s32 word_read(s32* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_WORD_READ_H */
