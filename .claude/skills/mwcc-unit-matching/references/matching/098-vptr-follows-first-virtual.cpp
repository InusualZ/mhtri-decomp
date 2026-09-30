/* Demo for idea 98: A class's vtable pointer lands where its first virtual is declared
 * FLAGS: -O3 -inline noauto
 * MWCC: Wii/1.3
 * EXPECT: insn stw r0,0(r3) in set_late     fields declared before the virtual: the field is at +0, the pointer follows them
 * EXPECT: insn stw r0,4(r3) in set_early    the virtual declared first: the vtable pointer owns +0, the field moves to +4
 */
#pragma peephole off

class Late {
public:
    int field;
    virtual ~Late();
};

class Early {
public:
    virtual ~Early();
    int field;
};

extern "C" void set_late(Late* p)
{
    p->field = 0;
}

extern "C" void set_early(Early* p)
{
    p->field = 0;
}
