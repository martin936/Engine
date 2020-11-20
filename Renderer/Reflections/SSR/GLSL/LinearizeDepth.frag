// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (std140) uniform cb0
{
	vec4 NearFar;
};

#define Near	NearFar.x
#define Far		NearFar.y


layout (location = 0) out float Depth;

layout(location = 0) uniform sampler2D ZMap;


void main( void )
{
	float z = textureLod(ZMap, interp.uv, 0).r * 2.f - 1.f;

	Depth = 2.f* Near * Far / (Near + Far - (2.f * z - 1.f) * (Far - Near));
}
