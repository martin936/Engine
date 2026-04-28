#version 450

layout(binding = 0) uniform texture2D	Source;
layout(binding = 1) uniform sampler		sampLinear;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main(void)
{
	color = texture(sampler2D(Source, sampLinear), uv);
}
