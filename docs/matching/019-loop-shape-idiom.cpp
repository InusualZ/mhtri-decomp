/* Demo for idea 19: Loop shape decides the loop idiom
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: insn srawi. r5,r4,2 in down_while   the count is computed with a record-form shift: no separate compare
 * EXPECT: count cmpwi 0 in down_while          the entry test is that record form + `ble`, then `mtctr`/`bdnz`
 * EXPECT: contains bdnz                         all three shapes still end in a counted loop
 * EXPECT: size down_while 0x88                  the `while (count-- > 0)` shape is the smallest ...
 * EXPECT: size down_for 0xb0                    ... the down-counting `for` is NOT the same code (extra compares)
 * EXPECT: size up_for 0xd8                      ... and the up-counting indexed `for` is larger still
 */
extern "C" {

int down_while(const int *p, int n)
{
    int s = 0;
    int count = n >> 2;
    while (count-- > 0)
        s += *p++;
    return s;
}

int down_for(const int *p, int n)
{
    int s = 0;
    int i;
    int count = n >> 2;
    for (i = count; i > 0; i--)
        s += *p++;
    return s;
}

int up_for(const int *p, int n)
{
    int s = 0;
    int i;
    int count = n >> 2;
    for (i = 0; i < count; i++)
        s += p[i];
    return s;
}

}
