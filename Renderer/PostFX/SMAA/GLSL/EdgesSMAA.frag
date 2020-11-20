// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 texcoord;
	vec4 offset[3];
} interp;


layout(std140) uniform cb0
{
	vec4 SMAA_RT_METRICS;
};


#define SMAA_PRESET_ULTRA
#define SMAA_GLSL_4
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1

#include "SMAA.glsl"


layout( location = 0 ) out vec2 output_color;

layout(location = 0) uniform sampler2D colorTex;


void main( void )
{
    output_color = SMAAColorEdgeDetectionPS(interp.texcoord, interp.offset, colorTex);
}
