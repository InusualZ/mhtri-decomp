/* homebutton/hkb_widget_tail.cpp - the last `.text` unit of the software keyboard: a widget class (constructor
 *   0x8056D814, table .data 0x80658458), a subclass of homebutton/hkb_layout.cpp's layout class (table .data
 *   0x80658480) whose slot 0x8056F02C constructs an object with table .data 0x806584C0 (it calls
 *   homebutton/hkb_animation.cpp's constructor 0x80533B04), and the helpers 0x8056F224..0x8056F2B4 the keyboard
 *   units call.
 * RANGE. .text 0x8056D814-0x8056F2B4 (17 functions); .rodata 0x8057C760-0x8057C808; .data 0x80658458-0x806584E4;
 *   .bss 0x80790DF8-0x80790E04; .sdata 0x80794750-0x80794754; .sdata2 0x8079D7B0-0x8079D7C8.
 *   Edges: the left edge follows homebutton/tiHKBManager.cpp, whose static initialiser 0x8056D78C (.ctors word
 *   0x8056F428) and destructor 0x8056D7D4 end it.  This unit has no `.ctors` word.
 *   0x8056D8DC is not a seam: the .bss object 0x80790DF8 is read only by 0x8056D838 and 0x8056D8DC, and the .sdata
 *   bytes 0x80794750/0x80794751 are one byte apart.  0x8056F02C is not proven: the class tables 0x80658458 ->
 *   0x80658480 rise (a zigzag, so a TU may start in 0x8056E93C..0x8056F02C), but no pool duplicate, string or
 *   private object confirms it, so the range stays one unit.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS; unmeasured here until bodies exist).
 * NAMES. GUESS: the file name, from its place after tiHKBManager.cpp; every function keeps the map's generated name.
 * RESIDUALS. every body unwritten (stub); the possible TU start in 0x8056E93C..0x8056F02C.
 */
