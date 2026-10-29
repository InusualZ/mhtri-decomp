/* sound/sound_obj_types.h - the sound player base classes `sound/sound_obj.cpp` defines and `sound/snd_stream_mgr.cpp`
 * derives the stream manager from (docs/plan.md 6.5 rule 1: a type two units use lives in one header). */
#ifndef MHTRI_SOUND_SOUND_OBJ_TYPES_H
#define MHTRI_SOUND_SOUND_OBJ_TYPES_H

#include "types.h"

#ifdef __cplusplus
/* The root of the sound players: its constructor installs the table at 0x80597DF8, whose slot 0 is its destructor
 * and whose other seven slots (zero in retail: pure) are the players' interface, not declared here. */
class SndBase {
public:
    SndBase() __attribute__((never_inline));
    virtual ~SndBase();
}; /* size: 0x4 (the vtable pointer) */

/* The player base the sound object and the stream manager derive from: its table is at 0x80597DD0, and its
 * constructor clears the word at +0x04. */
class SndPlayerBase : public SndBase {
public:
    SndPlayerBase() __attribute__((never_inline));
    virtual ~SndPlayerBase();

    /* +0x04 */ u32 flags;
}; /* size: 0x8 */
#endif

#endif
