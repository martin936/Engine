// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (location = 0) out float Depth;
layout (location = 1) out vec4	Color;

layout(location = 0) uniform sampler2D ZMap;
layout(location = 1) uniform sampler2D ColorMap;


void main( void )
{
	Color = texture(ColorMap, interp.uv).rgba;

	vec4 vDepth = textureGather(ZMap, interp.uv);
	Depth = min(vDepth.x, min(vDepth.y, min(vDepth.z, vDepth.w)));
}
