/* Demo for idea 72: The declared SHAPE is a codegen input: a struct's exact size, an index's signedness, and arity
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count lwz 2 in copy_even            54-word struct: the copy loop moves two words per turn, nothing peeled
 * EXPECT: count lwz 3 in copy_odd             53-word struct: MWCC peels one word after the loop
 * EXPECT: count cmpw 1 in loop_s32            an s32 index compares with cmpw ...
 * EXPECT: count cmplw 1 in loop_u32           ... a u32 index with cmplw
 */
typedef unsigned int u32;
typedef int s32;

struct Even { u32 w[54]; };                     /* 0xD8 */
struct Odd { u32 w[53]; };                      /* 0xD4 */

extern "C" {

void sink(void *);

void copy_even(const Even *s)
{
    Even c;
    c = *s;
    sink(&c);
}

void copy_odd(const Odd *s)
{
    Odd c;
    c = *s;
    sink(&c);
}

u32 loop_s32(const u32 *t, s32 (*len)(void))
{
    u32 r = 0;
    for (s32 i = 0; i < len(); i++)
        r += t[i];
    return r;
}

u32 loop_u32(const u32 *t, s32 (*len)(void))
{
    u32 r = 0;
    for (u32 i = 0; i < len(); i++)
        r += t[i];
    return r;
}

}
