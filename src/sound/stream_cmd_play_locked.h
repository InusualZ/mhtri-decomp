/* sound/stream_cmd_play_locked.h - leaf: `sound/fn_800E46E8.cpp`'s 0x800E4D00, which plays stream command `cmd` on `slot` with interrupts off
 * (`stream_play_cmd(stream_mgr_get(), slot, cmd, 0)` between OSDisableInterrupts/OSRestoreInterrupts). */
#ifndef MHTRI_SOUND_STREAM_CMD_PLAY_LOCKED_H
#define MHTRI_SOUND_STREAM_CMD_PLAY_LOCKED_H

#include "types.h"
#include "sound/snd_stream_mgr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void stream_cmd_play_locked(u32 slot, StreamCmd* cmd);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_STREAM_CMD_PLAY_LOCKED_H */
