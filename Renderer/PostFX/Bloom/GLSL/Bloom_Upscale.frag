#version 450

layout(location = 0) in vec2 Texc0;

layout(binding = 0) uniform texture2D Base;
layout(binding = 1) uniform sampler sampLinear;

layout(location = 0) out vec4 Output;


void main()
{
	Output = textureLod(sampler2D(Base, sampLinear), Texc0, 0);
}
