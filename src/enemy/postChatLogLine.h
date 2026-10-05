/* enemy/postChatLogLine.h - the declaration of `postChatLogLine`, which `enemy/em_prog_support.cpp` owns (docs/plan.md 6.5
 * rule 2, a leaf header: the owner's full header pulls `note_work.h` -> `sound/mhchar.h`, whose GX enums collide with the
 * network units).  The signature is the one its callers use. */
#ifndef MHTRI_ENEMY_POSTCHATLOGLINE_H
#define MHTRI_ENEMY_POSTCHATLOGLINE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
/* 0x80383BA8 - posts one chat-log line (kind, member, colour, text, id text, name) unless the sender is muted, with
 * sound 0x18 (GUESS name: its callers post member joined/left lines and chat records through it). */
void postChatLogLine(u8 kind, s8 member, s32 color, const char* text, const char* id_text, const char* name);
#ifdef __cplusplus
}
#endif

#endif
