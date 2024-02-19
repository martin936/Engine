#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(binding = 0) uniform texture2D	HDRColor;
layout(binding = 1) uniform texture3D	LUT;
layout(binding = 2) uniform texture2D	ContrastCurve;
layout(binding = 3) uniform sampler		sampLinear;


layout(location = 0) out vec4 LDRTarget;


float Luminance(vec3 color)
{
    return dot(color, vec3(0.27f, 0.67f, 0.06f));
}


void main(void)
{
	vec3 hdr		= texelFetch(HDRColor, ivec2(gl_FragCoord.xy), 0).rgb;

	mat3 transform	= mat3(	0.842479062253094f,	 0.0784335999999992f, 0.0792237451477643f,
							0.0423282422610123f, 0.878468636469772f,  0.0791661274605434f, 
							0.0423756549057051f, 0.0784336f,		  0.879142973793104f);

	vec3 xyzHDR		= transform * hdr;
	vec3 inputLUT	= clamp((log2(xyzHDR) + 12.47393f) * (1.f / (4.026069f + 12.47393f)), 0.f.xxx, 1.f.xxx);

	vec3 outputLUT	= texture(sampler3D(LUT, sampLinear), inputLUT).rgb; 

	outputLUT.r		= texture(sampler2D(ContrastCurve, sampLinear), vec2(outputLUT.r, 0.5f)).r; 
	outputLUT.g		= texture(sampler2D(ContrastCurve, sampLinear), vec2(outputLUT.g, 0.5f)).r; 
	outputLUT.b		= texture(sampler2D(ContrastCurve, sampLinear), vec2(outputLUT.b, 0.5f)).r; 

	LDRTarget		= vec4(outputLUT, 0.f);
}
