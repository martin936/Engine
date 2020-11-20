// shadertype=glsl

#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoords;

layout (std140) uniform cb1
{
	mat4	WorldViewProj;
};

out vec2 Texc0;

void main() 
{
	vec4 pos = WorldViewProj*vec4( position, 1.f );
    
    gl_Position = pos;

	Texc0 = texcoords;
}