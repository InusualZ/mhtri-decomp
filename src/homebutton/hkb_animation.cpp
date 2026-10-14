/* homebutton/hkb_animation.cpp - the keyboard layout engine's animation code: the step/Hermite curve evaluators, an
 *   animation-transform class (constructor, destructor, resource set, pane/material bind and animate) and the
 *   animation-resource and bind helpers the layout builder calls.
 * RANGE. .text 0x80533854-0x80534D68 (26 functions); .data 0x8064A8F8-0x8064A920; .sdata2 0x8079D5F0-0x8079D610.
 *   Edges: the left edge is the pool duplicate 1.0 (0x8079D5EC in homebutton/hkb_drawinfo.cpp, 0x8079D5F8 here) and
 *   the `.sdata2` order (0x8079D5F0 is first read by 0x80533854).  The class table .data 0x8064A8F8 holds
 *   0x80533B38..0x805341FC and is stored by 0x80533B04; the next table 0x8064A920 belongs to
 *   homebutton/hkb_resource_accessor.cpp, whose three functions start at 0x80534D68.  The float 0.0 is pooled
 *   again at 0x8079D610 (read by 0x80535454), so the pool ends here.
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 * NAMES. GUESS: the file name, from the Hermite pool constants (-0.001, 0.001, 2, 3, -2) and the bind callees;
 *   every function keeps the map's generated name.
 * RESIDUALS. every body unwritten (stub).
 */
