// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (std140) uniform cb0
{
	float	Scattering;
	float	Transmittance;
	float	NearPlane;
	float	FarPlane;

	float	SurfaceFactor;
	float	Padding[3];
};


#define VOLUME_DEPTH 64


layout(location = 0) out vec4 rays;

layout(location = 0) uniform sampler2D ZMap;
layout(location = 1) uniform sampler3D Grid;


void main( void )
{
	vec4 color = vec4(0.f);
	float vis = 1.f;

	float fZDist = textureLod(ZMap, interp.uv, 0.f).r * 2.f - 1.f;
	float depth = 2.f * (FarPlane * NearPlane) / (FarPlane + NearPlane - (fZDist * 2.f - 1.f) * (FarPlane - NearPlane));

	for (uint i = 0; i < VOLUME_DEPTH; i++)
	{
		float ZCellMin = float(i) / float(VOLUME_DEPTH);
		float ZCellMax = float(i + 1.f) / float(VOLUME_DEPTH);

		float minZ = NearPlane * pow(FarPlane / NearPlane, ZCellMin);
		float maxZ = NearPlane * pow(FarPlane / NearPlane, ZCellMax);

		if (depth < minZ)
			break;

		vec4 light = textureLod(Grid, vec3(interp.uv, (i + 0.5f) / float(VOLUME_DEPTH)), 0.f);

		float cellWidth = min(depth - minZ, maxZ - minZ);
		float cellVolume = SurfaceFactor * 0.5f * (minZ + maxZ) * cellWidth;

		color.rgb += light.rgb * max(0.f, 1.f - exp(- min(depth, maxZ) * Transmittance)) / Transmittance * cellVolume;

		vis *= exp(- cellWidth * Transmittance);
	}
	
	color.a =	vis;

	rays = color;
}
