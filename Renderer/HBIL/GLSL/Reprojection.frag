// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (std140) uniform cb0
{
	mat4 InvViewProj;
	mat4 LastViewProj;
	mat4 View;
};


layout(location = 0) out vec4 TotalIrradiance;
layout(location = 1) out vec4 ReprojectedIrradiance;


layout(location = 0) uniform sampler2D ZMap;
layout(location = 1) uniform sampler2D DirectLighting;
layout(location = 2) uniform sampler2D AccumulatedIrradiance;
layout(location = 3) uniform sampler2D NormalMap;
layout(location = 4) uniform sampler2D AlbedoMap;


void main( void )
{
	float	ZDist 	= textureLod(ZMap, interp.uv, 0.f).r * 2.f - 1.f;
	vec3	albedo	= textureLod(AlbedoMap, interp.uv, 0.f).rgb;

	vec4	pos		= InvViewProj * vec4(2.f * interp.uv - 1.f, ZDist, 1.f);
	pos /= pos.w;

	vec4 PosProj	= LastViewProj * pos;
	PosProj /= PosProj.w;

	vec3	normal	= textureLod(NormalMap, interp.uv, 0.f).xyz * 2.f - 1.f;
	normal			= normalize((View * vec4(normal, 0.f)).xyz) * 0.5f + 0.5f;

	PosProj.xy		= PosProj.xy * 0.5f + 0.5f;
	float factor	= step(0.f, PosProj.x * (1.f - PosProj.x)) * step(0.f, PosProj.y * (1.f - PosProj.y));

	ReprojectedIrradiance	= textureLod(AccumulatedIrradiance, PosProj.xy, 0.f) * factor;
	TotalIrradiance			= textureLod(DirectLighting, interp.uv, 0.f) + ReprojectedIrradiance;

	TotalIrradiance.rgb *= pow(max(1e-6f.xxx, albedo), 2.2f.xxx);
	TotalIrradiance.a = uintBitsToFloat((uint(normal.x * 255.f) << 8U) | (uint(normal.y * 255.f) & 0xff));

	ReprojectedIrradiance.a	= factor;
}
