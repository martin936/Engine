#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(binding = 0) uniform texture2D DOF_Color;
layout(binding = 1) uniform texture2D ZMap;
layout(binding = 2) uniform sampler sampLinear;

layout(location = 0) out vec4 Color;


layout(push_constant) uniform pc0
{
	float	MaxBlurRadiusFar;
	float	FocalLength;
	float	NearPlane;
	float	FarPlane;
};


float ComputeCoC(float depth)
{
    float ZCam = 2.f * (NearPlane * FarPlane) / (FarPlane + NearPlane + (2.f * depth - 1.f) * (FarPlane - NearPlane));

    return MaxBlurRadiusFar * abs(1.f - FocalLength / ZCam);
}


void main()
{
    float depth = texelFetch(ZMap, ivec2(gl_FragCoord.xy), 0).r;

    vec2 size = textureSize(ZMap, 0).xy;

	float CoC = min(10.f, ComputeCoC(depth) * size.y);
    
	vec4 DOFed = textureLod(sampler2D(DOF_Color, sampLinear), gl_FragCoord.xy / size, 0);

    vec4 alpha = textureGather(sampler2D(DOF_Color, sampLinear), gl_FragCoord.xy / size, 3);

    float minDepth = max(alpha.r, max(alpha.g, max(alpha.b, alpha.a)));

    for (uint i = 0; i < 4; i++)
    {
        if (alpha[i] > 0.f)
            minDepth = min(minDepth, alpha[i]);
    }

    float linearZ = 2.f * NearPlane * FarPlane / (NearPlane + FarPlane + (2.f * depth - 1.f) * (FarPlane - NearPlane));

    float blend = smoothstep(0.f, 0.65f, CoC - 0.25f);

    if (blend < 1.f && minDepth > 0.f && minDepth < linearZ + 50.f)
        blend = 1.f;

	Color = vec4(DOFed.rgb, blend);
}