// shadertype=glsl

#version 450


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoords;

out vec2 Texc0;

void main()
{
	Texc0 = texcoords;
	gl_Position = vec4 (position, 1.f);
}