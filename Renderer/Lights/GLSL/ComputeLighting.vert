// shadertype=glsl

#version 450

layout(location = 0) in vec3 Position;

layout(location = 0) out vec2 Texcoords;

void main() 
{
	Texcoords = Position.xy * 0.5f + 0.5f;

	gl_Position = vec4( Position, 1.f );
}
