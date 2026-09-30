/* Demo for idea 34: A switch tail's constant returns are if-converted, so write the arms negated
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count cntlzw 1 in tail_default_return   default `return 0` folds the case test into a branchless boolean
 * EXPECT: count li 1 in tail_default_return       ... and leaves ONE constant block
 * EXPECT: size tail_default_return 0x2c
 * EXPECT: count cntlzw 0 in tail_default_break    default `break` keeps the explicit `li 1` / `li 0` blocks
 * EXPECT: count li 2 in tail_default_break
 * EXPECT: size tail_default_break 0x30
 * EXPECT: size arms_negated 0x2c                  negated and plain arms compile identically in isolation
 * EXPECT: size arms_plain 0x2c
 */
extern "C" {

int tail_default_return(int kind, int allowed)
{
    switch (kind) {
    case 0: case 2: case 4:
        if (!allowed) return 1;
        break;
    default:
        return 0;
    }
    return 0;
}

int tail_default_break(int kind, int allowed)
{
    switch (kind) {
    case 0: case 2: case 4:
        if (!allowed) return 1;
        break;
    default:
        break;
    }
    return 0;
}

int arms_negated(int kind, int allowed)
{
    switch (kind) {
    case 0: case 4: case 2:
        if (!allowed) return 1;
        break;
    default:
        return 1;
    }
    return 0;
}

int arms_plain(int kind, int allowed)
{
    switch (kind) {
    case 0: case 4: case 2:
        if (allowed) return 0;
        break;
    default:
        return 1;
    }
    return 1;
}

}
