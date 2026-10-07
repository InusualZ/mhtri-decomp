/* Demo for idea 112: A table's source position fixes its .data order against the switch tables of the functions that use it
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: order .data tbl_a1 < tbl_a2       shape A: initialised tables come out in source order
 * EXPECT: order .data tbl_a2 < @20           ... and a table defined BEFORE its user precedes that function's switch table (@20)
 * EXPECT: order .data @39 < tbl_b1           shape B: the tables defined AFTER the user follow its switch table (@39)
 * EXPECT: reloc ...data.0                    shape C: three tables defined before their user are reached through one section-anchor base
 * EXPECT: reloc tbl_d3                       shape D: declared above and defined after, each table is loaded by its own name
 * EXPECT: reloc tbl_b2                       (shape B loads by name too)
 */
extern "C" void sink(int v);

struct Rec {
    int a;
    int b;
};

/* Shape A: two tables defined before the function that uses them. */
Rec tbl_a1[2] = {{1, 2}, {3, 4}};
Rec tbl_a2[2] = {{5, 6}, {7, 8}};

extern "C" void user_a(int n)
{
    switch (n) {
    case 0: sink(10); break;
    case 1: sink(21); break;
    case 2: sink(32); break;
    case 3: sink(43); break;
    case 4: sink(54); break;
    case 5: sink(65); break;
    case 6: sink(76); break;
    case 7: sink(87); break;
    }
    sink(tbl_a1[n & 1].a);
    sink(tbl_a2[n & 1].a);
}

/* Shape B: the same tables declared above the function and defined after it. */
extern Rec tbl_b1[2];
extern Rec tbl_b2[2];

extern "C" void user_b(int n)
{
    switch (n) {
    case 0: sink(10); break;
    case 1: sink(21); break;
    case 2: sink(32); break;
    case 3: sink(43); break;
    case 4: sink(54); break;
    case 5: sink(65); break;
    case 6: sink(76); break;
    case 7: sink(87); break;
    }
    sink(tbl_b1[n & 1].a);
    sink(tbl_b2[n & 1].a);
}

Rec tbl_b1[2] = {{1, 2}, {3, 4}};
Rec tbl_b2[2] = {{5, 6}, {7, 8}};

/* Shape C: three tables defined before a function that reads them one after the other (no switch). */
Rec tbl_c1[2] = {{1, 2}, {3, 4}};
Rec tbl_c2[2] = {{5, 6}, {7, 8}};
Rec tbl_c3[2] = {{9, 6}, {7, 8}};

extern "C" void user_c(int n)
{
    sink(tbl_c1[n & 1].a);
    sink(tbl_c2[n & 1].a);
    sink(tbl_c3[n & 1].a);
}

/* Shape D: the same function with the tables declared above it and defined after it. */
extern Rec tbl_d1[2];
extern Rec tbl_d2[2];
extern Rec tbl_d3[2];

extern "C" void user_d(int n)
{
    sink(tbl_d1[n & 1].a);
    sink(tbl_d2[n & 1].a);
    sink(tbl_d3[n & 1].a);
}

Rec tbl_d1[2] = {{1, 2}, {3, 4}};
Rec tbl_d2[2] = {{5, 6}, {7, 8}};
Rec tbl_d3[2] = {{9, 6}, {7, 8}};
