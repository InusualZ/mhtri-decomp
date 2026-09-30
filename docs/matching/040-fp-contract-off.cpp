/* Demo for idea 40: `-fp_contract off` keeps a*b + c as two instructions
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: contains fmsubs
 * EXPECT: count fmsubs 1 in scale_contract_on    fused multiply-subtract
 * EXPECT: count fmuls 0 in scale_contract_on
 * EXPECT: count fmsubs 0 in scale_contract_off
 * EXPECT: seq fmuls fsubs in scale_contract_off  retail's unfused pair
 */
extern "C" {

float scale_contract_on(float t)
{
    return 2.0f * t - 1.0f;
}

#pragma fp_contract off
float scale_contract_off(float t)
{
    return 2.0f * t - 1.0f;
}
#pragma fp_contract on

}
