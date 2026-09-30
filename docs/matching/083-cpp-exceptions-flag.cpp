/* Demo for idea 83: -Cpp_exceptions on adds extab records; with a `new` expression it also moves .text
 * FLAGS: -O4,p -inline auto -Cpp_exceptions on
 * MWCC: Wii/1.3
 * EXPECT: section extab 32                      -Cpp_exceptions on: unwind records exist (a lib built `off` emits no extab)
 * EXPECT: insn mr r31,r3 in make_new_expr        a `new` expression keeps the pointer alive across the ctor (unwind path)
 * EXPECT: count mr 0 in make_manual              operator new + null check + ctor call: r3 dies at the ctor, no copy
 * EXPECT: reloc T_ctor
 */
typedef unsigned long size_t;
#define NULL 0

struct T {
    int a;
    T();
};

void *operator new(size_t n);
extern "C" void T_ctor(T *p);

extern "C" void make_new_expr(void)
{
    new T;
}

extern "C" void make_manual(void)
{
    T *p = (T *)operator new(sizeof(T));
    if (p != NULL)
        T_ctor(p);
}

extern "C" int no_cleanup(int x)
{
    return x + 1;
}
