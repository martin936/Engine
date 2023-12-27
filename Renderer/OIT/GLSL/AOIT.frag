#version 450
#extension GL_ARB_fragment_shader_interlock : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "../../Lights/GLSL/Lighting.glsl"
#include "../../Lights/GLSL/Clustered.glsl"
#include "AOIT.glsl"


layout(location= 0) in struct
{
	vec3	Normal;
	vec3	Tangent;
	vec3	Bitangent;
	vec3	WorldPos;
	vec2	Texcoords;
	vec3	CurrPos;
	vec3	LastPos;
} interp;



layout(binding = 4) uniform utexture3D	LightListPtr;

layout(binding = 5, std430) readonly buffer buf1
{
	uint	dummy;
	uint	LightIndices[];
};

layout(binding = 6) uniform texture2D			MaterialTex[];
layout(binding = 7) uniform sampler				sampLinear;

layout(binding = 8) uniform texture2DArray		ShadowMaps;
layout(binding = 9) uniform texture2DArray		SunShadowMap;
layout(binding = 10) uniform sampler				sampShadow;

layout(binding = 11) uniform texture2DArray		IrradianceFieldFine;

layout(binding = 12) uniform itexture2DArray	ProbeMetadataFine;
layout(binding = 13) uniform texture3D			ProbeOcclusionFine0;
layout(binding = 14) uniform texture3D			ProbeOcclusionFine1;

layout(binding = 15) uniform texture2DArray		IrradianceFieldCoarse;

layout(binding = 16) uniform itexture2DArray	ProbeMetadataCoarse;
layout(binding = 17) uniform texture3D			ProbeOcclusionCoarse0;
layout(binding = 18) uniform texture3D			ProbeOcclusionCoarse1;

layout(binding = 19) uniform texture2DArray		IrradianceFieldFar;

layout(binding = 20) uniform itexture2DArray	ProbeMetadataFar;
layout(binding = 21) uniform texture3D			ProbeOcclusionFar0;
layout(binding = 22) uniform texture3D			ProbeOcclusionFar1;


layout (binding = 23, std140) uniform cb23
{
	vec4	Color;

	float	Roughness;
	float	Emissive;
	float	BumpHeight;
	float	Reflectivity;

	float	Metalness;
	float	SSSProfileID;
	float	SSSRadius;
	float	SSSThickness;

	uint 	DiffuseTextureID;
	uint 	NormalTextureID;
	uint 	InfoTextureID;
	uint	SSSTextureID;
};


layout (binding = 24, std140) uniform cb24
{
	SLight lightData[128];
};


layout (binding = 25, std140) uniform cb25
{
	SLightShadow shadowLightData[128];
};


layout (binding = 26, std140) uniform cb26
{
	vec4	m_SampleOffsets[8];
};


layout (binding = 27, std140) uniform cb27
{
	mat4 SunShadowMatrix;
	vec4 SunColor;
	vec4 SunDir;

	vec4 Center0;
	vec4 Size0;
	vec4 Center1;
	vec4 Size1;
	vec4 Center2;
	vec4 Size2;
	vec4 RealCenter;

	vec4 m_Eye;

	uint FrameIndex;
	float Near;
	float Far;
};


#define EnableGI (m_Eye.w > 0.5f)


float InterleavedGradientNoise()
{
	vec2 coords = vec2(gl_FragCoord.xy) + 100.f * vec2(FrameIndex);
	return fract(52.9829189*fract(0.06711056 * coords.x + 0.00583715 * coords.y));
}


float FilterShadowMap(vec3 texcoord, float depth, float radius)
{
	uint i = 0;
	float light = 0.f;

	float a = InterleavedGradientNoise() * 0.5f;
	float c = 1 - a * a;
	float s = a;

	mat2 rot = mat2(c, s, -s, c);

	for (i = 0; i < 16; i++)
	{
		vec2 offset;

		if ((i & 1) == 0)
			offset = radius * m_SampleOffsets[i >> 1].xy;
		else
			offset = radius * m_SampleOffsets[i >> 1].zw;

		offset = rot * offset;

		light += texture(sampler2DArrayShadow(ShadowMaps, sampShadow), vec4(texcoord.xy + offset , texcoord.z, depth)).r;
	}

	return light * (1.f / 16.f);
}


