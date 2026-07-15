Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

struct Input
{
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
    
};

float4 main(Input input) : SV_Target0
{
    float4 color = Texture.Sample(Sampler, input.TexCoord);
    
    // apply gamma correction
    color.rgb = pow(color.rgb, 2.2f);

    float4 finalColor = color * input.Color;
    finalColor.rgb = pow(finalColor.rgb, 1.0f / 2.2f);

    return color;
    // return float4(0.0f, 0.0f, 0.0f, 1.0f);
    // return Texture.Sample(Sampler, TexCoord);
}
