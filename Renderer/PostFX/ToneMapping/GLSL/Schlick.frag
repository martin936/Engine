// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 Texcoords;
} interp;

/*uniform sampler2D	Tex[1];
#define HDRColor	Tex[0]
*/

layout(location = 0) uniform sampler2D HDRColor;

layout (std430, binding=0) buffer MinMaxBuffer
{
	float MinLuma;
	float MaxLuma;
};

layout(location = 0) out vec4 LDRTarget;

float Luminance(vec3 color)
{
	return dot(color.rgb, vec3(0.2126f, 0.7152f, 0.0722f));
}

void main(void)
{
	vec4 hdr = textureLod(HDRColor, interp.Texcoords, 0);

	float luma = clamp(Luminance(hdr.rgb), MinLuma, MaxLuma);

	float p = MaxLuma / (256 * MinLuma);

	float factor = p / ((p - 1.f) * luma + MaxLuma);

	hdr.rgb *= factor;

	hdr.rgb = pow(hdr.rgb, vec3(1.f / 2.2f));

	LDRTarget = hdr;
}
