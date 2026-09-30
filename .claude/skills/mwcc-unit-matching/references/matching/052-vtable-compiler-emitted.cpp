/* Demo for idea 52: A vtable we own must be compiler-emitted
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: contains __vt__6Widget          a class with virtual methods and a constructor: MWCC emits the table itself
 * EXPECT: reloc __vt__6Widget             the constructor stores its address (the vptr store is the discriminator)
 * EXPECT: size __vt__6Widget 16           two slots plus the 8-byte header
 * EXPECT: section .data 16                the struct-of-function-pointers view emits NO table of its own: .data is Widget's alone
 * EXPECT: reloc lbl_805FA908              the view reaches the retail table through its map symbol (an extern)
 */
extern "C" {
extern void *const lbl_805FA908;
}

struct Widget {
    int state;
    Widget();
    virtual void update(int n);
    virtual void draw(int n);
};

Widget::Widget()
{
    state = 0;
}

void Widget::update(int n)
{
    state = n;
}

void Widget::draw(int n)
{
    state += n;
}

struct SlotTable {
    void *rtti;
    void *pad;
    void (*update)(void *self, int n);
};

struct ForeignView {
    const SlotTable *vtable;
};

extern "C" void call_foreign_slot(ForeignView *obj, int n)
{
    obj->vtable = (const SlotTable *)&lbl_805FA908;
    obj->vtable->update(obj, n);
}
