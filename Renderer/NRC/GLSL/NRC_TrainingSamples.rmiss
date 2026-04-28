#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

struct Payload 
{
    uint	albedo;
    uint	packedNormal;
	float	dist;
};


layout (location=1) rayPayloadInEXT Payload payload;

layout(binding = 4) uniform sampler		samp;
layout(binding = 5) uniform textureCube SkyLight;


void main() 
{
    payload.albedo          = packUnorm4x8(textureLod(samplerCube(SkyLight, samp), gl_WorldRayDirectionEXT, 0.f));
	payload.packedNormal    = 0;
    payload.dist            = -1.f;
}
