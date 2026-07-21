cbuffer Global : register(b0, space1) // TODO: make it so the gpu only needs one vp matrix (need separate global and local UBOs)
{
    float4x4 viewProjectionMat;
};

cbuffer Local : register(b1, space1) // TODO: make it so the gpu only needs one vp matrix (need separate global and local UBOs)
{
    float4x4 modelMat;
    float4x4 normalMat;
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

    float4 worldPosition = mul(modelMat, float4(input.Position, 1.0f));
    float4 clipPosition = mul(viewProjectionMat, float4(worldPosition));

    Output output;

    output.ClipPosition = clipPosition;
    output.Position = clipPosition.xyz;
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    output.Normal = normalize(mul(normalMat, float4(input.Normal, 0)).xyz);

    return output;
}
