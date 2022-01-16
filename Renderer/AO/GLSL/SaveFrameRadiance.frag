#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(binding = 0) uniform texture2D	Radiance;
layout(binding = 1) uniform texture2D	MotionVector;

layout(location = 0) out vec4 outColor;


void main()
{
	vec4 color = texelFetch(Radiance, ivec2(gl_FragCoord.xy), 0);

	color.w = length(texelFetch(MotionVector, ivec2(gl_FragCoord.xy), 0).rg);

	outColor = color;
}
