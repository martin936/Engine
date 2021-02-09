#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "Lighting.glsl"
#include "Clustered.glsl"


layout (binding = 25, std140) uniform cb25
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


layout (binding = 26, std140) uniform cb26
{
	SLight lightData[128];
};


layout (binding = 27, std140) uniform cb27
{
	SLightShadow shadowLightData[128];
};


layout(push_constant) uniform pc0
{
	vec4 Center0;
	vec4 Size0;
	vec4 Center1;
	vec4 Size1;
	vec4 Center2;
	vec4 Size2;

	vec4 Params;
	vec4 SunColor;
	vec4 SunDir;
};


#define EnableGI			(Params.x > 0.5f)
#define EnableAO			(Params.y > 0.5f)
#define SkyLightIntensity	(Params.z)
#define Near				(Params.w)
#define Far					(SunDir.w)


layout(location = 0) in vec2 Texcoords;


layout(location = 0) out vec4 Diffuse;
layout(location = 1) out vec4 Specular;


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
layout(binding = 7) uniform texture2D			AOMap;

#if FP16_IRRADIANCE_PROBES
layout(binding = 8) uniform texture2DArray		IrradianceFieldFine;
#else
layout(binding = 8) uniform utexture2DArray		IrradianceFieldFine;
#endif

layout(binding = 9) uniform itexture2DArray		ProbeMetadataFine;
layout(binding = 10) uniform texture2DArray		shProbesFine;
layout(binding = 11) uniform texture3D			ProbeOcclusionFine0;
layout(binding = 12) uniform texture3D			ProbeOcclusionFine1;

#if FP16_IRRADIANCE_PROBES
layout(binding = 13) uniform texture2DArray		IrradianceFieldCoarse;
#else
layout(binding = 13) uniform utexture2DArray	IrradianceFieldCoarse;
#endif

layout(binding = 14) uniform itexture2DArray	ProbeMetadataCoarse;
layout(binding = 15) uniform texture2DArray		shProbesCoarse;
layout(binding = 16) uniform texture3D			ProbeOcclusionCoarse0;
layout(binding = 17) uniform texture3D			ProbeOcclusionCoarse1;

#if FP16_IRRADIANCE_PROBES
layout(binding = 18) uniform texture2DArray		IrradianceFieldFar;
#else
layout(binding = 18) uniform utexture2DArray	IrradianceFieldFar;
#endif

layout(binding = 19) uniform itexture2DArray	ProbeMetadataFar;
layout(binding = 20) uniform texture2DArray		shProbesFar;
layout(binding = 21) uniform texture3D			ProbeOcclusionFar0;
layout(binding = 22) uniform texture3D			ProbeOcclusionFar1;
layout(binding = 23) uniform sampler			sampLinear;
layout(binding = 24) uniform texture2D			BRDF;



vec3 DecodeNormal(in vec3 e) 
{
	e = e * 2.f - 1.f;
	
	vec2 temp = vec2(e.x + e.y, e.x - e.y) * 0.5;
	vec3 v = vec3(temp, 1.0 - abs(temp.x) - abs(temp.y));

	return normalize(v) * sign(e.z);
}


float VanDerCorput2(uint inBits)
{
	uint bits = inBits;
	bits = (bits << 16U) | (bits >> 16U);
	bits = ((bits & 0x55555555U) << 1U) | ((bits & 0xAAAAAAAAU) >> 1U);
	bits = ((bits & 0x33333333U) << 2U) | ((bits & 0xCCCCCCCCU) >> 2U);
	bits = ((bits & 0x0F0F0F0FU) << 4U) | ((bits & 0xF0F0F0F0U) >> 4U);
	bits = ((bits & 0x00FF00FFU) << 8U) | ((bits & 0xFF00FF00U) >> 8U);
	return bits * 2.3283064365386963e-10f;
}


float VanDerCorput3(uint inBits)
{
	float f = 1.f;
	float r = 0.f;
	uint i = inBits;

	while (i > 0)
	{
		f /= 3.f;
		r += f * (i % 3U);
		i /= 3;
	}

	return r;
}


float sdBox( vec3 p, vec3 b )
{
	vec3 q = abs(p) - b;
	return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}


