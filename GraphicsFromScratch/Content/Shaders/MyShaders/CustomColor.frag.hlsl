cbuffer UBO : register(b0, space3)
{
    float time;
};

float4 main(float4 Color : TEXCOORD0) : SV_Target0
{
    float speed = 1.0f;
    float pulse = 0.5 + 0.5 * sin(speed * time * (3.141592f));
    return float4(Color.rgb * pulse, Color.a);
    // return Color;
}
