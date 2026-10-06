/*
 * Network/NetworkConnectionStable.cpp - `NetworkConnectionStable`, the connection a `NetworkSessionStable` slot owns
 *   (its log strings: `NetworkConnectionStable[%d] ...`), the slot queues it embeds (`NetworkSlotQueues`), the send pool
 *   and the frame writer's entry points.
 * RANGE. .text 0x803CA484-0x803CCDF8 (53 functions), extab 0x80019618-0x800198C8, extabindex 0x80039EF4-0x8003A0BC,
 *   .data 0x805F91F0-0x805F94E0 (the log strings and the class's table 0x805F9490).  Left edge: the V->S seam at
 *   0x805F91F0; the `.text` edge 0x803CA484 is a GUESS (`Network/NetworkConnection.cpp`).  Right edge:
 *   `Network/NetworkPeerBase.cpp` at 0x803CCDF8.
 * FLAGS. the `NetworkStreamSink` row's flags, measured on the two `NetworkSlotQueues` rows only.
 * NAMES. The file name is the class's (the log strings).
 * RESIDUALS. Written: `NetworkSlotQueues`'s constructor and destructor.  The class's own methods are unwritten: it is
 *   declared in `Network/NetworkSessionStable.h` with an opaque `pad_04[0x224C]` and no base.  The `NetworkStreamWriter`
 *   constructors and destructors (0x803CB8FC..0x803CBA2C) are not written: their tables (0x805FCDD4, 0x805FCE10) lie in
 *   another unit's `.data`, so defining the key functions here would emit them in the wrong TU.
 */

#include "Network/NetworkConnectionStable.h"
#include "Network/NetworkSessionStable.h"   /* NetworkConnectionStable, NetworkSlotQueues */

#pragma peephole off

/* ==== the slot queues ========================================================================================= */

/* Destroys the two receive queues and the two send queues. */
NetworkSlotQueues::~NetworkSlotQueues()
{
}

/* Builds the two send queues and the two receive queues. */
NetworkSlotQueues::NetworkSlotQueues()
{
}
