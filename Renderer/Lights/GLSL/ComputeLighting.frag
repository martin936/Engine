#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "Lighting.glsl"
#include "Clustered.glsl"


layout (binding = 7, std140) uniform cb7
{
	mat4	m_View;
	mat4	m_Proj;
	mat4	m_ViewProj;
	mat4	m_InvView;
	mat4	m_InvViewProj;

	mat4	m_LastView;
	mat4	m_LastProj;
	mat4	m_LastViewProj;
	mat4	m_LastInvView;
	mat4	m_LastInvViewProj;

	vec4	m_Eye;
};


layout (binding = 8, std140) uniform cb8
{
	SLight lightData[128];
};


layout (binding = 9, std140) uniform cb9
{
	SLightShadow shadowLightData[128];
};


layout(push_constant) uniform pc0
{
	vec4 Params;
	vec4 SunColor;
	vec4 SunDir;
};

#define NumSlices			(Params.x)
#define SkyLightIntensity	(Params.z)
#define Near				(Params.w)
#define Far					(SunDir.w)


layout(location = 0) in vec2 Texcoords;


layout(location = 0) out vec3 Diffuse;
layout(location = 1) out vec3 Specular;


layout(binding = 0) uniform utexture3D	LightListPtr;

layout(binding = 1, std430) readonly buffer buf1
{
	uint	dummy;
	uint	LightIndices[];
};

layout(binding = 2) uniform texture2D			ZBuffer;
layout(binding = 3) uniform texture2D			AlbedoTex;
layout(binding = 4) uniform texture2D			NormalTex;
layout(binding = 5) uniform texture2D			InfoTex;
layout(binding = 6) uniform texture2DArray		FilteredShadows;



vec3 DecodeNormal(in vec3 e) 
{
	e = e * 2.f - 1.f;
	
	vec2 temp = vec2(e.x + e.y, e.x - e.y) * 0.5;
	vec3 v = vec3(temp, 1.0 - abs(temp.x) - abs(temp.y));

	return normalize(v) * sign(e.z);
}


void main( void )
{
	float depth = texelFetch(ZBuffer, ivec2(gl_FragCoord.xy), 0).r;

	if (depth == 0.f)
	{
		Diffuse = SkyLightIntensity.xxx;
		Specular = 0.f.xxx;
		return;
	}

	vec4 pos = m_InvViewProj * vec4(Texcoords.xy * vec2(2.f, 2.f) - vec2(1.f, 1.f), depth, 1.f);
	pos /= pos.w;

	vec4 normalTex	= texelFetch(NormalTex, ivec2(gl_FragCoord.xy), 0);
	vec4 infoTex	= texelFetch(InfoTex,	ivec2(gl_FragCoord.xy), 0);
	vec4 albedo		= texelFetch(AlbedoTex, ivec2(gl_FragCoord.xy), 0);

	vec3 normal;
	normal.rg = normalTex.rg * 2. - 1.;
	normal.b = sqrt(1 - clamp(dot(normal.rg, normal.rg), 0.f, 1.f));
	normal.b *= sign(normalTex.a - 0.01);

	float linearRoughness	= normalTex.b;
	float roughness			= linearRoughness * linearRoughness;
	vec3 fresnel			= (0.16f * infoTex.g * infoTex.g).xxx;
	float metallicity		= infoTex.r;

	if (metallicity > 0.f)
		fresnel = mix(fresnel, albedo.rgb, metallicity);

	vec3 view				= normalize(m_Eye.xyz - pos.xyz);

	Diffuse.rgb				= 0.f.xxx;
	Specular.rgb			= 0.f.xxx;

	if (SunColor.w > 0.f)
	{
		//int		slice		= textureSize(FilteredShadows, 0).z - 1;
		vec3	Illuminance = SunColor.w * SunColor.rgb * max(0.f, dot(-SunDir.xyz, normal)) * texelFetch(FilteredShadows, ivec3(gl_FragCoord.xy, NumSlices - 1), 0).r;

		Diffuse		+= Illuminance * DisneyDiffuse(normal, -SunDir.xyz, view, linearRoughness) * (1.f / 3.1415926f);
		Specular	+= Illuminance * min(1.f.xxx, SpecularGGX(roughness, normal, -SunDir.xyz, view, fresnel));
	}

	/*uint index				= GetLightListIndex(LightListPtr, texCoords, depth, Near, Far);
	uint numLights			= index == 0xffffffff ? 0 : LightIndices[index];
	index++;

	uint numShadows			= 0;

	while (numLights > 0)
	{
		uint lightID		= LightIndices[index];

		if (lightID == subgroupMin(lightID))
		{
			vec3 l, Illuminance;

			SLight light;

			if ((lightID & (1 << 15)) == 0)
				light = lightData[lightID];
			else
				light = shadowLightData[lightID & 0x7fff].m_light;

			ComputeLight(light, pos.xyz, normal, Illuminance, l);

			if ((lightID & (1 << 15)) != 0)
			{
				Illuminance *= texelFetch(FilteredShadows, ivec3(gl_FragCoord.xy, numShadows), 0).r;
				numShadows++;
			}

			Diffuse.rgb		+= Illuminance * DisneyDiffuse(normal, l, view, linearRoughness) * (1.f / 3.1415926f);
			Specular.rgb	+= Illuminance * SpecularGGX(roughness, normal, l, view, fresnel); 

			numLights--;
			index++;
		}
	}*/

	//Diffuse.rgb *= 1.f - metallicity;
	//Diffuse = max(0.f, dot(normal, -SunDir.xyz)).xxx;
}
