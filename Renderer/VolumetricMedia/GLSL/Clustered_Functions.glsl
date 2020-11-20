

#include "PointLight_Functions.glsl"
#include "SpotLight_Functions.glsl"
#include "AreaDiskLight_Functions.glsl"
#include "AreaSphereLight_Functions.glsl"



void ComputeLighting(out vec3 Illuminance, out vec3 L, in vec3 Pos, in uint lightType, in uint lightID)
{
	Illuminance = vec3(0.f);

	if (lightType == 0U) // PointLight
	{
		Compute_PointLight(Illuminance, L, Pos, Light[lightID]);
	}

	else if (lightType == 1U) // PointLightShadow
	{
		Compute_PointLightShadow(Illuminance, L, Pos, LightShadow[lightID]);
	}

	else if (lightType == 2U) // SpotLight
	{
		Compute_SpotLight(Illuminance, L, Pos, Light[lightID]);
	} 

	else if (lightType == 3U) // SpotLightShadow
	{
		Compute_SpotLightShadow(Illuminance, L, Pos, LightShadow[lightID]);
	}

	else if (lightType == 4U) // AreaDiskLight
	{

	}

	else if (lightType == 5U) // AreaDiskLightShadow
	{

	}

	else if (lightType == 6U) // AreaSphereLight
	{

	}

	else if (lightType == 7U) // AreaSphereLightShadow
	{

	}
}


uvec2 grid2tex(uvec3 gridCoords, uint offset)
{
	uint index = (gridCoords.z * 4096U + gridCoords.y * 64U + gridCoords.x) * 16U + offset;

	uvec2 texCoords;

	texCoords.x = index / 2048U;
	texCoords.y = index % 2048U;

	return texCoords;
}


