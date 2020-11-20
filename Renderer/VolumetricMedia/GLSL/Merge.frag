// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout( location = 0 ) out vec4 color;

layout(location = 0) uniform sampler2D VolumetricData;
layout(location = 1) uniform sampler2D Source;


void main( void )
{
	vec4 Vol = textureLod(VolumetricData, interp.uv, 0.f);

	color = textureLod(Source, interp.uv, 0.f);

	color.rgb = color.rgb * Vol.a + Vol.rgb;
}
