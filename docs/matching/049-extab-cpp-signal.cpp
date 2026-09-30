/* Demo for idea 49: extab in a no-exceptions lib is a cheap C++ language signal (scoped: it is an EXCEPTION-MODE signal)
 * FLAGS: -O4,p -inline auto -Cpp_exceptions off -lang c
 * MWCC: Wii/1.3
 * EXPECT: section extab 8           a plain C function still gets an extab record once exceptions are switched on locally
 * EXPECT: section extabindex 12     ... and its extabindex entry: the sections follow the exception MODE, not the language
 * EXPECT: contains plain_c
 */
#pragma exceptions on
extern void use(int *p);

void plain_c(int n)
{
    int v = n;
    use(&v);
}
