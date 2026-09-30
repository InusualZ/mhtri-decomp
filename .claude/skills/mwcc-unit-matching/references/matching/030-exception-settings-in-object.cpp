/* Demo for idea 30: A C++ unit's exception settings live in its object, not in the source
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: section extab 0x18            the base flags carry -Cpp_exceptions off; the pragma alone brings the unwind record back
 * EXPECT: section extabindex 0xc
 * EXPECT: size with_cleanup 0x30         the same 0x30 B of .text as with `#pragma exceptions off` (which emits no extab at all)
 */
#pragma exceptions on

struct Guard {
    int v;
    ~Guard();
};

extern "C" {

void work(int v);

void with_cleanup(int a)
{
    Guard g;
    g.v = a;
    work(a);
}

}
