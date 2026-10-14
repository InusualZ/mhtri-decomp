/* homebutton/hkb_arc_resource_accessor.cpp - the keyboard layout engine's archive resource accessor: name lookup
 *   over an ARC directory ("." and ".." skipped), the resource getters, the font-link list and the class
 *   constructor/destructor.
 * RANGE. .text 0x80534DC0-0x805352DC (11 functions); .data 0x8064A938-0x8064A950; .sdata 0x807944B8-0x807944C0.
 *   Edges: 0x80534DC0 is called only by 0x80534E8C, which is called only by the table slot 0x805351C0; the class
 *   table .data 0x8064A938 holds 0x805350FC, 0x805351C0 and 0x8053526C and is stored by the constructor 0x8053508C,
 *   which calls the base constructor 0x80534DA8.  The next `.data` object (0x8064A950) is a pointer table read from
 *   0x80536F08 on (a table followed by data, the V->S seam), so the archive class ends the TU; the cut sits after
 *   its last method 0x8053526C, and nothing in 0x805352DC..0x80535454 reads data that places it more exactly.
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 * NAMES. GUESS: the file name, from the ARC calls and the "."/".." strings; every function keeps the map's
 *   generated name.
 * RESIDUALS. every body unwritten (stub).
 */
