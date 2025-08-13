
cbuffer UBO : register(b0, space1)
{
    float4x4 mvp;
};

struct Input
{
    float3 Position : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
};

struct Output
{
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float4 Position : SV_Position;
};

Output main(Input input)
{
    Output output;
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;

    // float4 modelMultiplied = mul(model, float4(input.Position, 1.0f));
    // float4 viewMultiplied = mul(view, modelMultiplied);
    // float4 projectionMultiplied = mul(projection, viewMultiplied);

    // output.Position = projectionMultiplied;

    // multiply by the MVP matrix
    output.Position = mul(mvp, float4(input.Position, 1.0f));

    return output;
}
