#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(binding = 0) uniform texture2D	Radiance;
layout(binding = 1) uniform sampler		sampLinear;

layout(location = 0) in vec2 Texc0;

layout(location = 0) out vec4 mipColor;


void main()
{
	mipColor = textureLod(sampler2D(Radiance, sampLinear), Texc0, 0);
}
