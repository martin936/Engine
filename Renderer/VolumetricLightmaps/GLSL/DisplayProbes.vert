// shadertype=glsl

#version 450

layout(location = 0) in vec3 Position;

layout (std140) uniform cb0
{
	mat4	WorldViewProj;
};


void main() 
{
	vec4 pos = WorldViewProj * vec4( Position, 1.f );
    
	gl_PointSize	= 20.f;
    gl_Position		= pos;
}
