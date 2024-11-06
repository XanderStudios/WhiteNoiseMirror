//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 17:18:17
//

struct VertexIn
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normals : NORMALS;
    int4 bone_ids : BONES;
    float4 weights: WEIGHTS;
};

struct VertexOut
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normals : NORMAL;
    float3 frag_pos : POSITION;
};

struct PushConstants
{
    column_major float4x4 View;
    column_major float4x4 Projection;
    float4 CameraPosition;
};

struct ModelData
{
    column_major float4x4 Transform;
};

ConstantBuffer<PushConstants> Camera : register(b0);
ConstantBuffer<ModelData> Model : register(b1);

VertexOut Main(VertexIn input)
{
    VertexOut vert = (VertexOut)0;
    
    vert.pos = float4(input.pos, 1.0);
    vert.pos = mul(Model.Transform, vert.pos);
    vert.pos = mul(Camera.View, vert.pos);
    vert.pos = mul(Camera.Projection, vert.pos);
    
    vert.uv = input.uv;
    
    vert.normals = normalize(input.normals);
    
    vert.frag_pos = mul(Model.Transform, float4(input.pos, 1.0)).xyz;
    
    return vert;
}
