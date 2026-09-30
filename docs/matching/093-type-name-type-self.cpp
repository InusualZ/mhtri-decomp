/* Demo for idea 93: A Type_name(Type* self) free function is a member: define Type::name and rename the map row to the mangling
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: contains Tcp_send       the free form emits the plain C name
 * EXPECT: contains send__3TcpFi   the member emits the mangled name (class length 3, one int)
 * EXPECT: absent send__Fi         a free function named `send` would be mangled without the class
 * EXPECT: size Tcp_send 12        lwz + add + blr
 * EXPECT: size send__3TcpFi 12    the same three instructions: `this` arrives in r3 like `self`
 * EXPECT: count lwz 1 in Tcp_send
 * EXPECT: count lwz 1 in send__3TcpFi
 * NOTE: written without a compiler in reach (no build/ tree in the authoring worktree); the sizes and the
 *   mangling are the expected values - `ideas.py demo-check 93` is the check, and a mismatch means the demo
 *   records what the compiler did and the idea text gets a "Demonstration" note.
 */

struct Tcp {
    int a;
    int send(int n);
};

/* The C way: a free function whose first parameter is the object. */
extern "C" int Tcp_send(Tcp* self, int n)
{
    return self->a + n;
}

/* The member: the same body, `this` in r3. */
int Tcp::send(int n)
{
    return a + n;
}
