/* Demo for idea 60: The declaration set is part of the codegen
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: contains @10     the pool entry for 3.5f is named @10 with THREE unused prototypes above it ...
 * EXPECT: absent @7        ... and @7 when only `sink` is declared (delete x1..x3 and re-run --dump): declarations are numbered
 */
extern "C" void x1(float v);
extern "C" void x2(float v);
extern "C" void x3(float v);
extern "C" void sink(float v);

extern "C" void scale_a(float x)
{
    sink(x * 3.5f);
}
