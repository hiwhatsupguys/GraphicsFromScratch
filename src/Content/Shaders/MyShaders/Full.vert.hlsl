cbuffer UBO : register(b0, space1) // TODO: make it so the gpu only needs one vp matrix (need separate global and local UBOs)
{
    float4x4 vp;
    float4x4 m;
};

struct Input
{
    float3 Position : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
    float3 Normal : TEXCOORD3;
};

struct Output
{
    float4 ClipPosition : SV_Position; // clip space position, not world position
    float3 Position : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
    float3 Normal : TEXCOORD3; // surface normal, may not be normalized
};

Output main(Input input)
{

    float4 worldPosition = mul(m, float4(input.Position, 1.0f));
    float4 clipPosition = mul(vp, float4(worldPosition));

    Output output;

    output.ClipPosition = clipPosition;
    output.Position = clipPosition.xyz;
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    output.Normal = normalize(mul(m, float4(input.Normal, 0)).xyz); // TODO: use normal matrix to support non-uniform scale

    return output;
}