float GetFilteredShadow(uint lightID, vec3 pos)
{
	vec4 shadowPos;
	
	// Spot
	if (shadowLightData[lightID].m_ShadowIndex.z < 0.5f)
	{
		shadowPos = shadowLightData[lightID].m_ShadowMatrix * vec4(pos, 1);
		shadowPos.xyz = (shadowPos.xyz / shadowPos.w);
		shadowPos.xy = shadowPos.xy * vec2(0.5f, -0.5f) + 0.5f.xx;

		shadowPos.z += 4.f / 65536.f;
	}

	// Omni
	else
	{
		vec3 d = pos - shadowLightData[lightID].m_light.m_Pos.xyz;
		float depth = length(d);

		float n = shadowLightData[lightID].m_ShadowMatrix[0][0];
		float f = shadowLightData[lightID].m_ShadowMatrix[0][1];

		shadowPos.z = ((f + n - 2.f * (f * n) / depth) / (n - f)) * 0.5f + 0.5f;

		shadowPos.xy = EncodeOct(normalize(d));
	}

	float light = 1.f;

	if (shadowLightData[lightID].m_ShadowIndex.x >= 0)
	{
		light *= FilterShadowMap(vec3(shadowPos.xy, shadowLightData[lightID].m_ShadowIndex.x), shadowPos.z, 3.f * shadowLightData[lightID].m_ShadowIndex.w).r;
	}

	if (shadowLightData[lightID].m_ShadowIndex.y >= 0)
	{
		light *= FilterShadowMap(vec3(shadowPos.xy, shadowLightData[lightID].m_ShadowIndex.y), shadowPos.z, 3.f * shadowLightData[lightID].m_ShadowIndex.w).r;
	}

	return light;
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

	float d0 = sdBox((pos.xyz - RealCenter.xyz) / (Size.xyz * (1.f - 1.f / textureSize(ProbeMetadataFine, 0).xyz)), 0.5f.xxx); 

	if (d0 < 0.f)
		Diffuse = ComputeGI(IrradianceFieldFine, ProbeMetadataFine, ProbeOcclusionFine0, ProbeOcclusionFine1, sampLinear, pos, giPos, Center.xyz, Size.xyz, normal, 0.f.xxx) * (1.f / 3.1415926f);

	if (d0 > -0.1f)
	{
		Center	= Center1.xyz;
		Size	= Size1.xyz;

		giPos = (pos.xyz - Center.xyz) / Size.xyz + 0.5f.xxx;
		float d1 = sdBox((pos.xyz - RealCenter.xyz) / (Size.xyz * (1.f - 1.f / textureSize(ProbeMetadataCoarse, 0).xyz)), 0.5f.xxx); 

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
			float d2 = sdBox((pos.xyz - Center.xyz) / Size.xyz, 0.5f.xxx); 

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



vec4 Shade()
{
	vec4 albedo = 0.f.xxxx;

	if (DiffuseTextureID == 0xffffffff)
		albedo		= Color;
	else
		albedo		= texture(sampler2D(MaterialTex[DiffuseTextureID], sampLinear), interp.Texcoords);

	if (albedo.a == 0.f)
		discard;

	vec3 normal;
	float roughness;

	vec3 pos	= interp.WorldPos;
	vec3 view	= normalize(m_Eye.xyz - pos.xyz);

	vec3 VN		= normalize(interp.Normal);

	VN *= sign(dot(view, VN));

	if (NormalTextureID == 0xffffffff)
	{
		normal = VN;
		roughness = 1.f;
	}

	else
	{
		vec3 VT = normalize(interp.Tangent);
		vec3 VB = normalize(interp.Bitangent);

		vec4 normalTex = texture(sampler2D(MaterialTex[NormalTextureID], sampLinear), interp.Texcoords);
		roughness = 1.f - normalTex.a;

		vec3 NTex;
		NTex.xy		= BumpHeight * (normalTex.xy - 0.5f.xx);
		float fdotz = 1.f - dot(NTex.xy, NTex.xy);
		NTex.z		= sqrt(max(fdotz, 0.f));

		normal = normalize(NTex.z * VN - NTex.x * VT - NTex.y * VB);
	}

	float linearRoughness	= Roughness * roughness;
	roughness = linearRoughness * linearRoughness;

	vec3 Diffuse		= 0.f.xxx;
	vec3 Specular		= 0.f.xxx;

	vec2 screenSize			= imageSize(AOITCtrlBuffer).xy;
	vec2 texCoords			= gl_FragCoord.xy / screenSize;

	if (EnableGI)
		CascadeGI(Diffuse.rgb, Specular.rgb, pos.xyz, normal);

	if (SunColor.w > 0.f)
	{
		vec3 Illuminance	= SunColor.w * SunColor.rgb * max(0.f, dot(-SunDir.xyz, normal)) * ComputeSunShadow(SunShadowMatrix, pos, SunShadowMap, sampShadow);

		Diffuse.rgb			+= Illuminance * DisneyDiffuse(normal, -SunDir.xyz, view, linearRoughness) * (1.f / 3.1415926f);
		Specular.rgb		+= Illuminance * SpecularGGX(roughness, normal, -SunDir.xyz, view, 0.04f.xxx);
	}

	uint index				= GetLightListIndex(LightListPtr, texCoords, gl_FragCoord.z, Near, Far);
	uint numLights			= index == 0xffffffff ? 0 : LightIndices[index];
	index++;

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
				Illuminance *= GetFilteredShadow(lightID & 0x7fff, pos.xyz);

			Diffuse.rgb		+= Illuminance * DisneyDiffuse(normal, l, view, linearRoughness) * (1.f / 3.1415926f);
			Specular.rgb	+= Illuminance * SpecularGGX(roughness, normal, l, view, 0.04f.xxx); 

			numLights--;
			index++;
		}
	}

	return vec4(albedo.rgb * (Diffuse + Emissive) + Specular, albedo.a);
}





layout(early_fragment_tests) in;
void main( void )
{
	vec4 FinalColor = Shade();

	beginInvocationInterlockARB();

	WriteNewPixelToAOIT(gl_FragCoord.xy, gl_FragCoord.z, FinalColor);

	endInvocationInterlockARB();
}
