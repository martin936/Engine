// shadertype=glsl

#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;

layout (std140) uniform cb0
{
	mat4	ViewProj;
	mat4	World;
};

out struct VS_OUTPUT
{
	vec3 Normal;

} interp;

void main() 
{
	vec4 pos = (World * ViewProj) * vec4( Position, 1.f );
    
    gl_Position = pos;

	vec4 normal = World * vec4( Normal, 0.f );

	interp.Normal = normalize(normal.xyz);
}
