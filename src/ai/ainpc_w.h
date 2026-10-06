/*
 * ai/ainpc_w.h - `ainpc_w`, the AI companion record `ai/ai_npc.cpp` defines (`.bss` 0x806BD360; its `.ctors` word
 *   `fn_802D9E14` runs the record's constructor).  `_AINPC_W` is only forward-declared here, so a unit with its own view
 *   of the record can name the object.
 */
#ifndef MHTRI_AI_AINPC_W_H
#define MHTRI_AI_AINPC_W_H

extern "C" struct _AINPC_W ainpc_w;

#endif /* MHTRI_AI_AINPC_W_H */
