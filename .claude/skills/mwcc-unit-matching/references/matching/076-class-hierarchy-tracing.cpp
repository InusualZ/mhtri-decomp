/* Demo for idea 76: the constructor's vtable store and the indirect-call slot offsets are what a class hierarchy leaves in the binary
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: reloc __vt__6Holder           the constructor stores the table's address: `__vt__` names a real class
 * EXPECT: size __vt__6Holder 20         2-word header + 3 slots (dtor at +0x08, then release, step)
 * EXPECT: insn lwz r12,16(r12) in drive   `step` is slot +0x10: the offset an indirect call site shows
 * EXPECT: seq lwz lwz mtctr bctr in drive   lwz r12,0(obj); lwz r12,NN(r12); mtctr; bctr (bctrl when not a tail call)
 * EXPECT: absent __vt__5Plain            a class with no virtual member has no table at all
 */
struct Holder {
    virtual ~Holder();
    virtual void release();
    virtual void step();
    int a;
    Holder() : a(0) {}
};

struct Plain {
    int a;
    Plain() : a(0) {}
};

Holder::~Holder() {}
void Holder::release() {}
void Holder::step() {}

void drive(Holder *h)
{
    h->step();
}

Plain plain_instance;
Holder holder_instance;
