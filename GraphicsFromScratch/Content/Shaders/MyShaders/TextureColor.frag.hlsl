Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

cbuffer UBO : register(b0, space3)
{
    float time : packoffset(c0);
};

struct Input
{
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
};

float4 main(Input input) : SV_Target0
{
    float t = time * 2 * 3.141592;
    float r = 0.5 + 0.5 * sin(t);
    float g = 0.5 + 0.5 * sin(t + 3.141592 / 2);
    float b = 0.5 + 0.5 * sin(t + 3.141592);

    float4 color = float4(r, g, b, input.Color.a);
    return color * Texture.Sample(Sampler, input.TexCoord);
}
