#version 450

layout(location = 0) in vec3 WorldPos;

layout(binding = 0, r32ui) uniform writeonly uimage3D	Voxels;

void main( void )
{
	ivec3 size = imageSize(Voxels).xyz;

	ivec3 coords = ivec3(WorldPos * size);

	imageStore(Voxels, coords, 1u.xxxx);
}
