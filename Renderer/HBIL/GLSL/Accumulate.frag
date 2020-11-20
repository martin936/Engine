// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (std140) uniform cb0
{
	vec4 Params;
	vec4 Rotation;

	vec3 Padding;
	float tan_fov;

	mat4 View;
};


#define SamplePattern	Params.xy
#define PixelSize		Params.zw


layout(location = 0) out vec4 AccumulatedIrradiance;


layout(location = 0) uniform sampler2D InterleavedIrradiance;
layout(location = 1) uniform sampler2D ReprojectedIrradiance;
layout(location = 2) uniform sampler2D ZMap;


void main( void )
{
	vec4 LastIrradiance = textureLod(ReprojectedIrradiance, interp.uv, 0.f);

	vec2 Texcoords = interp.uv + SamplePattern * PixelSize;

	float fZDist_offset = textureLod(ZMap, Texcoords, 0.f).r * 2.f - 1.f;
	float fZDist = textureLod(ZMap, interp.uv, 0.f).r * 2.f - 1.f;

	vec2 samples	= floor(interp.uv / PixelSize + 0.5f.xx);
	vec2 lerpVals	= clamp(interp.uv / PixelSize - samples, -0.5f.xx, 0.5f.xx);

	vec2 delta		= abs(lerpVals - SamplePattern);
	delta			= min(delta, 1.f - delta);

	float factor	= 0.01f * clamp(1.f - length(delta) * 0.707f, 0.f, 1.f);
	factor			= mix(1.f, factor, LastIrradiance.a);

	if (abs(fZDist - fZDist_offset) > 1e-4f)
		factor = 0.f;

	AccumulatedIrradiance = mix(LastIrradiance, textureLod(InterleavedIrradiance, interp.uv, 0.f), factor);
}
