/* Demo for idea 80: A TU's .data is globals, strings, vtables in reverse, then inline-function strings
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: order .data g_first < g_second      initialised globals over 8 B, in definition order
 * EXPECT: order .data g_second < __vt__1B     then the strings, then the vtables ...
 * EXPECT: order .data __vt__1B < __vt__1A     ... in the reverse of class order (B was defined after A)
 * EXPECT: order .data __vt__1A < @STRING@inline_log__Fi   the inline function's string comes last
 * EXPECT: absent @stringBase0
 */
extern "C" void puts_(const char *s);
extern "C" void log_(const char *s, int n);

int g_first[6] = {1, 2, 3, 4, 5, 6};
int g_second[6] = {7, 8, 9, 10, 11, 12};

struct A {
    virtual void first();
    virtual void second();
    int a;
};

struct B {
    virtual void first();
    virtual void second();
    int b;
};

inline void inline_log(int n)
{
    log_("inline string", n);
}

void A::first()  { puts_("A first"); inline_log(1); }
void A::second() { puts_("A second"); }
void B::first()  { puts_("B first"); inline_log(2); }
void B::second() { puts_("B second"); }
