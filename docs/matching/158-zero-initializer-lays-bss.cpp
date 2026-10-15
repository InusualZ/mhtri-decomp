/* Demo for idea 130: A zero initializer lays bss out in definition order instead of first-use order
 * FLAGS: -O4,p -inline auto -lang c
 * MWCC: Wii/1.3
 * EXPECT: order .bss gameName < checkPlain      uninitialised (a C tentative definition): first use decides
 * EXPECT: order .bss checkPlain < hostName      ... so the probe state lands between the two names
 * EXPECT: order .bss gameNameZero < hostNameZero   zero-initialised: definition order decides
 * EXPECT: order .bss hostNameZero < checkZero      ... the host name stays ahead of the probe state
 */
typedef unsigned char u8;
typedef struct Rec { int a; int b[0x19]; } Rec;

void fill(u8* a, Rec* b);
void copy(u8* a, const u8* b);

u8 gameName[0x40];
u8 hostName[0x40];
Rec checkPlain;

u8 gameNameZero[0x40] = {0};
u8 hostNameZero[0x40] = {0};
Rec checkZero;

void touchPlain(const u8* name)
{
    copy(gameName, name);
    checkPlain.a = -1;
    fill(0, 0);
    if (hostName[0] == 0) {
        fill(gameName, &checkPlain);
    }
}

void touchZero(const u8* name)
{
    copy(gameNameZero, name);
    checkZero.a = -1;
    fill(0, 0);
    if (hostNameZero[0] == 0) {
        fill(gameNameZero, &checkZero);
    }
}
