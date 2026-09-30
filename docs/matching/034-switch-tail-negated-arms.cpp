/* Demo for idea 34: A switch tail's constant returns - `default: return K` against `default: break`
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: size fold_default_ret0 0x2c   arms reduce to a boolean: `default: return 0` merges with the tail and the case test folds
 * EXPECT: count cntlzw 1 in fold_default_ret0
 * EXPECT: size fold_default_break 0x30  `default: break` keeps explicit li 1 / li 0 blocks
 * EXPECT: count cntlzw 0 in fold_default_break
 * EXPECT: size call_default_ret0 0x6c   arms that call: `default: return 0` is a return-0 block of its own (8 B more)
 * EXPECT: size call_default_break 0x64  `default: break` shares the one tail
 * EXPECT: size arms_negated 0x2c       the same function with the tested condition negated: identical size
 * EXPECT: size arms_plain 0x2c
 */
extern "C" {

int chk(int);

int fold_default_ret0(int id, int allowed)
{
    switch (id) {
    case 0: case 4: case 2:
        if (!allowed) return 1;
        break;
    default:
        return 0;
    }
    return 0;
}

int fold_default_break(int id, int allowed)
{
    switch (id) {
    case 0: case 4: case 2:
        if (!allowed) return 1;
        break;
    default:
        break;
    }
    return 0;
}

int call_default_ret0(int id, int x)
{
    switch (id) {
    case 0: case 4:
        if (chk(x)) return 1;
        break;
    case 2:
        if (x > 3) return 1;
        break;
    default:
        return 0;
    }
    return 0;
}

int call_default_break(int id, int x)
{
    switch (id) {
    case 0: case 4:
        if (chk(x)) return 1;
        break;
    case 2:
        if (x > 3) return 1;
        break;
    default:
        break;
    }
    return 0;
}

int arms_negated(int id, int allowed)
{
    switch (id) {
    case 0: case 4: case 2:
        if (allowed) break;
        return 1;
    default:
        return 1;
    }
    return 0;
}

int arms_plain(int id, int allowed)
{
    switch (id) {
    case 0: case 4: case 2:
        if (!allowed) return 1;
        break;
    default:
        return 1;
    }
    return 0;
}

}
