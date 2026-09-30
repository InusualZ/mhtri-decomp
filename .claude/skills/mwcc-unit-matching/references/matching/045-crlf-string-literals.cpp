/* Demo for idea 45: A hand-written string literal's `\n` becomes CRLF on this host
 * FLAGS: -O4,p -inline auto -str reuse
 * MWCC: Wii/1.3
 * EXPECT: bytes .sdata 610a620a00     "a\nb\n" is emitted with LF bytes when the source file is LF
 * EXPECT: nobytes .sdata 0d0a
 */
extern "C" {

const char *newline_string(void)
{
    return "a\nb\n";
}

}
