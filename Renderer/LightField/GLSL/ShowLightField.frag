#version 450
#extension GL_GOOGLE_include_directive : require

#include "../../Lights/GLSL/Lighting.glsl"

layout(location = 0) in vec2		Texcoord;
layout(location = 1) in flat ivec3	probeID;

#if FP16_IRRADIANCE_PROBES
layout(binding = 0) uniform texture2DArray	IrradianceField;
#else
layout(binding = 0) uniform utexture2DArray	IrradianceField;
#endif

layout(binding = 2) uniform sampler			samp;

layout(binding = 3) uniform texture2DArray	shProbes;

layout(location = 0) out vec4 color;


layout(push_constant) uniform pc0
{
	vec4	m_Up;
	vec4	m_Right;
	vec4	Center;
	vec4	Size;
};


void main() 
{
	vec3 n;
	n.xy = Texcoord * 2.f - 1.f;

	if (dot(n.xy, n.xy) > 1.f)
		discard;

	n.z = sqrt(max(0.f, 1.f - dot(n.xy, n.xy)));

	n = n.x * m_Right.xyz + n.y * m_Up.xyz + n.z * cross(m_Right.xyz, m_Up.xyz);

	vec2 pixcoord = probeID.xy * 10.f + 1.f.xx;
	pixcoord += EncodeOct(n) * 8.f - 0.5f;

	ivec2 texSize = textureSize(IrradianceField, 0).xy;

	vec2 texcoord = (pixcoord + 0.5f) / texSize.xy;

	color.rgb = ExtractSH(shProbes, probeID, n);
	//color.rgb = InterpolateIrradiance(IrradianceField, samp, vec3(texcoord, probeID.z));

	//color.rgb *= color.rgb;
	color.a = 0.f;
}
