#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

struct Hit 
{
    vec3 position;
    vec3 normal;
};

struct Ray_Payload 
{
    Hit  hit;
    uint color;
};

vec4 ToRGBE(vec3 inColor)
{
    float base = max(inColor.r, max(inColor.g, inColor.b));
    int e;
    float m = frexp(base, e);
    return vec4(clamp(inColor.rgb / exp2(e), 0.f.xxx, 1.f.xxx), e + 127);
}

vec3 FromRGBE(vec4 inColor)
{
    return inColor.rgb * exp2(inColor.a - 127);
}


vec3 UnpackRGBE(uint packedInput)
{
    vec4 unpackedOutput;
    uvec4 p = uvec4((packedInput & 0xFFU),
		(packedInput >> 8U) & 0xFFU,
		(packedInput >> 16U) & 0xFFU,
		(packedInput >> 24U));

    unpackedOutput = vec4(p) / vec4(255, 255, 255, 1.0f);
    return FromRGBE(unpackedOutput);
}


uint PackRGBE(vec3 unpackedInput)
{
    uvec4 u = uvec4(ToRGBE(unpackedInput) * vec4(255, 255, 255, 1.0f));
    uint packedOutput = (u.w << 24U) | (u.z << 16U) | (u.y << 8U) | u.x;
    return packedOutput;
}


layout (location=0) rayPayloadInEXT Ray_Payload payload;

layout(binding = 7) uniform textureCube		Skybox;
layout(binding = 6) uniform sampler		    samp;


void main() 
{
    payload.hit.position    = 0.f.xxx;
    payload.hit.normal      = 0.f.xxx;
    payload.color           = PackRGBE(200.f * textureLod(samplerCube(Skybox, samp), vec3(-gl_WorldRayDirectionEXT.x, gl_WorldRayDirectionEXT.z, -gl_WorldRayDirectionEXT.y), 0).rgb);
}
