/* Demo for idea 53: A sparse switch's jump table is readable once its .data range is claimed
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: contains mtctr          a dense switch dispatches through a table: mtctr / bctr
 * EXPECT: contains bctr
 * EXPECT: section .data 32        the table is DATA the compiler emitted for THIS translation unit: 8 slots x 4 B, in .data
 * EXPECT: reloc @16               the code reaches the table through an anonymous @N symbol (ha/lo pair): the reloc that pairs with retail's own
 */
extern "C" void sink(int v);

extern "C" void dispatch(int n)
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
}
