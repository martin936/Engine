#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(binding = 0) uniform texture2D	LastFrameRadiance;
layout(binding = 1) uniform texture2D	MotionVectors;
layout(binding = 2) uniform sampler		sampLinear;


layout(location = 0) out vec4 Radiance;


void main()
{
	vec2 mv = texelFetch(MotionVectors, ivec2(gl_FragCoord.xy), 0).rg;

	float v = length(mv);

	mv *= abs(mv);

	vec4 history = textureLod(sampler2D(LastFrameRadiance, sampLinear), gl_FragCoord.xy / textureSize(LastFrameRadiance, 0).xy - mv, 0);

	if (abs(v - history.w) > 200.f * length(1.f / textureSize(LastFrameRadiance, 0).xy))
		history.rgb = 0.f.xxx;

	Radiance = vec4(history.rgb, dot(history.rgb, 1.f.xxx) == 0.f ? 0.f : 1.f);
}
