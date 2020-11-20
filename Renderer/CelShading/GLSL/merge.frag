// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout(location = 0) out vec4 color;

layout(location = 0) uniform sampler2D Edges;
layout(location = 1) uniform sampler2D Base;

void main( void )
{
	color = texture(Edges, interp.uv) * texture(Base, interp.uv);
}
