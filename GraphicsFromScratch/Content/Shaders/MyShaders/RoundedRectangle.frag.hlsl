cbuffer UBO : register(b0, space3)
{
    float radius;
};

struct Input
{
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float4 Position : TEXCOORD2;
};

float4 main(Input input) : SV_Target0
{
    // float2 rectangleCenter = float2(0.5, 0.5);
    float2 centerPosition = float2(0.5, 0.5);
    float2 size = float2(1, 1);

    // example:
    // https://www.shadertoy.com/view/WtdSDs 
    float dist = length(max(abs(input.TexCoord - size / 2.0) - size / 2.0 + radius, 0.0)) - radius;
    float alpha = 1.0 - smoothstep(0.0, 0.003, dist);

    return float4(input.Color.rgb, alpha);
}
