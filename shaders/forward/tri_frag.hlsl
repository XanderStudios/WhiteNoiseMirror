//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 17:19:53
//

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

struct Lights
{
    float3 Position;
    float Pad;

    float3 Direction;
    float Cutoff;

    float OuterCutoff;
    bool Lit;
    float2 Pad1;
};

ConstantBuffer<PushConstants> Camera : register(b0);
ConstantBuffer<Lights> Flashlight : register(b2);
Texture2D Albedo : register(t3);
SamplerState Sampler : register(s4);

float4 Main(VertexOut vert) : SV_TARGET
{
    float2 uv = float2(vert.uv.x, 1.0 - vert.uv.y);
    
    float4 albedo = Albedo.Sample(Sampler, uv);
    albedo.xyz = pow(albedo.xyz, 2.2);
    if (albedo.a < 0.1)
        discard;

    /// Apply lighting
    float3 result = 0.0;
    if (Flashlight.Lit) {
        float3 light_dir = normalize(Flashlight.Position - vert.frag_pos);
        float theta = dot(light_dir, normalize(-Flashlight.Direction));
        if (theta > Flashlight.Cutoff) {
            // ambient
            float3 ambient = 0.01 * albedo.xyz;

            // diffuse
            float3 norm = vert.normals;
            float diff = max(dot(norm, light_dir), 0.2);
            float3 diffuse = diff * albedo.xyz;

            // specular
            float3 view_dir = normalize(Camera.CameraPosition.xyz - vert.frag_pos);
            float3 reflect_dir = reflect(-light_dir, norm);
            float spec = max(dot(view_dir, reflect_dir), 0.2);
            float3 specular = spec * albedo.xyz;

            float theta = dot(light_dir, normalize(-Flashlight.Direction));
            float epsilon = Flashlight.Cutoff - Flashlight.OuterCutoff;
            float intensity = clamp((theta - Flashlight.OuterCutoff) / epsilon, 0.0, 1.0);

            diffuse *= intensity;
            specular *= intensity;

            result = (ambient + diffuse + specular) * 4.0;
        } else {
            result = albedo.xyz * 0.01;
        }
    } else {
        result = albedo.xyz;
    }

    return float4(result, 1.0);
}
 