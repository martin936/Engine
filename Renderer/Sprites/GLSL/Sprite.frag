// shadertype=glsl

#version 450

layout (location = 0) out vec4 Color;

layout (std140) uniform cb0
{
	vec4	ModulateColor;
};

in vec2 Texc0;

void main()
{
	Color = pow(ModulateColor, vec4(vec3(2.2f), 1.f));
}
