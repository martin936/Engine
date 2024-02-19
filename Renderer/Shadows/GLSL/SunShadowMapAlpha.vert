// shadertype=glsl

#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 Texcoord;

layout(location = 0) out vec2 texc0;


layout(push_constant) uniform pc0
{
	mat4x4	SunShadowMap;
	mat3x4	ModelMatrix;
};


void main() 
{
	gl_Position = SunShadowMap * vec4(vec4(Position, 1.f) * ModelMatrix, 1.f);

	texc0	= Texcoord;
	texc0.y	= 1.f - Texcoord.y;
}
