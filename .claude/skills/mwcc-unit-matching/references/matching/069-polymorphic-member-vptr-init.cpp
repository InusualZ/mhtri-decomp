/* Demo for idea 69: A POLYMORPHIC MEMBER CLASS makes MWCC initialise the vptr of every array element
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: reloc __construct_array               the class-member array is built by the runtime helper ...
 * EXPECT: reloc __ct__4RecCFv                   ... and the record gets a synthesised ctor/dtor
 * EXPECT: count bl 0 in __ct__7HolderSFv         the struct-view version is a plain store: no helper, no element ctor
 */
typedef unsigned int u32;

class Small {
public:
    virtual ~Small();
    u32 x;
    Small();
};

struct SmallView {
    void **vt;
    u32 x;
};

struct RecC { Small obj; u32 state; };          /* polymorphic member */
struct RecS { SmallView obj; u32 state; };      /* the table only viewed */

struct HolderC {
    RecC recs[4];
    u32 n;
    HolderC();
};

struct HolderS {
    RecS recs[4];
    u32 n;
    HolderS();
};

HolderC::HolderC() { n = 0; }
HolderS::HolderS() { n = 0; }
