#version 450
#extension GL_EXT_samplerless_texture_functions : require
#extension GL_GOOGLE_include_directive : require


layout(binding = 6, std140) uniform cb6
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


layout(push_constant) uniform pc0
{
	uint TemporalOffset;
};


layout(binding = 0) uniform texture2D		DepthMap;
layout(binding = 1) uniform texture2D		NormalMap;
layout(binding = 2) uniform texture2D		InfoMap;
layout(binding = 3) uniform texture2D		RayColor;
layout(binding = 4) uniform texture2D		BRDFMap;
layout(binding = 5) uniform sampler			sampLinear;


layout(location = 0) out vec4	Color;


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


vec3 DecodeNormal(in vec3 e) 
{
	e = e * 2.f - 1.f;
	
	vec2 temp = vec2(e.x + e.y, e.x - e.y) * 0.5;
	vec3 v = vec3(temp, 1.0 - abs(temp.x) - abs(temp.y));

	return normalize(v) * sign(e.z);
}




void main() 
{
	float depth = texelFetch(DepthMap, ivec2(gl_FragCoord.xy), 0).r;

	vec2 size = textureSize(DepthMap, 0).xy;

	vec2 texcoord = gl_FragCoord.xy / size;

	vec4 pos = m_InvViewProj * vec4(texcoord.xy * vec2(2.f, -2.f) - vec2(1.f, -1.f), depth, 1.f);
	pos /= pos.w;

	vec4 normalTex	= texelFetch(NormalMap, ivec2(gl_FragCoord.xy), 0);
	vec4 infoTex	= texelFetch(InfoMap, ivec2(gl_FragCoord.xy), 0);

	vec3 normal		= DecodeNormal(normalTex.rga);
	float roughness = normalTex.b * normalTex.b;
	vec3 fresnel	= (0.16f * infoTex.g * infoTex.g).xxx;
	vec3 view		= normalize(pos.xyz - m_Eye.xyz);

	vec4 rayColor = texelFetch(sampler2D(RayColor, sampLinear), ivec2(gl_FragCoord.xy), 0);

	vec2 radius = min(128.f, rayColor.w) / size;

	uint seed = uint(gl_FragCoord.y * size.x + gl_FragCoord.x) + TemporalOffset;

	float offset = VanDerCorput2(seed);
	float angle = 2.f * 3.1415926f * VanDerCorput3(seed);

	float sumW = 0.f;
	vec3 color = 0.f.xxx;

	for (int i = 0; i < 64; i++)
	{
		float x = i + offset;
		float r = sqrt(x / 64.f);
		float theta = 3.88322f * x + angle;

		vec2 offset = r * vec2(cos(theta), sin(theta));

		float w = exp(-0.5f * r * r);

		color += textureLod(sampler2D(RayColor, sampLinear), texcoord + radius * offset, 0.f).rgb * w;
		sumW += w;
	}

	color /= sumW;

	vec2 brdf	= textureLod(sampler2D(BRDFMap, sampLinear), vec2(dot(-view, normal), roughness), 0.f).rg;

	Color.rgb = (brdf.x * fresnel + brdf.y) * color;

	Color.a = 0.f;
}
