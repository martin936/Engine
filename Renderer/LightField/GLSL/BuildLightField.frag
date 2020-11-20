#version 450
#extension GL_GOOGLE_include_directive : require

#include "../../Lights/GLSL/Lighting.glsl"

layout(push_constant) uniform pc0
{
	uint mask[12];
	uint matID;
	float Near;
	float Far;
};

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 WorldPos;
layout(location = 3) in vec2 Zcmp;


layout(location = 0) out uvec4 gbuffer;


layout(binding = 1, std140) uniform cb1
{
	vec4	ProbePos[64];
};


void main( void )
{
	vec2 n = EncodeOct(normal);

	gbuffer.x = (uint(n.y * 255.f) << 8U) | uint(n.x * 255.f);
	
	uint uv = packHalf2x16(texcoord);
	gbuffer.y = uv >> 16U;
	gbuffer.z = uv & 0xffff;

	gbuffer.w = matID;

	float depth = distance(WorldPos, ProbePos[gl_Layer / 6].xyz);

	gl_FragDepth = ((2.f * (Far * Near) / depth - (Far + Near)) / (Far - Near)) * 0.5f + 0.5f;//  - abs(gl_FragCoord.z - Zcmp.x / Zcmp.y);
}
