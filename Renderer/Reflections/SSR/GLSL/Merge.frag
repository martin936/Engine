// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (location = 0) out vec4 Color;

layout(location = 0) uniform sampler2D SSRMap;
layout(location = 1) uniform sampler2D BRDFMap;
layout(location = 2) uniform sampler2D InfoMap;
layout(location = 3) uniform sampler2D FresnelMap;


void main( void )
{
	vec4	reflectedData	= vec4(0.44198f.xxx, 1.f) * textureLod(SSRMap, interp.uv, 0);
	vec2	size			= textureSize(SSRMap, 0);
	vec2	pixelSize		= 1.f / size;

	reflectedData.rgb += 0.27901f * textureLod(SSRMap, interp.uv - vec2(0.f, pixelSize.y), 0.f).rgb;
	reflectedData.rgb += 0.27901f * textureLod(SSRMap, interp.uv + vec2(0.f, pixelSize.y), 0.f).rgb;

	vec3	fresnel			= texelFetch(FresnelMap, ivec2(gl_FragCoord.xy), 0).xyz;
	float	linearRoughness = texelFetch(InfoMap, ivec2(gl_FragCoord.xy), 0).g;
	float	roughness		= linearRoughness * linearRoughness;

	float NdotV = reflectedData.w;

	if (NdotV < 0.f)
		discard;

	vec2 brdf = textureLod(BRDFMap, vec2(NdotV, roughness), 0).rg;

	Color.rgb = (fresnel * brdf.x + brdf.y) * reflectedData.rgb;

	Color.a = 0.f;
}
