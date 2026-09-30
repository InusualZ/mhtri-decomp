/* Demo for idea 16: Scope optimizer settings per function with pragmas
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count srawi. 1 in with_peep         peephole on: the entry test is a record-form shift
 * EXPECT: count srawi. 0 in no_peep           `#pragma peephole off` scopes it to this one function
 * EXPECT: count srawi. 1 in with_peep_again   `#pragma peephole on` restores it for the next function
 * EXPECT: size level1_at_head 0x2c            optimization_level 1 before the function: not unrolled
 * EXPECT: size level1_mid_body 0x2c           the same pragma written mid-body: identical, it applies to the whole function
 * EXPECT: size with_peep 0x88                 the same source at level 4: unrolled by 8
 */
extern "C" {

int with_peep(int *p, int x)
{
    int n = x >> 2;
    int s = 0;
    while (n-- > 0)
        s += *p++;
    return s;
}

#pragma peephole off
int no_peep(int *p, int x)
{
    int n = x >> 2;
    int s = 0;
    while (n-- > 0)
        s += *p++;
    return s;
}
#pragma peephole on

int with_peep_again(int *p, int x)
{
    int n = x >> 2;
    int s = 0;
    while (n-- > 0)
        s += *p++;
    return s;
}

#pragma optimization_level 1
int level1_at_head(int *p, int x)
{
    int n = x >> 2;
    int s = 0;
    while (n-- > 0)
        s += *p++;
    return s;
}
#pragma optimization_level 4

int level1_mid_body(int *p, int x)
{
    int n = x >> 2;
    int s = 0;
#pragma optimization_level 1
    while (n-- > 0)
        s += *p++;
    return s;
}
#pragma optimization_level 4

}
