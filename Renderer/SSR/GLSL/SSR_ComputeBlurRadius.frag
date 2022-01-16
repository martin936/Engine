#version 450
#extension GL_EXT_samplerless_texture_functions : require


layout(binding = 3, std140) uniform cb3
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
	float screenWidth;
};


layout(binding = 0) uniform texture2D	DepthMap;
layout(binding = 1) uniform texture2D	NormalMap;
layout(binding = 2) uniform texture2D	RayLength;

layout (location = 0) out vec4 Color;


vec3 DecodeNormal(in vec3 e) 
{
	e = e * 2.f - 1.f;
	
	vec2 temp = vec2(e.x + e.y, e.x - e.y) * 0.5;
	vec3 v = vec3(temp, 1.0 - abs(temp.x) - abs(temp.y));

	return normalize(v) * sign(e.z);
}


float roughnessToConeHalfWidth(in float r)
{
    float m = 2.f / (r * r * r * r) - 2.f;

    if (m >= 1e8f)
        return 0.f;

    float cos_a = exp2(-2.035f / (m + 1.f));

    return sqrt(2.f * (1.f - cos_a) / (1.f - cos_a * cos_a) - 1.f);
}



float ComputeMipLevel(float roughness, float dist_to_cam, float dist_to_cone, out float ratio)
{
    float halfWidth = roughnessToConeHalfWidth(roughness);

    ratio = max(1.f, textureSize(DepthMap, 0).x * (dist_to_cone * halfWidth) / (dist_to_cam * screenWidth));

    return clamp(log2(ratio * 0.33f), 0.f, 10.f);
}


void main( void )
{
	float depth = texelFetch(DepthMap, ivec2(gl_FragCoord.xy), 0).r;

	if (depth == 0.f)
		discard;

	float rayLength = abs(texelFetch(RayLength, ivec2(gl_FragCoord.xy), 0).r);

	vec2 size = textureSize(DepthMap, 0).xy;

	vec2 texcoord = gl_FragCoord.xy / size;

	vec4 pos = m_InvViewProj * vec4(texcoord.xy * vec2(2.f, -2.f) - vec2(1.f, -1.f), depth, 1.f);
	pos /= pos.w;

	vec4 normalTex	= texelFetch(NormalMap, ivec2(gl_FragCoord.xy), 0);

	vec3 normal		= DecodeNormal(normalTex.rga);
	float roughness = normalTex.b * normalTex.b;
	vec3 view		= (pos.xyz - m_Eye.xyz);
	float dist2cam	= length(view);

	view /= dist2cam;

	float blur;

	if (rayLength > 65000)
		rayLength = dist2cam;

	float mip		= ComputeMipLevel(roughness, dist2cam, rayLength, blur);

	float radius	= min(blur * 0.5f, 64.f);

	Color			= radius.xxxx;
}