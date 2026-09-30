/* Demo for idea 68: A derived class's VTABLE is emitted where its KEY FUNCTION is defined
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: size __vt__9DtorFirst 0x14     first declared virtual (the dtor) has a body here: the table is emitted
 * EXPECT: size __vt__8KeyFirst 0         first declared virtual (`kmove`) has no body here: no table, only the store
 * EXPECT: reloc __vt__8KeyFirst          the constructor still stores the (undefined) table's address
 */
typedef unsigned int u32;

class Base {
public:
    virtual ~Base();
    virtual void move();
    u32 x;
    Base();
};

class KeyFirst : public Base {
public:
    virtual void kmove();               /* key function first, body lives in another unit */
    KeyFirst();
    virtual ~KeyFirst();
};

class DtorFirst : public Base {
public:
    virtual ~DtorFirst();               /* dtor first, defined below */
    virtual void dmove();
    DtorFirst();
};

Base::Base() { x = 0; }
KeyFirst::KeyFirst() { x = 1; }
DtorFirst::DtorFirst() { x = 2; }
Base::~Base() {}
KeyFirst::~KeyFirst() {}
DtorFirst::~DtorFirst() {}
void Base::move() {}
void DtorFirst::dmove() {}
