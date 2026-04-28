#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#define OBJECT_BUFFER_SLOT      1
#define MATERIAL_BUFFER_SLOT    2

layout(binding = 3) uniform texture2D	MaterialTex[];
layout(binding = 4) uniform sampler		samp;

#include "../../RTX/GLSL/RayTracing.glsli"

hitAttributeEXT vec2 attribs;

struct Payload 
{
    uint	albedo;
    uint	packedNormal;
	float	dist;
};

layout (location=1) rayPayloadInEXT Payload payload;


// Returns ±1
vec2 signNotZero(vec2 v)
{
	return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}

// Assume normalized input. Output is on [-1, 1] for each component.
vec2 EncodeOct(in vec3 v)
{
	// Project the sphere onto the octahedron, and then onto the xy plane
	vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
	// Reflect the folds of the lower hemisphere over the diagonals
	return ((v.z <= 0.0) ? ((1.0 - abs(p.yx)) * signNotZero(p)) : p) * 0.5f + 0.5f.xx;
}


void main() 
{    
    uint matID;
    vec2 texcoords;
    vec3 normal;
    vec4 albedo;

    ExtractTexcoordsAndNormals(attribs, texcoords, normal, matID);

    uint texID = materials[matID].DiffuseTextureID;

    if (texID != 0xffffff)
        albedo = textureLod(sampler2D(MaterialTex[texID], samp), texcoords, 0.f);
    else
        albedo = materials[matID].Color;

    payload.albedo          = packUnorm4x8(albedo);
    payload.packedNormal    = packUnorm2x16(EncodeOct(normal));
    payload.dist            = gl_HitTEXT;
}
