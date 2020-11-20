#extension GL_EXT_samplerless_texture_functions : require

#define DISTRIB_OFFSET 10.f


uint GetLightListIndex(utexture3D LightGrid, vec2 texCoords, float depth, float p_Near, float p_Far)
{
	float Zd = 2.f * p_Far * p_Near / (p_Far + p_Near + (depth * 2.f - 1.f) * (p_Far - p_Near));
	float Zc = log2((Zd + DISTRIB_OFFSET) / (p_Near + DISTRIB_OFFSET)) / log2((p_Far + DISTRIB_OFFSET) / (p_Near + DISTRIB_OFFSET));

	uvec3 gridSize = textureSize(LightGrid, 0).xyz;

	ivec3 gridCoords = ivec3(texCoords * gridSize.xy, clamp(uint(Zc * gridSize.z), 0U, gridSize.z - 1));

	return texelFetch(LightGrid, gridCoords, 0).r;
}