/* enemy/em_act_step_tail.cpp - the tail of the enemy work record's action-step band: four small bodies after
 *   `enemy/em_act_step.cpp`'s last step function.
 * RANGE. .text 0x80330194-0x8033041C (4 functions); extab, extabindex.  `enemy/em_act_step.cpp` keeps the band's
 *   `.data`; `hud/pl_frame_sync.cpp` follows at 0x8033041C.
 * NAMES. `em_act_step_tail` is a GUESS from the band's role and its neighbour.
 * RESIDUALS. 4 rows unwritten: 0x80330194-0x8033041C (the whole range); the internal seams are unknown.
 *   flipcheck: the object emits none of the claimed sections (`.text`, extab, extabindex).
 */
