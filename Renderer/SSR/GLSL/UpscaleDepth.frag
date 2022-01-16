#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) in vec2 Texc0;

layout(binding = 0) uniform texture2D	Source;
layout(binding = 1) uniform sampler		samp;

layout(location = 0) out float depth;


layout(push_constant) uniform pc0
{
	float Near;
	float Far;
};


void main( void )
{
	ivec2 texSize = textureSize(Source, 0).xy;
	vec2 texcoord = Texc0;

	vec2 filterWeight = (texcoord * vec2(texSize)) - vec2(0.5);
	texcoord.xy = (floor(filterWeight) + 0.5f) / texSize;

	vec4 z = textureGather(sampler2D(Source, samp), texcoord, 0);
	z = 2.f * Near * Far / (Near + Far - (2.f * z - 1.f) * (Near - Far));

	filterWeight = clamp(filterWeight - floor(filterWeight), 0.f.xx, 1.f.xx);

	float tmp0 = mix(z.x, z.y, filterWeight.x);
	float tmp1 = mix(z.w, z.z, filterWeight.x);

	float d = mix(tmp1, tmp0, filterWeight.y);

	depth = ((Near + Far) / (Near - Far) + 2.f * Near * Far / (d * (Far - Near))) * 0.5f + 0.5f;
}
