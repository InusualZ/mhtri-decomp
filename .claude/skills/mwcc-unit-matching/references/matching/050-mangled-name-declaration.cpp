/* Demo for idea 50: An already-mangled map name must not be declared as a C++ identifier
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: reloc Panic__Q24nw4r2dbFPCciPCce__FPCciPCce   the mangled spelling written as an identifier is mangled AGAIN
 * EXPECT: contains via_identifier                        both callers compile; only the LINK would fail
 * EXPECT: contains via_namespace
 */
extern void Panic__Q24nw4r2dbFPCciPCce(const char *file, int line, const char *fmt, ...);

namespace nw4r {
namespace db {
void Panic(const char *file, int line, const char *fmt, ...);
}
}

extern "C" void via_identifier(void)
{
    Panic__Q24nw4r2dbFPCciPCce("a.cpp", 1, "x");
}

extern "C" void via_namespace(void)
{
    nw4r::db::Panic("a.cpp", 1, "x");
}
