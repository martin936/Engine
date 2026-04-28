#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

hitAttributeEXT vec2 attribs;

struct Payload 
{
    float shadowed;
};

layout (location=0) rayPayloadInEXT Payload payload;


void main() 
{    
    payload.shadowed = 0.f;
}
