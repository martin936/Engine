// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (location = 0) out vec4 Color;

layout(location = 0) uniform sampler2D Base;


void main( void )
{
	Color = textureLod(Base, interp.uv, 0.f);
}
