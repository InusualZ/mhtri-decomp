/* Demo for idea 73: Never append `, ...` to a definition to dodge an argument-count mismatch
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: size fixed 4              a fixed unused parameter: the retail thunk, one `blr`
 * EXPECT: size variadic 0x50        `, ...` buys a full varargs prologue (register save area) for nothing
 * EXPECT: contains stfd             the FP argument registers are spilled too
 */
typedef int s32;

extern "C" {

void fixed(s32 a, void *unused)
{
}

void variadic(s32 a, ...)
{
}

}
