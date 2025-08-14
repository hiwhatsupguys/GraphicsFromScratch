// cbuffer UBO : register(b0, space3)
// {
    // float NearPlane;
    // float FarPlane;
// };

struct Input
{
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float4 Position : SV_Position;
};

struct Output
{
    float4 Color : SV_Target0;
};

Output main(Input input)
{
    Output result;

    // scale to -1, 1 on x and y
    float2 cartesianUV = 2 * input.TexCoord - float2(1, 1);
    
    result.Color = float4(max(0, 1 - length(cartesianUV)), 0, 0, 1);
    return result;
}
