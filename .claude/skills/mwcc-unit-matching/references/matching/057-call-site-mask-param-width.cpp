/* Demo for idea 57: A call-site mask comes from the callee's declared parameter type and the value's own type
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count clrlwi 1 in pass_to_narrow   callee declared u16: the caller masks its u32 argument
 * EXPECT: count clrlwi 0 in pass_to_wide     callee declared u32: the argument goes through as it is
 * EXPECT: size pass_to_narrow 8
 * EXPECT: size pass_to_wide 4
 * EXPECT: count extsh 1 in sum_to_s16        an int value into an s16 parameter is sign-extended by the caller ...
 * EXPECT: count extsh 0 in sum_to_s32        ... and not when the parameter is int
 * EXPECT: count extsh 1 in s16_value_to_s32  but an s16-TYPED value is re-extended even for an int parameter: widening the callee cannot help there
 */
typedef short s16;
typedef unsigned short u16;
typedef unsigned int u32;

extern "C" {

void take_narrow(u16 value);
void take_wide(u32 value);
void take_s16(s16 value);
void take_s32(int value);

void pass_to_narrow(u32 v)
{
    take_narrow(v);
}

void pass_to_wide(u32 v)
{
    take_wide(v);
}

void sum_to_s16(int a, int b)
{
    take_s16(a + b);
}

void sum_to_s32(int a, int b)
{
    take_s32(a + b);
}

void s16_value_to_s32(s16 v)
{
    take_s32(v);
}

}
