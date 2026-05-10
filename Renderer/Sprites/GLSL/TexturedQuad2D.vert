// shadertype=glsl

#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec4 color;

layout(location = 0) out vec2 vTexcoord;
layout(location = 1) out vec4 vColor;

void main()
{
	vTexcoord = texcoord;
	vColor    = color;
	gl_Position = vec4(position, 1.f);
}
