/* Demo for idea 62: A `new` expression is not `operator new` plus a constructor call
 * FLAGS: -O4,p -inline auto -Cpp_exceptions off
 * MWCC: Wii/1.3
 * EXPECT: insn mr r31,r3 in expr_dead      the new expression keeps its pointer alive across the ctor (unwind path)
 * EXPECT: count mr 0 in manual_dead        the hand-written form lets it die in r3
 * EXPECT: reloc __ct__3MedFv
 * EXPECT: reloc Med_init__FP3Med
 * EXPECT: section extab 0x20               the pragma turns the unwind tables on in a lib built -Cpp_exceptions off
 */
#pragma exceptions on
typedef unsigned long size_t;
void *operator new(size_t);

struct Med {
    int a;
    int b;
    Med();
};
void Med_init(Med *);
Med *g;

void expr_dead()
{
    g = 0;
    new Med();
}

void manual_dead()
{
    Med *p = (Med *)operator new(sizeof(Med));
    g = 0;
    if (p != 0) {
        Med_init(p);
    }
}
