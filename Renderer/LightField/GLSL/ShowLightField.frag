#version 450
#extension GL_GOOGLE_include_directive : require

#include "../../Lights/GLSL/Lighting.glsl"

layout(location = 0) in vec2		Texcoord;
layout(location = 1) in flat ivec3	probeID;


layout(binding = 0) uniform utexture2DArray	IrradianceField;
layout(binding = 1) uniform sampler			samp;

layout(location = 0) out vec4 color;


layout(push_constant) uniform pc0
{
	vec4	m_Up;
	vec4	m_Right;
	vec4	Center;
	vec4	Size;
	ivec4	numProbes;
};


vec3 filterIrradiance(vec2 texcoord, ivec3 probeID)
{
	vec2 texscale = 1.f / textureSize(IrradianceField, 0).xy;

	float fx = fract(texcoord.x);
	float fy = fract(texcoord.y);
	texcoord.x -= fx;
	texcoord.y -= fy;

	vec4 xcubic = cubic(fx);
	vec4 ycubic = cubic(fy);

	vec4 c = vec4(texcoord.x - 0.5, texcoord.x + 1.5, texcoord.y - 0.5, texcoord.y + 1.5);
	vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);
	vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;

	offset = clamp(offset, -0.5f.xxxx, 8.5f.xxxx);

	vec3 sample0 = InterpolateIrradiance(IrradianceField, samp, vec3((vec2(offset.x, offset.z) + probeID.xy * 10.f + 1.f) * texscale.xy, probeID.z));
	vec3 sample1 = InterpolateIrradiance(IrradianceField, samp, vec3((vec2(offset.y, offset.z) + probeID.xy * 10.f + 1.f) * texscale.xy, probeID.z));
	vec3 sample2 = InterpolateIrradiance(IrradianceField, samp, vec3((vec2(offset.x, offset.w) + probeID.xy * 10.f + 1.f) * texscale.xy, probeID.z));
	vec3 sample3 = InterpolateIrradiance(IrradianceField, samp, vec3((vec2(offset.y, offset.w) + probeID.xy * 10.f + 1.f) * texscale.xy, probeID.z));

	float sx = s.x / (s.x + s.y);
	float sy = s.z / (s.z + s.w);

	return mix(
	mix(sample3, sample2, sx),
	mix(sample1, sample0, sx), sy);
}


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

	color.rgb = InterpolateIrradiance(IrradianceField, samp, vec3(texcoord, probeID.z));

	color.rgb = pow(color.rgb, 5.f.xxx);
	//color.rgb = filterIrradiance(EncodeOct(n) * 6.f, probeID);
	color.a = 0.f;
}
