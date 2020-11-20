#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(binding = 0) uniform texture2D	HDRColor;
layout(binding = 1) uniform texture2D	AE;
layout(binding = 2) uniform texture3D	LUT;
layout(binding = 3) uniform texture2D	Contrast;
layout(binding = 4) uniform sampler		samp;


layout(location = 0) out vec4 LDRTarget;


float Luminance(vec3 color)
{
    return dot(color, vec3(0.27f, 0.67f, 0.06f));
}


void main(void)
{
	vec2 ae		= texelFetch(AE, ivec2(0), 0).rg;
	vec3 hdr	= texelFetch(HDRColor, ivec2(gl_FragCoord.xy), 0).rgb * 139.26f;

	hdr = clamp(log2(hdr / ae.g) / 65.f, 0.f, 1.f);

	hdr = textureLod(sampler3D(LUT, samp), hdr, 0).bgr / textureLod(sampler3D(LUT, samp), clamp(log2(ae.r / ae.g) / 65.f, 0.f, 1.f).xxx, 0).bgr;

	hdr.r = textureLod(sampler2D(Contrast, samp), vec2(hdr.r, 0), 0).r;
	hdr.g = textureLod(sampler2D(Contrast, samp), vec2(hdr.g, 0), 0).r;
	hdr.b = textureLod(sampler2D(Contrast, samp), vec2(hdr.b, 0), 0).r;

	LDRTarget = vec4(hdr, 0.f);
}
