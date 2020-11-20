#version 450
#extension GL_EXT_samplerless_texture_functions : require
#extension GL_GOOGLE_include_directive : require

#include "../../Lights/GLSL/Lighting.glsl"

layout(location = 0) in vec2 texcoords;

layout(binding = 0) uniform textureCubeArray	CubeMaps;
layout(binding = 1) uniform sampler				samp;


void main(void)
{
	gl_FragDepth = textureLod(samplerCubeArray(CubeMaps, samp), vec4(DecodeOct(texcoords), gl_Layer), 0).r;
}
