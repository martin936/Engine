// shadertype=glsl

#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout (std140) uniform cb0
{
	mat4	ViewProj;
	vec4	Eye;

	vec4	DiffuseColor;

	vec3	LightDir;
	float	LightPower;

	mat4	ShadowMatrix;
	vec4	ShadowPos;
	vec4	ShadowDir;

	float	NearPlane;
	float	FarPlane;
};

out struct VS_OUTPUT
{
	vec3 position;
	vec3 normal;
} interp;

void main() 
{
	vec4 pos = ViewProj*vec4( position, 1.f );
    
    gl_Position = pos;

	interp.position = position;
	interp.normal = normal;
}
