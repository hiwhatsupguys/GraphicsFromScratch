// cool website for alignment: https://maraneshi.github.io/HLSL-ConstantBufferLayoutVisualizer/
cbuffer Global : register(b0, space3) 
{
    // gpu constant buffer alignment rules: each row is 16 bytes
    // note: a float is 4 bytes, and a float3 is 12 bytes
    float3 lightPosition;
    float3 lightColor;
    float lightIntensity;

};

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

// cbuffer UBO : register(b0, space3)
// {
// };

struct Input
{
    float4 ClipPosition : SV_Position;
    float3 Position : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
    float3 Normal : TEXCOORD3; // surface normal, may not be normalized

};

float4 main(Input input) : SV_Target0
{
    // float4 color = Texture.Sample(Sampler, input.TexCoord);
    // return color;

    // fragment: tiny surface that radiates light

    float3 vecToLight = lightPosition - input.Position;
    float distToLight = length(vecToLight);
    float3 vecToLightNormalized = vecToLight / distToLight;

    float3 surfaceNormal = normalize(input.Normal);

    float incidenceAngleFactor = dot(vecToLightNormalized, surfaceNormal);

    float3 reflectedRadiance;

    if (incidenceAngleFactor > 0.0)
    { 
        float attenuationFactor = 1.0 / (distToLight * distToLight); // TODO: add more control variables
        float3 incomingRadiance = lightColor * lightIntensity;
        float3 irradiance = incomingRadiance * incidenceAngleFactor * attenuationFactor;  // radiance received by the surface (how much light is received by the fragment/pixel)
        // value of 1: no change in radiance, all just being directed towards the eye
        float3 brdf = 1; // brdf: bidirectional reflection distribution function: calculates how much radiance to reflect depending on the material
        reflectedRadiance = irradiance * brdf; 
    }
    else
    {
        reflectedRadiance = float3(0.0, 0.0, 0.0);
    }

    float3 emittedRadiance = float3(0.0, 0.0, 0.0);

    float3 outRadiance = emittedRadiance + reflectedRadiance;

    return float4(outRadiance, 1.0);
}
