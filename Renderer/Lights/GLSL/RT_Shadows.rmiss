#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

struct Payload 
{
    float shadowed;
};

layout (location=0) rayPayloadInEXT Payload payload;


void main() 
{
	payload.shadowed = 1.f;
}
