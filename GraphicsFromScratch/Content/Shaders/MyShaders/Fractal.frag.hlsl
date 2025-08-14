cbuffer UBO : register(b0, space3)
{
    float2 resolution;
    float2 constant;
    float yScale;
    uint maxIterations;
};

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

// z: (real, imag)
float2 complexSquare(float2 z)
{
    float real = z.x * z.x - z.y * z.y;
    float imag = 2 * z.x * z.y;

    return float2(real, imag);
}

float2 computeNext(float2 current, float2 constant)
{
    return complexSquare(current) + constant;
}

// returns final zn's x, y, and the number of iterations until it escapes R or reaches maxIterations
float3 computeIterations(float2 z0, float2 constant, float escapeRadius, uint maxIterations)
{
    float2 zn = z0;
    uint iteration = 0;

    while (length(zn) < escapeRadius && iteration < maxIterations)
    {
        zn = computeNext(zn, constant);
        iteration++;
    }
    
    return float3(zn.x, zn.y, iteration);
    
}

float computeBrightness(float numIterations, float2 zn)
{
    // apply smoothing (reducing color banding) straight from wikipedia:
    // https://en.wikipedia.org/wiki/Julia_set#Pseudocode_for_multi-Julia_sets

    // if we hit the max, then fill
    // this hits the case where absZ is <=1, meaning log log is undefined
    if (numIterations == maxIterations)
    {
        return maxIterations;
    }

    float absZ = length(zn);

    return max(0, 
    float(numIterations) + 1 - log(log(absZ)) / log(2)
    );

}

Output main(Input input)
{
    Output result;

    // scale to -1, 1 on x and y
    float2 centeredUV = 2 * input.TexCoord - float2(1, 1);
    centeredUV *= float2(resolution.x / resolution.y, 1); // maintain aspect ratio
    // scale y from -scale to scale and x along with it
    centeredUV *= yScale;

    float escapeRadius = 2;

    float3 computeResult = computeIterations(centeredUV, constant, escapeRadius, maxIterations);
    float numIterations = computeResult.z;
    float2 zn = computeResult.xy;

    // float brightness = max(0, float(numIterations));
    float brightness = computeBrightness(numIterations, zn);

    // need to divide by a big number since we could be getting a number near or at maxIterations
    brightness /= 100;
    
    result.Color = float4(brightness, 0, 0, 1);
    return result;
}
