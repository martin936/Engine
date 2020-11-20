// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 Texcoords;
} interp;


#define MIN_LOG -12.473931188f
#define MAX_LOG 12.526068812f


layout(location = 0) uniform sampler2D HDRColor;
layout(location = 1) uniform sampler3D LUT;
layout(location = 2) uniform sampler2D Look;


layout(location = 0) out vec4 LDRTarget;

float Luminance(vec3 color)
{
	return dot(color.rgb, vec3(0.2126f, 0.7152f, 0.0722f));
}

void main(void)
{
	vec4 hdr = textureLod(HDRColor, interp.Texcoords, 0);

	vec3 lg2 = clamp(log2(hdr.rgb), vec3(MIN_LOG), vec3(MAX_LOG));

	lg2 = (lg2 - MIN_LOG) / (MAX_LOG - MIN_LOG);

	vec3 desat = clamp(textureLod(LUT, lg2, 0.f).bgr / 0.66f, vec3(0.f), vec3(1.f));

	desat = desat * (4.026068812f - MIN_LOG) + MIN_LOG;

	vec3 color = exp2(desat);

	//float luma = Luminance(color);

	//color *= textureLod(Look, vec2(luma, 0.5f), 0.f).r / max(1e-6f, luma);

	LDRTarget.rgb = pow(color, vec3(1.f / 2.2f));
	LDRTarget.a = hdr.a;
}
