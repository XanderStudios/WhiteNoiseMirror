//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-11-02 17:35:28
//

float3 ACESFilm(float3 X)
{
    float A = 2.51f;
    float B = 0.03f;
    float C = 2.43f;
    float D = 0.59f;
    float E = 0.14f;
    return saturate((X * (A * X + B)) / (X * (C * X + D) + E));
}

Texture2D HDRTexture : register(t0);
RWTexture2D<float4> LDRTexture : register(u1);

[numthreads(8, 8, 1)]
void Main(uint3 ThreadID : SV_DispatchThreadID)
{
    uint Width, Height;
    HDRTexture.GetDimensions(Width, Height);

    if (ThreadID.x <= Width && ThreadID.y <= Height) {
        float4 HDRColor = HDRTexture[ThreadID.xy];
        float4 final = float4(ACESFilm(HDRColor.xyz), HDRColor.a);
        final.rgb = pow(final.rgb, 1.0 / 2.2);

        LDRTexture[ThreadID.xy] = final;
    }
}
