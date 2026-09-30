/* Demo for idea 37: A switch's `default` arm goes first in the source
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: size default_first 0x48   default written first: 4 bytes smaller
 * EXPECT: size default_last 0x4c    default written last
 */
extern "C" {

int helper1(int x);
int helper2(int x);

int default_last(int kind, int x)
{
    switch (kind) {
    case 1: return helper1(x);
    case 2: return helper1(x + 1);
    case 3: return helper1(x + 2);
    case 4: return helper1(x + 3);
    default: return helper2(x);
    }
}

int default_first(int kind, int x)
{
    switch (kind) {
    default: return helper2(x);
    case 1: return helper1(x);
    case 2: return helper1(x + 1);
    case 3: return helper1(x + 2);
    case 4: return helper1(x + 3);
    }
}

}
