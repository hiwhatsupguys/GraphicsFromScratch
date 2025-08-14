cbuffer UBO : register(b0, space3)
{
    float NearPlane;
    float FarPlane;
};

struct Output
{
    float4 Color : SV_Target0;
};

float LinearizeDepth(float depth, float near, float far)
{
    return 0;
}

Output main(float4 Color : TEXCOORD0, float4 Position : SV_Position)
{
    Output result;
    result.Color = Color;
    return result;
}