void CascadeGI(out vec3 Diffuse, out vec3 Specular, in vec3 pos, in vec3 normal)
{
	Diffuse		= 0.f.xxx;
	Specular	= 0.f.xxx;

	vec3 Center = Center0.xyz;
	vec3 Size	= Size0.xyz;

	vec3 giPos = (pos.xyz - Center.xyz) / Size.xyz + 0.5f.xxx;

	float d0 = sdBox(giPos - 0.5f, 0.5f.xxx); 

	if (d0 < 0.f)
		Diffuse = ComputeGI(IrradianceFieldFine, ProbeMetadataFine, ProbeOcclusionFine0, ProbeOcclusionFine1, sampLinear, pos, giPos, Center.xyz, Size.xyz, normal, 0.f.xxx) * (1.f / 3.1415926f);

	if (d0 > -0.1f)
	{
		Center	= Center1.xyz;
		Size	= Size1.xyz;

		giPos = (pos.xyz - Center.xyz) / Size.xyz + 0.5f.xxx;
		float d1 = sdBox(giPos - 0.5f, 0.5f.xxx);

		if (d1 < 0.f)
		{
			vec3 diffuse1 = ComputeGI(IrradianceFieldCoarse, ProbeMetadataCoarse, ProbeOcclusionCoarse0, ProbeOcclusionCoarse1, sampLinear, pos, giPos, Center.xyz, Size.xyz, normal, 0.f.xxx) * (1.f / 3.1415926f);

			if (d0 < 0.f)
				Diffuse = mix(diffuse1, Diffuse, -d0 * 10.f);

			else
				Diffuse = diffuse1;
		}

		if (d1 > -0.1f)
		{
			Center	= Center2.xyz;
			Size	= Size2.xyz;

			giPos = (pos.xyz - Center.xyz) / Size.xyz + 0.5f.xxx;
			float d2 = sdBox(giPos - 0.5f, 0.5f.xxx);

			if (d2 < 0.f)
			{
				vec3 diffuse2 = ComputeGI(IrradianceFieldFar, ProbeMetadataFar, ProbeOcclusionFar0, ProbeOcclusionFar1, sampLinear, pos, giPos, Center.xyz, Size.xyz, normal, 0.f.xxx) * (1.f / 3.1415926f);

				if (d1 < 0.f)
					Diffuse = mix(diffuse2, Diffuse, -d1 * 10.f);

				else
					Diffuse = diffuse2;
			}
		}
	}		
}


void main( void )
{
	float depth = texelFetch(ZBuffer, ivec2(gl_FragCoord.xy), 0).r;

	if (depth == 0.f)
	{
		Diffuse = SkyLightIntensity.xxxx;
		Specular = 0.f.xxxx;
		return;
	}

	vec4 pos = m_InvViewProj * vec4(Texcoords.xy * vec2(2.f, 2.f) - vec2(1.f, 1.f), depth, 1.f);
	pos /= pos.w;

	vec4 normalTex	= texelFetch(NormalTex, ivec2(gl_FragCoord.xy), 0);
	vec4 infoTex	= texelFetch(InfoTex, ivec2(gl_FragCoord.xy), 0);
	vec4 albedo		= texelFetch(AlbedoTex, ivec2(gl_FragCoord.xy), 0);

	vec3 normal = DecodeNormal(normalTex.rga);

	float linearRoughness	= normalTex.b;
	float roughness			= linearRoughness * linearRoughness;
	vec3 fresnel			= (0.16f * infoTex.g * infoTex.g).xxx;
	float metallicity		= infoTex.r;

	if (metallicity > 0.f)
		fresnel = mix(fresnel, pow(albedo.rgb, 2.2f.xxx), metallicity);

	vec3 view				= normalize(m_Eye.xyz - pos.xyz);

	Diffuse.rgb				= 0.f.xxx;
	Specular.rgb			= 0.f.xxx;
	
	vec3 bentNormal = normal;

	vec3 AO = 1.f.xxx;
	
	if (EnableAO)
	{
		AO = texelFetch(AOMap, ivec2(gl_FragCoord.xy), 0).rgb;
	}

	vec2 screenSize			= textureSize(ZBuffer, 0).xy;
	vec2 texCoords			= gl_FragCoord.xy / screenSize;

	if (EnableGI)
	{
		CascadeGI(Diffuse.rgb, Specular.rgb, pos.xyz, normal);
	}

	if (SunColor.w > 0.f)
	{
		int slice = textureSize(FilteredShadows, 0).z - 1;
		vec3 Illuminance = SunColor.w * SunColor.rgb * max(0.f, dot(-SunDir.xyz, normal)) * texelFetch(FilteredShadows, ivec3(gl_FragCoord.xy, slice), 0).r;

		Diffuse.rgb		+= Illuminance * DisneyDiffuse(normal, -SunDir.xyz, view, linearRoughness) * (1.f / 3.1415926f);
		Specular.rgb	+= Illuminance * SpecularGGX(roughness, normal, -SunDir.xyz, view, fresnel);
	}

	uint index				= GetLightListIndex(LightListPtr, texCoords, depth, Near, Far);
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
	}

	Diffuse.rgb *= 1.f - metallicity;

	Diffuse.a = 0.f;
	Specular.a = 0.f;
}
