/* Demo for idea 57: A call-site mask means the callee's parameter is declared wider than the value
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count clrlwi 1 in pass_to_narrow   callee declared u16: the caller masks its u32 argument
 * EXPECT: count clrlwi 0 in pass_to_wide     callee declared u32: the argument goes through as it is
 * EXPECT: size pass_to_narrow 8
 * EXPECT: size pass_to_wide 4
 */
typedef unsigned short u16;
typedef unsigned int u32;

extern "C" {

void take_narrow(u16 value);
void take_wide(u32 value);

void pass_to_narrow(u32 v)
{
    take_narrow(v);
}

void pass_to_wide(u32 v)
{
    take_wide(v);
}

}
