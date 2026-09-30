/* Demo for idea 96: An index used before and after a call: name the element pointer so the address survives it
 * FLAGS: -O3 -inline noauto
 * MWCC: Wii/1.3
 * EXPECT: count slwi 1 in f_ptr      the scaled index is computed once and lives across the call
 * EXPECT: count slwi 2 in f_plain    the index is scaled again after the call
 */
#pragma peephole off

struct Table {
    unsigned int pad_00[4];
    unsigned int ids_10[4];
    unsigned int slots_20[4];
    unsigned char count_30;
    unsigned char state_31[4];
};

extern "C" int notify(int id);

extern "C" void f_plain(Table* t, int index)
{
    unsigned char i;

    notify((int)t->ids_10[index]);
    for (i = 0; i < t->count_30; i++) {
        if (index == t->state_31[i]) {
            t->state_31[i] = 3;
            break;
        }
    }
    t->slots_20[index] = 0;
    t->ids_10[index] = 0;
}

extern "C" void f_ptr(Table* t, int index)
{
    unsigned char i;
    unsigned int* id = &t->ids_10[index];

    notify((int)*id);
    for (i = 0; i < t->count_30; i++) {
        if (index == t->state_31[i]) {
            t->state_31[i] = 3;
            break;
        }
    }
    t->slots_20[index] = 0;
    *id = 0;
}
