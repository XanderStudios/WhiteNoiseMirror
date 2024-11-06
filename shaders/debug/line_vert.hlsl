//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-31 16:37:45
//

struct VertexInput
{
    float3 pos : POSITION;
    float3 col : COLOR;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

struct Camera
{
    column_major float4x4 View;
    column_major float4x4 Projection;
};

ConstantBuffer<Camera> Matrices : register(b0);

VertexOutput Main(VertexInput input)
{
    VertexOutput vert = (VertexOutput)0;

    vert.pos = float4(input.pos, 1.0);
    vert.pos = mul(Matrices.View, vert.pos);
    vert.pos = mul(Matrices.Projection, vert.pos);
    vert.col = input.col;

    return vert;
}
