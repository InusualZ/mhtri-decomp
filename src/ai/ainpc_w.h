/*
 * `ainpc_w` - the one declaration of the AI-NPC work record (rule 2), defined by `src/ai/fn_802D44F4.cpp` (its
 * `.bss` 0x806BD360-0x806BDA58; the unit's static constructor `fn_802D9E14` runs the record's constructor over
 * it).  The record type `_AINPC_W` is shared by the `ai` band and lives in `ai/ainpc.h` (rule 1); this header only
 * forward-declares it, so a unit that carries its own view of the record can still name the object.
 */
#ifndef MHTRI_AI_AINPC_W_H
#define MHTRI_AI_AINPC_W_H

extern "C" struct _AINPC_W ainpc_w;

#endif /* MHTRI_AI_AINPC_W_H */
