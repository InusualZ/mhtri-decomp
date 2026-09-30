/* Demo for idea 48: A C++ unit's unmangled map name is not a reason for extern "C"
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: contains Pl_Skill_ck__FUs     a C++ free function is emitted under its MANGLED name (u16 -> Us)
 * EXPECT: contains Pl_Skill_ck_c        extern "C" keeps the bare name ...
 * EXPECT: absent Pl_Skill_ck_c__FUs     ... and emits no mangled twin
 * EXPECT: contains f__3BoxFi            a member function has no extern "C" spelling: its name is a class-qualified mangling
 */
typedef unsigned short u16;

extern "C" void sink(u16 x);

void Pl_Skill_ck(u16 x)
{
    sink(x);
}

extern "C" void Pl_Skill_ck_c(u16 x)
{
    sink(x);
}

struct Box {
    int v;
    void f(int a);
};

void Box::f(int a)
{
    v = a;
}
