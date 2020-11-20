// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 Texcoords;
} interp;


layout(location = 0) uniform sampler2D Base;
layout(location = 0) out vec4 Color;


void main(void)
{
	Color = 10.f * clamp(textureLod(Base, interp.Texcoords, 0.f) - 1.f.xxxx, 0.f.xxxx, 1.f.xxxx);
}
