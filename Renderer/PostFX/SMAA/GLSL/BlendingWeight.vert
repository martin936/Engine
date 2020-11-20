// shadertype=glsl

#version 450

layout (location = 0) in vec3 position;

out struct PS_INPUT
{
	vec2 texcoord;
	vec2 pixcoord;
	vec4 offset[3];
} interp;


layout(std140) uniform cb0
{
	vec4 SMAA_RT_METRICS;
};

#define SMAA_PRESET_ULTRA
#define SMAA_GLSL_4
#define SMAA_INCLUDE_VS 1
#define SMAA_INCLUDE_PS 0

#include "SMAA.glsl"

 
void main(void)
{
	vec2 texcoord = ( position.xy + vec2( 1.f, 1.f ) ) * 0.5f;
	gl_Position = vec4( position, 1.f );
	
	interp.texcoord = texcoord;

	SMAABlendingWeightCalculationVS(texcoord, interp.pixcoord, interp.offset);
}
