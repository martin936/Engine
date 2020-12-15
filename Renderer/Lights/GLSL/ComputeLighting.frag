#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require


#define SDF_CB_SLOT				14
#define SDF_TEX_SLOT			9


#include "SDF.glsl"
#include "Lighting.glsl"
#include "Clustered.glsl"


layout (binding = 11, std140) uniform cb11
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


layout (binding = 12, std140) uniform cb12
{
	SLight lightData[128];
};


layout (binding = 13, std140) uniform cb13
{
	SLightShadow shadowLightData[128];
};


layout(push_constant) uniform pc0
{
	vec3 Center;
	float MinCellAxis;

	vec3 Size;
	float Bias;

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

layout(binding = 2) uniform texture2D		ZBuffer;
layout(binding = 3) uniform texture2D		NormalTex;
layout(binding = 4) uniform texture2D		InfoTex;
layout(binding = 5) uniform texture2DArray	FilteredShadows;
layout(binding = 6) uniform texture2D		AOMap;
layout(binding = 7) uniform utexture2DArray	IrradianceField;
layout(binding = 8) uniform itexture2DArray	ProbeMetadata;
//layout(binding = 9) uniform texture2DArray	FieldDepth;
layout(binding = 10) uniform sampler		sampLinear;



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
		Diffuse = SkyLightIntensity.xxxx;
		Specular = 0.f.xxxx;
		return;
	}

	vec4 pos = m_InvViewProj * vec4(Texcoords.xy * vec2(2.f, 2.f) - vec2(1.f, 1.f), depth, 1.f);
	pos /= pos.w;

	vec4 normalTex = texelFetch(NormalTex, ivec2(gl_FragCoord.xy), 0);

	vec3 normal = DecodeNormal(normalTex.rga);

	float linearRoughness	= 0.5f * normalTex.b;
	float roughness			= linearRoughness * linearRoughness;

	vec3 view				= normalize(m_Eye.xyz - pos.xyz);

	Diffuse.rgb				= 0.f.xxx;
	Specular.rgb			= 0.f.xxx;

	vec3 giPos = clamp((pos.xyz - Center) / Size + 0.5f, 0.f.xxx, 1.f.xxx);
	
	vec3 bentNormal = normal;

	float AO = 1.f;
	
	if (EnableAO)
	{
		AO = texelFetch(AOMap, ivec2(gl_FragCoord.xy), 0).r;
	}

	if (EnableGI)
		Diffuse.rgb = AO * ComputeGI(IrradianceField, ProbeMetadata, sampLinear, pos.xyz, giPos, Center, Size, normal, -view) * (1.f / 3.14159126f);

	vec2 screenSize			= textureSize(ZBuffer, 0).xy;
	vec2 texCoords			= gl_FragCoord.xy / screenSize;

	if (SunColor.w > 0.f)
	{
		int slice = textureSize(FilteredShadows, 0).z - 1;
		vec3 Illuminance = SunColor.w * SunColor.rgb * max(0.f, dot(-SunDir.xyz, normal)) * texelFetch(FilteredShadows, ivec3(gl_FragCoord.xy, slice), 0).r;

		Diffuse.rgb		+= Illuminance * DisneyDiffuse(normal, -SunDir.xyz, view, linearRoughness) * (1.f / 3.1415926f);
		Specular.rgb	+= Illuminance * SpecularGGX(roughness, normal, -SunDir.xyz, view, 0.04f.xxx);
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
			Specular.rgb	+= Illuminance * SpecularGGX(roughness, normal, l, view, 0.04f.xxx); 

			numLights--;
			index++;
		}
	}

	Diffuse.a = 0.f;
	Specular.a = 0.f;
}
