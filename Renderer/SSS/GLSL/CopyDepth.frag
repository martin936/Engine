// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout(location = 0) uniform sampler2D ZMap;

layout(location = 0) out float Depth;


void main(void)
{
	Depth = texture(ZMap, interp.uv).r;
}
