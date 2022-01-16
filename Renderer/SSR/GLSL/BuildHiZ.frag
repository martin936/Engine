#version 450

layout(location = 0) in vec2 Texc0;

layout(binding = 0) uniform texture2D ZMap;
layout(binding = 1) uniform sampler samp;

layout (location = 0) out float Depth;

void main( void )
{
	vec4 z = textureGather(sampler2D(ZMap, samp), Texc0, 0);

	Depth = max(z.x, max(z.y, max(z.z, z.w)));
}
