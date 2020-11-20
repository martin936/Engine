// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout(location = 0) out vec4 TotalIrradiance;


layout(location = 0) uniform sampler2D IndirectIrradiance;


void main( void )
{
	TotalIrradiance = textureLod(IndirectIrradiance, interp.uv, 0.f);
}
