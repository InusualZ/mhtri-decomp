/* Demo for idea 155: A member cleanup record retail lacks: define the callee before its caller (MWCC's nothrow knowledge)
 * FLAGS: -O3 -inline noauto -Cpp_exceptions on -lang=c++
 * MWCC: Wii/1.3
 * EXPECT: section extab 72    Known 8 + Late 28 + Msl 8 + Plain 28: only the late callee and the plain memset record the member
 * EXPECT: bytes extab 8780001f    the member-cleanup action (destroy `this`+4) the two may-throw calls register
 */

struct Member {
    Member();
    ~Member();
    int word;
};

/* `reset` is compiled before the constructor that calls it: MWCC knows it cannot throw. */
struct Known {
    Known();
    void reset();
    int state;
    Member member;
};

/* `reset` is defined after the constructor: the call may throw, so `member` gets a cleanup record. */
struct Late {
    Late();
    void reset();
    int state;
    Member member;
};

struct Msl {
    Msl();
    int state;
    Member member;
};

struct Plain {
    Plain();
    int state;
    Member member;
};

extern "C" void* memset(void* dst, int val, unsigned long size) __attribute__((nothrow));
extern "C" void* plain_memset(void* dst, int val, unsigned long size);

#pragma dont_inline on

inline void Known::reset()
{
    state = 0;
}

Known::Known()
{
    reset();
}

Late::Late()
{
    reset();
}

inline void Late::reset()
{
    state = 0;
}

/* MSL declares its C functions `nothrow` (`_MSL_CANT_THROW`): no cleanup around the call. */
Msl::Msl()
{
    memset(&state, 0, 4);
}

Plain::Plain()
{
    plain_memset(&state, 0, 4);
}

#pragma dont_inline off
