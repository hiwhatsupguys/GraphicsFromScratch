// cool website for alignment: https://maraneshi.github.io/HLSL-ConstantBufferLayoutVisualizer/
cbuffer Global : register(b0, space3) 
{
    // gpu constant buffer alignment rules: each row is 16 bytes
    // note: a float is 4 bytes, and a float3 is 12 bytes
    float3 lightPosition;
    float3 lightColor;
    float lightIntensity;
    float3 viewPosition;
    float3 ambientLightColor;

};

// breaks everything for some reason?
cbuffer Local : register(b1, space3) 
{
    float3 materialSpecularColor;
    float materialShininess;
}; 

Texture2D<float4> DiffuseMap : register(t0, space2); // previously was "Texture"
SamplerState Sampler : register(s0, space2);

struct Input
{
    float4 ClipPosition : SV_Position;
    float3 Position : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
    float3 Normal : TEXCOORD3; // surface normal, may not be normalized

};
float3 blinnPhongBRDF(float3 dirToLight, float3 dirToView, float3 surfaceNormal, float3 materialDiffuseReflection)
{


    float3 halfWayDir = normalize(dirToLight + dirToView);

    float specularDot = max(0.0, dot(halfWayDir, surfaceNormal)); // Angle between dirToLight and surfaceNormal. Don't need to divide by the lengths since the dir vectors are normal vectors

    float specularFactor = pow(specularDot, materialShininess);

    float3 specularReflection = materialSpecularColor * specularFactor;

    return materialDiffuseReflection + specularReflection;
    
}

// float4 main(Input Input) : SV_Target0 {
// 	float3 vecToLight = lightPosition - Input.Position;
// 	float distToLight = length(vecToLight);
// 	float3 dirToLight = vecToLight / distToLight;
// 
// 	float3 dirToView = normalize(viewPosition - Input.Position);
// 
// 	float3 surfaceNormal = normalize(Input.Normal);
// 
// 	float3 materialDiffuseReflection = DiffuseMap.Sample(Sampler, Input.TexCoord).rgb;
// 
// 	float3 ambientIrradiance = ambientLightColor;
// 
// 	float3 reflectedRadiance = ambientIrradiance * materialDiffuseReflection;
// 
// 	float incidenceAngleFactor = dot(dirToLight, surfaceNormal); // 1 direct incidence, 0 - no incidence, -1 incidence from the other side
// 	if (incidenceAngleFactor > 0) {
// 		float attenuationFactor = 1 / (distToLight * distToLight); // TODO: add more control variables
// 		float3 incomingRadiance = lightColor * lightIntensity;
// 		float3 irradiance = incomingRadiance * incidenceAngleFactor * attenuationFactor;
// 		float3 brdf = blinnPhongBRDF(dirToLight, dirToView, surfaceNormal, materialDiffuseReflection);
// 		reflectedRadiance += irradiance * brdf;
// 	}
// 
// 	float3 emittedRadiance = float3(0, 0, 0);
// 
// 	float3 outRadiance = emittedRadiance + reflectedRadiance;
// 
// 	return float4(outRadiance, 1);
// }


float4 main(Input input) : SV_Target0
{

    // fragment: tiny surface that radiates light

    float3 vecToLight = lightPosition - input.Position;
    float distToLight = length(vecToLight);
    float3 vecToLightNormalized = vecToLight / distToLight;

    float3 vecToViewNormalized = normalize(viewPosition - input.Position);

    float3 surfaceNormal = normalize(input.Normal);

    float3 materialDiffuseReflection = DiffuseMap.Sample(Sampler, input.TexCoord).rgb; // reflects "x" of red, "y" of green, and "z" of blue

    float3 ambientIrradiance = ambientLightColor;

    float incidenceAngleFactor = dot(vecToLightNormalized, surfaceNormal);

    float3 reflectedRadiance = ambientIrradiance * materialDiffuseReflection;

    if (incidenceAngleFactor > 0.0)
    { 
        float attenuationFactor = 1.0 / (distToLight * distToLight); // TODO: add more control variables
        float3 incomingRadiance = lightColor * lightIntensity;
        float3 irradiance = incomingRadiance * incidenceAngleFactor * attenuationFactor;  // radiance received by the surface (how much light is received by the fragment/pixel)
        // value of 1: no change in radiance, all just being directed towards the eye
        float3 brdf = blinnPhongBRDF(vecToLightNormalized, vecToViewNormalized, surfaceNormal, materialDiffuseReflection); // bidirectional reflection distribution function: calculates how much radiance to reflect depending on the material
        reflectedRadiance += irradiance * brdf;
    }

    float3 emittedRadiance = float3(0.0, 0.0, 0.0);

    float3 outRadiance = emittedRadiance + reflectedRadiance;

    return float4(outRadiance, 1.0);

    // float4 color = DiffuseMap.Sample(Sampler, input.TexCoord);
    // return color;
}
