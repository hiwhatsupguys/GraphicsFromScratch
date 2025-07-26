
struct Input
{
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float4 Position : TEXCOORD2;
};

float4 main(Input input) : SV_Target0
{
    const float2 circleCenter = float2(0.5, 0.5);
    const float radius = 0.5;
    float fade = 0.005;

    // percent of radius
    float thickness = 0.2;
    thickness *= radius;

    float dist = distance(input.TexCoord, circleCenter);
    float distFromEdge = radius - dist;


    // circle code: https://www.youtube.com/watch?v=xf7Y988cPRk 
    // this should probably be the other way around semantically
    // 0 if negative, 1 otherwise
    float blend = smoothstep(0.0f, fade, distFromEdge);
    // other side blend
    // blend *= 1.0 - smoothstep(thickness - fade, thickness, distFromEdge);

    // float4 outputColor = float4(input.Color.r, input.Color.g, input.Color.b, blend);
    float4 outputColor = float4(input.Color.rgb, blend);

    return outputColor;
}
