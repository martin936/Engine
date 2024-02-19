#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(binding = 0) uniform texture2D	HDRColor;
layout(binding = 1) uniform texture3D	LUT;
layout(binding = 2) uniform sampler		sampLinear;


layout(location = 0) out vec4 LDRTarget;

layout(push_constant) uniform pc0
{
	mat4 BrightnessContrastSaturation;
};


float Luminance(vec3 color)
{
    return dot(color, vec3(0.27f, 0.67f, 0.06f));
}


vec3 RGB2XYZ_D65(vec3 rgb)
{
	return mat3(vec3(0.5767309f, 0.1855540f, 0.1881852f), vec3(0.2973769f, 0.6273491f, 0.0752741f), vec3(0.0270343f, 0.0706872f, 0.9911085f)) * rgb;
}


void main(void)
{
	/*vec2 ae		= texelFetch(AE, ivec2(0), 0).rg;
	vec3 hdr	= texelFetch(HDRColor, ivec2(gl_FragCoord.xy), 0).rgb * 139.26f;

	hdr = clamp(log2(hdr / ae.g) / 65.f, 0.f, 1.f);

	hdr = textureLod(sampler3D(LUT, samp), hdr, 0).bgr / textureLod(sampler3D(LUT, samp), clamp(log2(ae.r / ae.g) / 65.f, 0.f, 1.f).xxx, 0).bgr;

	hdr.r = textureLod(sampler2D(Contrast, samp), vec2(hdr.r, 0), 0).r;
	hdr.g = textureLod(sampler2D(Contrast, samp), vec2(hdr.g, 0), 0).r;
	hdr.b = textureLod(sampler2D(Contrast, samp), vec2(hdr.b, 0), 0).r;*/

	vec3 color_hdr_srgb = texelFetch(HDRColor, ivec2(gl_FragCoord.xy), 0).rgb;
	vec3 color_hdr_xyz	= RGB2XYZ_D65(color_hdr_srgb);

	vec3 color_log2		= clamp((log2(color_hdr_xyz) + 12.47393f) * 0.04f, 0.f.xxx, 1.f.xxx);

	vec3 color_agx_srgb = textureLod(sampler3D(LUT, sampLinear), color_log2, 0).rgb;

	color_agx_srgb = pow(color_agx_srgb, 1.09f.xxx); 

	color_agx_srgb = (BrightnessContrastSaturation * vec4(color_agx_srgb, 1.f)).rgb;

	LDRTarget = vec4(color_agx_srgb, 0.f);
}
