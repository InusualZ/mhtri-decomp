/* Demo for idea 97: A record passed by value to a virtual slot: give the pointer slot an inline by-value overload
 * FLAGS: -O3 -inline noauto
 * MWCC: Wii/1.3
 * EXPECT: count stw 7 in by_value     LR save + the record built (3) + the argument temporary (3), constants again
 * EXPECT: count stw 4 in by_pointer   LR save + the record built once (3)
 */
#pragma peephole off

struct Posted {
    int code;
    int param1;
    int param2;
};

class Dispatch {
public:
    virtual void pad_08();
    virtual void post(Posted* info);
    /* the by-value spelling: the caller's argument copy is what the slot is handed */
    inline void post(Posted info) { post(&info); }
};

extern "C" void by_value(Dispatch* d)
{
    Posted error;

    error.code = 0x80000000;
    error.param1 = 0;
    error.param2 = 0;
    d->post(error);
}

extern "C" void by_pointer(Dispatch* d)
{
    Posted error;

    error.code = 0x80000000;
    error.param1 = 0;
    error.param2 = 0;
    d->post(&error);
}
