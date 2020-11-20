// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (std140) uniform cb0
{
	mat4	InvViewProj;
	mat4	LastViewProj;
};


layout (location = 0) out float Depth;
layout (location = 1) out vec4	Color;

layout(location = 0) uniform sampler2D ZMap;
layout(location = 1) uniform sampler2D ColorMap;


void main( void )
{
	Depth = textureLod(ZMap, interp.uv, 0.f).r;

	vec4 pos = InvViewProj * vec4(interp.uv * 2.f - 1.f, Depth * 2.f - 1.f, 1.f);
	pos /= pos.w;

	vec4 proj_pos = LastViewProj * vec4(pos.xyz, 1.f);
	proj_pos.xy = 0.5f * proj_pos.xy / proj_pos.w + 0.5f;

	Color = textureLod(ColorMap, proj_pos.xy, 0.f).rgba;
}
