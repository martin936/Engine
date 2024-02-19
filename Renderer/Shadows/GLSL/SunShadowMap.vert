// shadertype=glsl

#version 450

layout(location = 0) in vec3 Position;


layout(push_constant) uniform pc0
{
	mat4x4	SunShadowMap;
	mat3x4	ModelMatrix;
};


void main() 
{
	gl_Position = SunShadowMap * vec4(vec4(Position, 1.f) * ModelMatrix, 1.f);
}
