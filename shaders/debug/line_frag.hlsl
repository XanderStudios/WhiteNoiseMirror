//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-31 16:45:14
//

struct FragmentIn
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

float4 Main(FragmentIn input) : SV_TARGET
{
    return float4(input.col, 1.0);
}
